#include "internal.h"
#include <abi/clock.h>
#include <abi/console.h>
#include <abi/file.h>
#include <abi/launcher.h>
#include <abi/memory.h>
#include <abi/namespace.h>
#include <abi/pipe.h>
#include <fcntl.h>
#include <errno.h>
#include <handle.h>
#include <launcher.h>
#include <pipe.h>
#include <process.h>
#include <startup.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct child_stream {
  handle_t child;
  int parent_fd;
  unsigned parent_access;
  uv_pipe_t *stream;
  struct pyxis_descriptor_binding inherited;
};

static int prepare_streams(uv_loop_t *loop, const uv_process_options_t *options,
    struct child_stream streams[STARTUP_STREAM_COUNT], unsigned *needed)
{
  *needed = 1;
  for (size_t i = 0; i < STARTUP_STREAM_COUNT; ++i) {
    streams[i].parent_fd = -1;
    if (i >= (size_t)options->stdio_count || options->stdio[i].flags == UV_IGNORE) {
      continue;
    }
    const uv_stdio_container_t *stdio = &options->stdio[i];
    unsigned access = i == STARTUP_STDIN ? PYXIS_DESCRIPTOR_READ : PYXIS_DESCRIPTOR_WRITE;
    if (stdio->flags & UV_CREATE_PIPE) {
      unsigned direction = i == STARTUP_STDIN ? UV_READABLE_PIPE : UV_WRITABLE_PIPE;
      if ((unsigned)stdio->flags != (UV_CREATE_PIPE | direction) || !stdio->data.stream ||
          stdio->data.stream->type != UV_NAMED_PIPE || stdio->data.stream->loop != loop ||
          uv__is_closing(stdio->data.stream)) {
        return UV_ENOSYS;
      }
      uv_pipe_t *stream = (uv_pipe_t *)stdio->data.stream;
      if (stream->fd >= 0 || stream->ipc) {
        return UV_EINVAL;
      }
      for (size_t j = 0; j < i; ++j) {
        if (streams[j].stream == stream) {
          return UV_EINVAL;
        }
      }
      streams[i].stream = stream;
      streams[i].parent_access = i == STARTUP_STDIN ? PYXIS_DESCRIPTOR_WRITE :
          PYXIS_DESCRIPTOR_READ;
      ++*needed;
    } else {
      int fd;
      if (stdio->flags == UV_INHERIT_FD) {
        fd = stdio->data.fd;
      } else if (stdio->flags == UV_INHERIT_STREAM && stdio->data.stream) {
        int error = uv_fileno((uv_handle_t *)stdio->data.stream, &fd);
        if (error) {
          return error;
        }
      } else {
        return UV_ENOSYS;
      }
      if (pyxis_descriptor_borrow(fd, &streams[i].inherited)) {
        return uv_translate_sys_error(errno);
      }
      const struct pyxis_descriptor_binding *binding = &streams[i].inherited;
      if (binding->info.kind != HANDLE_KIND_NATIVE ||
          (binding->info.protocol != PROTOCOL_PIPE && binding->info.protocol != PROTOCOL_CONSOLE) ||
          !(binding->access & access) || (i == STARTUP_STDIN && binding->buffered_read)) {
        return UV_ENOSYS;
      }
    }
  }
  return 0;
}

static int grant_same(struct launch_grant *grant, handle_t source)
{
  uint64_t rights, transport;
  enum call_status status = handle_rights(source, &rights, &transport);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  *grant = (struct launch_grant){source, rights, transport};
  return 0;
}

int uv_spawn(uv_loop_t *loop, uv_process_t *process, const uv_process_options_t *options)
{
  if (!loop || !process || !options) {
    return UV_EINVAL;
  }
  /* Spawn callers close this handle after failure too. It owns no native
   * observer or wait admission until publication succeeds. */
  uv__handle_init(loop, (uv_handle_t *)process, UV_PROCESS);
  process->observer = HANDLE_INVALID;
  process->exit_reason = 0;
  process->pid = UV_ENOSYS;
  process->exit_cb = options->exit_cb;
  process->admitted = 0;
  if (!options->file || !options->args || !options->args[0] ||
      options->stdio_count < 0 || (options->stdio_count && !options->stdio)) {
    return UV_EINVAL;
  }
  size_t file_length = strlen(options->file);
  if (options->flags || options->cwd || options->stdio_count > STARTUP_STREAM_COUNT ||
      (file_length >= 4 && !strcmp(options->file + file_length - 4, ".pxb"))) {
    return UV_ENOSYS;
  }
  struct child_stream streams[STARTUP_STREAM_COUNT] = {0};
  unsigned needed;
  int error = prepare_streams(loop, options, streams, &needed);
  if (error) {
    return error;
  }
  error = uv__pyxis_reserve(loop, needed);
  if (error) {
    return error;
  }
  int image_fd = -1;
  size_t depth = startup_working_directory_count();
  size_t roots_count = startup_root_count();
  const struct startup_binding *startup_bindings = startup_roots();
  struct launch_grant *grants = NULL;
  uint64_t *directories = NULL;
  struct startup_variable *environment = NULL;
  char **environment_names = NULL;
  size_t environment_count = 0;
  if (depth > SIZE_MAX / sizeof(*grants) - roots_count - 7) {
    error = UV_EOVERFLOW;
    goto done;
  }
  grants = uv__calloc(depth + roots_count + 7, sizeof(*grants));
  directories = uv__calloc(depth ? depth : 1, sizeof(*directories));
  if (!grants || !directories) {
    error = UV_ENOMEM;
    goto done;
  }
  const char *resource_names[] = {"memory", "clock", "launcher"};
  const uint64_t resource_rights[] = {MEMORY_RIGHT_MANAGE, CLOCK_RIGHTS, LAUNCHER_RIGHT_LAUNCH};
  struct launch_binding resources[ARRAY_SIZE(resource_names)];
  size_t grant_count = 0, resource_count = 0;
  for (size_t i = 0; i < ARRAY_SIZE(resource_names); ++i) {
    handle_t source = startup_resource(resource_names[i]);
    if (source == HANDLE_INVALID) {
      continue;
    }
    uint64_t rights, transport;
    enum call_status status = handle_rights(source, &rights, &transport);
    if (status != CALL_OK) {
      error = uv__pyxis_status(status);
      goto done;
    }
    resources[resource_count++] = (struct launch_binding){(uintptr_t)resource_names[i], grant_count};
    grants[grant_count++] = (struct launch_grant){source, rights & resource_rights[i], transport};
  }
  struct launch_binding roots[STARTUP_ROOT_LIMIT];
  size_t root_count = 0;
  for (size_t i = 0; i < roots_count; ++i) {
    const char *name = (const char *)(uintptr_t)startup_bindings[i].name;
    if (!strcmp(name, "app")) {
      continue;
    }
    if (root_count == ARRAY_SIZE(roots)) {
      error = UV_ENOSPC;
      goto done;
    }
    error = grant_same(&grants[grant_count], startup_bindings[i].handle);
    if (error) {
      goto done;
    }
    roots[root_count++] = (struct launch_binding){startup_bindings[i].name, grant_count++};
  }
  for (size_t i = 0; i < depth; ++i) {
    directories[i] = grant_count;
    error = grant_same(&grants[grant_count++], startup_working_directory(i));
    if (error) {
      goto done;
    }
  }
  struct launch_request request = {
    .grants = (uintptr_t)grants,
    .resources = (uintptr_t)resources,
    .resource_count = resource_count,
    .roots = (uintptr_t)roots,
    .root_count = root_count,
    .working_directories = (uintptr_t)directories,
    .working_directory_count = depth,
    .working_path = (uintptr_t)startup_working_path(),
    .argv = (uintptr_t)options->args,
  };
  while (options->args[request.argc]) {
    ++request.argc;
  }
  handle_t namespace = startup_namespace();
  if (namespace != HANDLE_INVALID) {
    error = grant_same(&grants[grant_count], namespace);
    if (error) {
      goto done;
    }
    grants[grant_count].rights &= NAMESPACE_RIGHT_LOOKUP;
    request.namespace_grant = ++grant_count;
  }
  if (options->env) {
    while (options->env[environment_count]) {
      ++environment_count;
    }
    environment = uv__calloc(environment_count ? environment_count : 1, sizeof(*environment));
    environment_names = uv__calloc(environment_count ? environment_count : 1, sizeof(*environment_names));
    if (!environment || !environment_names) {
      error = UV_ENOMEM;
      goto done;
    }
    for (size_t i = 0; i < environment_count; ++i) {
      const char *equal = strchr(options->env[i], '=');
      if (!equal || equal == options->env[i]) {
        error = UV_EINVAL;
        goto done;
      }
      environment_names[i] = uv__strndup(options->env[i], (size_t)(equal - options->env[i]));
      if (!environment_names[i]) {
        error = UV_ENOMEM;
        goto done;
      }
      environment[i] = (struct startup_variable){(uintptr_t)environment_names[i], (uintptr_t)(equal + 1)};
    }
    request.environment = (uintptr_t)environment;
    request.environment_count = environment_count;
  } else {
    request.environment = (uintptr_t)startup_environment_variables();
    request.environment_count = startup_environment_count();
  }
  image_fd = open(options->file, O_RDONLY);
  if (image_fd < 0) {
    error = uv_translate_sys_error(errno);
    goto done;
  }
  struct pyxis_descriptor_binding image;
  if (pyxis_descriptor_borrow(image_fd, &image)) {
    error = uv_translate_sys_error(errno);
    goto done;
  }
  request.image = image.handle;
  for (size_t i = 0; i < STARTUP_STREAM_COUNT; ++i) {
    if (streams[i].stream) {
      struct pipe_create_reply pair;
      enum call_status status = pipe_create(startup_resource("pipe"), &pair);
      if (status != CALL_OK) {
        error = uv__pyxis_status(status);
        goto done;
      }
      handle_t parent = i == STARTUP_STDIN ? pair.writer : pair.reader;
      streams[i].child = i == STARTUP_STDIN ? pair.reader : pair.writer;
      streams[i].parent_fd = pyxis_descriptor_adopt(&parent, streams[i].parent_access);
      if (streams[i].parent_fd < 0) {
        error = uv_translate_sys_error(errno);
        handle_close(parent);
        goto done;
      }
      request.streams[i] = (struct launch_stream){PROTOCOL_PIPE, grant_count};
      grants[grant_count++] = (struct launch_grant){streams[i].child,
          i == STARTUP_STDIN ? PIPE_RIGHT_READ : PIPE_RIGHT_WRITE, 0};
    } else if (streams[i].inherited.handle != HANDLE_INVALID) {
      uint64_t protocol = streams[i].inherited.info.protocol;
      uint64_t rights = protocol == PROTOCOL_PIPE ?
          (i == STARTUP_STDIN ? PIPE_RIGHT_READ : PIPE_RIGHT_WRITE) :
          (i == STARTUP_STDIN ? CONSOLE_RIGHT_READ : CONSOLE_RIGHT_WRITE);
      request.streams[i] = (struct launch_stream){protocol, grant_count};
      grants[grant_count++] = (struct launch_grant){streams[i].inherited.handle, rights, 0};
    }
  }
  request.grant_count = grant_count;
  handle_t observer;
  enum call_status status = program_launch(startup_resource("launcher"), &request, NULL, &observer);
  if (status != CALL_OK) {
    error = uv__pyxis_status(status);
    goto done;
  }
  process->observer = observer;
  process->admitted = 1;
  uv__handle_start(process);
  for (size_t i = 0; i < STARTUP_STREAM_COUNT; ++i) {
    uv_pipe_t *stream = streams[i].stream;
    if (stream) {
      stream->fd = streams[i].parent_fd;
      streams[i].parent_fd = -1;
      stream->access = streams[i].parent_access;
      stream->admitted = 1;
      stream->flags |= stream->access == PYXIS_DESCRIPTOR_READ ? UV_HANDLE_READABLE : UV_HANDLE_WRITABLE;
    }
  }
  needed = 0;
  error = 0;

done:
  if (image_fd >= 0) {
    close(image_fd);
  }
  for (size_t i = 0; i < STARTUP_STREAM_COUNT; ++i) {
    if (streams[i].parent_fd >= 0) {
      close(streams[i].parent_fd);
    }
    if (streams[i].child != HANDLE_INVALID) {
      handle_close(streams[i].child);
    }
  }
  if (environment_names) {
    for (size_t i = 0; i < environment_count; ++i) {
      uv__free(environment_names[i]);
    }
  }
  uv__free(environment_names);
  uv__free(environment);
  uv__free(directories);
  uv__free(grants);
  uv__pyxis_release(loop, needed);
  return error;
}

void uv__pyxis_process_dispatch(uv_process_t *process)
{
  struct process_result result;
  enum call_status status = process_wait(process->observer, &result);
  if (status != CALL_OK) {
    abort();
  }
  process->exit_reason = result.kind;
  uv__handle_stop(process);
  if (process->exit_cb) {
    process->exit_cb(process, result.kind == PROCESS_EXITED ? result.exit_status : -1, 0);
  }
}

void uv__pyxis_process_close(uv_process_t *process)
{
  if (process->observer != HANDLE_INVALID) {
    handle_close(process->observer);
    process->observer = HANDLE_INVALID;
  }
  uv__pyxis_release(process->loop, process->admitted);
  process->admitted = 0;
}

int uv_process_kill(uv_process_t *process, int signal)
{
  (void)process;
  (void)signal;
  return UV_ENOSYS;
}

int uv_kill(int pid, int signal)
{
  (void)pid;
  (void)signal;
  return UV_ENOSYS;
}
