#include "internal.h"
#include <abi/clock.h>
#include <abi/console.h>
#include <abi/file.h>
#include <abi/launcher.h>
#include <abi/memory.h>
#include <abi/pipe.h>
#include <fcntl.h>
#include <errno.h>
#include <handle.h>
#include <launcher.h>
#include <path.h>
#include <pipe.h>
#include <process.h>
#include <provider.h>
#include <pyxis/environment.h>
#include <pyxis/job.h>
#include <pyxis/working_path.h>
#include <startup.h>
#include <environment.h>
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
    struct child_stream streams[ENVIRONMENT_STREAM_COUNT], unsigned *needed)
{
  *needed = 1;
  for (size_t i = 0; i < ENVIRONMENT_STREAM_COUNT; ++i) {
    streams[i].parent_fd = -1;
    if (i >= (size_t)options->stdio_count || options->stdio[i].flags == UV_IGNORE) {
      continue;
    }
    const uv_stdio_container_t *stdio = &options->stdio[i];
    unsigned access = i == ENVIRONMENT_STDIN ? PYXIS_DESCRIPTOR_READ : PYXIS_DESCRIPTOR_WRITE;
    if (stdio->flags & UV_CREATE_PIPE) {
      unsigned direction = i == ENVIRONMENT_STDIN ? UV_READABLE_PIPE : UV_WRITABLE_PIPE;
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
      streams[i].parent_access = i == ENVIRONMENT_STDIN ? PYXIS_DESCRIPTOR_WRITE :
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
          !(binding->access & access) || (i == ENVIRONMENT_STDIN && binding->buffered_read)) {
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

static int open_image(const struct path_context *context, const char *path, handle_t *image)
{
  size_t length = strlen(path);
  if (length == SIZE_MAX || context->count > SIZE_MAX - length - 1 ||
      context->count + length + 1 > SIZE_MAX / sizeof(handle_t)) {
    return UV_EOVERFLOW;
  }
  size_t slots = context->count + length + 1;
  handle_t *directories = uv__malloc(slots * sizeof(*directories));
  char *component = uv__malloc(length + 1);
  struct provider_http_workspace *http = provider_http_uri(path) ?
      uv__malloc(sizeof(*http)) : NULL;
  if (!directories || !component || (provider_http_uri(path) && !http)) {
    uv__free(http);
    uv__free(component);
    uv__free(directories);
    return UV_ENOMEM;
  }
  struct path_workspace workspace = {
    .directories = directories, .directory_capacity = slots,
    .component = component, .component_capacity = length + 1,
    .http = http,
  };
  environment_capability(startup_environment_handle(), "clock", &workspace.clock);
  enum call_status status = path_open_file(context, path, FILE_RIGHT_READ, false,
      &workspace, image);
  if (workspace.clock != HANDLE_INVALID) {
    handle_close(workspace.clock);
  }
  uv__free(http);
  uv__free(component);
  uv__free(directories);
  return uv__pyxis_status(status);
}

static int validate_resources(const uv_pyxis_resource_t *resources, size_t count)
{
  if (count && !resources) {
    return UV_EINVAL;
  }
  if (count > LAUNCH_CAPTURE_MAX_SIZE / sizeof(struct launch_environment_entry) - 3) {
    return UV_ENOSPC;
  }
  for (size_t i = 0; i < count; ++i) {
    const char *name = resources[i].name;
    if (!name || !*name) {
      return UV_EINVAL;
    }
    if (strnlen(name, LAUNCH_CAPTURE_MAX_SIZE) == LAUNCH_CAPTURE_MAX_SIZE) {
      return UV_ENOSPC;
    }
    if (!strcmp(name, "memory") || !strcmp(name, "clock") ||
        !strcmp(name, "launcher") || !strcmp(name, "script")) {
      return UV_EINVAL;
    }
    for (size_t j = 0; j < i; ++j) {
      if (!strcmp(name, resources[j].name)) {
        return UV_EINVAL;
      }
    }
  }
  return 0;
}

int uv_pyxis_spawn(uv_loop_t *loop, uv_process_t *process,
    const uv_process_options_t *options, const uv_pyxis_resource_t *extra_resources,
    size_t extra_count)
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
  if (options->flags || options->stdio_count > ENVIRONMENT_STREAM_COUNT ||
      (file_length >= 4 && !strcmp(options->file + file_length - 4, ".pxb"))) {
    return UV_ENOSYS;
  }
  int error = validate_resources(extra_resources, extra_count);
  if (error) {
    return error;
  }
  struct child_stream streams[ENVIRONMENT_STREAM_COUNT] = {0};
  unsigned needed;
  error = prepare_streams(loop, options, streams, &needed);
  if (error) {
    return error;
  }
  error = uv__pyxis_reserve(loop, needed);
  if (error) {
    return error;
  }
  handle_t image = HANDLE_INVALID, launcher = HANDLE_INVALID;
  struct pyxis_working_snapshot working = {0};
  struct pyxis_environment_snapshot inherited_environment = {0};
  struct environment_capture selected = {0};
  struct launch_grant *grants = NULL;
  struct launch_environment_entry *entries = NULL;
  uint64_t *directories = NULL;
  char **environment_names = NULL;
  size_t environment_count = 0, grant_count = 0, entry_count = 0;
  enum call_status snapshot_status = pyxis_working_snapshot_init(&working, options->cwd);
  if (snapshot_status != CALL_OK) {
    error = snapshot_status == CALL_WRONG_TYPE ? UV_ENOTDIR : uv__pyxis_status(snapshot_status);
    goto done;
  }
  snapshot_status = environment_capture_init(startup_environment_handle(), 0, &selected);
  if (snapshot_status != CALL_OK) {
    error = uv__pyxis_status(snapshot_status);
    goto done;
  }
  if (options->env) {
    while (options->env[environment_count]) {
      ++environment_count;
    }
    environment_names = uv__calloc(environment_count ? environment_count : 1,
        sizeof(*environment_names));
    if (!environment_names) {
      error = UV_ENOMEM;
      goto done;
    }
  } else {
    snapshot_status = pyxis_environment_snapshot_init(&inherited_environment);
    if (snapshot_status != CALL_OK) {
      error = uv__pyxis_status(snapshot_status);
      goto done;
    }
    environment_count = inherited_environment.capture.count;
  }
  size_t capacity = selected.count + extra_count + environment_count + 3;
  size_t depth = working.context.count;
  if (capacity > LAUNCH_CAPTURE_MAX_SIZE / sizeof(*entries) ||
      depth > LAUNCH_CAPTURE_MAX_SIZE / sizeof(*grants) - capacity) {
    error = UV_ENOSPC;
    goto done;
  }
  grants = uv__calloc(capacity + depth, sizeof(*grants));
  entries = uv__calloc(capacity ? capacity : 1, sizeof(*entries));
  directories = uv__calloc(depth ? depth : 1, sizeof(*directories));
  if (!grants || !entries || !directories) {
    error = UV_ENOMEM;
    goto done;
  }
  const char *resource_names[] = {"memory", "clock", "launcher"};
  const uint64_t resource_rights[] = {MEMORY_RIGHT_MANAGE, CLOCK_RIGHTS, LAUNCHER_RIGHT_LAUNCH};
  for (size_t i = 0; i < selected.count; ++i) {
    const struct environment_value *value = &selected.values[i];
    if (value->kind != ENVIRONMENT_CAP) {
      continue;
    }
    struct handle_info info;
    enum call_status status = handle_query(value->handle, &info);
    if (status != CALL_OK) {
      error = uv__pyxis_status(status);
      goto done;
    }
    if (!strcmp(value->name, "stdin") || !strcmp(value->name, "stdout") ||
        !strcmp(value->name, "stderr") || !strcmp(value->name, "stdin.status") ||
        !strncmp(value->name, "source.", sizeof("source.") - 1)) {
      continue;
    }
    uint64_t rights = 0;
    if (info.kind == HANDLE_KIND_EXPORTED) {
      rights = info.rights;
    } else if (info.protocol == PROTOCOL_DIRECTORY && strcmp(value->name, "app")) {
      rights = info.rights;
    } else {
      for (size_t role = 0; role < ARRAY_SIZE(resource_names); ++role) {
        if (!strcmp(value->name, resource_names[role])) {
          rights = info.rights & resource_rights[role];
        }
      }
    }
    if (!rights) {
      continue;
    }
    entries[entry_count++] = (struct launch_environment_entry){
      (uintptr_t)value->name, ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, grant_count, 0};
    grants[grant_count++] = (struct launch_grant){value->handle, rights, info.transport};
  }
  for (size_t i = 0; i < extra_count; ++i) {
    const uv_pyxis_resource_t *resource = &extra_resources[i];
    uint64_t rights, transport;
    enum call_status status = handle_rights(resource->source, &rights, &transport);
    if (status != CALL_OK) {
      error = uv__pyxis_status(status);
      goto done;
    }
    if ((resource->rights & ~rights) || (resource->transport & ~transport)) {
      error = UV_EACCES;
      goto done;
    }
    entries[entry_count++] = (struct launch_environment_entry){
      (uintptr_t)resource->name, ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, grant_count, 0};
    grants[grant_count++] = (struct launch_grant){resource->source,
        resource->rights, resource->transport};
  }
  for (size_t i = 0; i < depth; ++i) {
    directories[i] = grant_count;
    error = grant_same(&grants[grant_count++], working.context.directories[i]);
    if (error) {
      goto done;
    }
  }
  for (size_t i = 0; i < environment_count; ++i) {
    const char *name, *value;
    if (options->env) {
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
      name = environment_names[i];
      value = equal + 1;
    } else {
      name = inherited_environment.capture.values[i].name;
      value = inherited_environment.capture.values[i].text;
    }
    entries[entry_count++] = (struct launch_environment_entry){(uintptr_t)name,
      ENVIRONMENT_TEXT, ENVIRONMENT_EXPORTED, (uintptr_t)value, strlen(value)};
  }
  struct launch_request request = {
    .grants = (uintptr_t)grants,
    .entries = (uintptr_t)entries,
    .working_directories = (uintptr_t)directories,
    .working_directory_count = depth,
    .working_path = (uintptr_t)working.path,
    .argv = (uintptr_t)options->args,
  };
  while (options->args[request.argc]) {
    ++request.argc;
  }
  snapshot_status = environment_capability(startup_environment_handle(), "launcher", &launcher);
  if (snapshot_status != CALL_OK) {
    error = uv__pyxis_status(snapshot_status);
    goto done;
  }
  error = open_image(&working.context, options->file, &image);
  if (error) {
    goto done;
  }
  request.image = image;
  for (size_t i = 0; i < ENVIRONMENT_STREAM_COUNT; ++i) {
    if (streams[i].stream) {
      struct pipe_create_reply pair;
      handle_t service = HANDLE_INVALID;
      enum call_status status = environment_capability(startup_environment_handle(), "pipe", &service);
      if (status == CALL_OK) {
        status = pipe_create(service, &pair);
        handle_close(service);
      }
      if (status != CALL_OK) {
        error = uv__pyxis_status(status);
        goto done;
      }
      handle_t parent = i == ENVIRONMENT_STDIN ? pair.writer : pair.reader;
      streams[i].child = i == ENVIRONMENT_STDIN ? pair.reader : pair.writer;
      streams[i].parent_fd = pyxis_descriptor_adopt(&parent, streams[i].parent_access);
      if (streams[i].parent_fd < 0) {
        error = uv_translate_sys_error(errno);
        handle_close(parent);
        goto done;
      }
      entries[entry_count++] = (struct launch_environment_entry){
        (uintptr_t)(i == ENVIRONMENT_STDIN ? "stdin" : i == ENVIRONMENT_STDOUT ? "stdout" : "stderr"),
        ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, grant_count, 0};
      grants[grant_count++] = (struct launch_grant){streams[i].child,
          i == ENVIRONMENT_STDIN ? PIPE_RIGHT_READ : PIPE_RIGHT_WRITE, 0};
    } else if (streams[i].inherited.handle != HANDLE_INVALID) {
      uint64_t protocol = streams[i].inherited.info.protocol;
      uint64_t rights = protocol == PROTOCOL_PIPE ?
          (i == ENVIRONMENT_STDIN ? PIPE_RIGHT_READ : PIPE_RIGHT_WRITE) :
          (i == ENVIRONMENT_STDIN ? CONSOLE_RIGHT_READ : CONSOLE_RIGHT_WRITE);
      entries[entry_count++] = (struct launch_environment_entry){
        (uintptr_t)(i == ENVIRONMENT_STDIN ? "stdin" : i == ENVIRONMENT_STDOUT ? "stdout" : "stderr"),
        ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, grant_count, 0};
      grants[grant_count++] = (struct launch_grant){streams[i].inherited.handle, rights, 0};
    }
  }
  request.grant_count = grant_count;
  request.entry_count = entry_count;
  handle_t observer;
  enum call_status status = program_launch(launcher, &request,
      &working.context, &observer);
  if (status != CALL_OK) {
    error = uv__pyxis_status(status);
    goto done;
  }
  process->observer = observer;
  process->admitted = 1;
  uv__handle_start(process);
  for (size_t i = 0; i < ENVIRONMENT_STREAM_COUNT; ++i) {
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
  if (image != HANDLE_INVALID) {
    handle_close(image);
  }
  if (launcher != HANDLE_INVALID) {
    handle_close(launcher);
  }
  environment_capture_close(&selected);
  pyxis_environment_snapshot_close(&inherited_environment);
  pyxis_working_snapshot_close(&working);
  for (size_t i = 0; i < ENVIRONMENT_STREAM_COUNT; ++i) {
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
  uv__free(directories);
  uv__free(entries);
  uv__free(grants);
  uv__pyxis_release(loop, needed);
  return error;
}

int uv_pyxis_spawn_job(uv_loop_t *loop, uv_process_t *process,
    handle_t launcher, const struct pyxis_job_options *command,
    uv_exit_cb exit_cb)
{
  if (!loop || !process || !command) {
    return UV_EINVAL;
  }
  uv__handle_init(loop, (uv_handle_t *)process, UV_PROCESS);
  process->observer = HANDLE_INVALID;
  process->exit_reason = 0;
  process->pid = UV_ENOSYS;
  process->exit_cb = exit_cb;
  process->admitted = 0;
  int error = uv__pyxis_reserve(loop, 1);
  if (error) {
    return error;
  }
  handle_t observer = HANDLE_INVALID;
  enum call_status status = pyxis_job_launch_observer(launcher, command, &observer);
  if (status != CALL_OK) {
    uv__pyxis_release(loop, 1);
    return uv__pyxis_status(status);
  }
  process->observer = observer;
  process->admitted = 1;
  uv__handle_start(process);
  return 0;
}

int uv_spawn(uv_loop_t *loop, uv_process_t *process, const uv_process_options_t *options)
{
  return uv_pyxis_spawn(loop, process, options, NULL, 0);
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

int uv_pyxis_process_terminate(uv_process_t *process)
{
  if (!process || process->type != UV_PROCESS || uv__is_closing(process)) {
    return UV_EINVAL;
  }
  if (process->observer == HANDLE_INVALID) {
    return UV_EBADF;
  }
  return uv__pyxis_status(process_terminate(process->observer));
}

int uv_kill(int pid, int signal)
{
  (void)pid;
  (void)signal;
  return UV_ENOSYS;
}
