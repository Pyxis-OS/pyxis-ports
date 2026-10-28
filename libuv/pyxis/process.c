#include "internal.h"
#include <abi/clock.h>
#include <abi/console.h>
#include <abi/file.h>
#include <abi/stream.h>
#include <abi/launcher.h>
#include <abi/jit.h>
#include <abi/memory.h>
#include <abi/pipe.h>
#include <abi/tcp.h>
#include <bundle.h>
#include <bundle_launch.h>
#include <command_search.h>
#include <fcntl.h>
#include <file.h>
#include <errno.h>
#include <handle.h>
#include <launcher.h>
#include <path.h>
#include <pipe.h>
#include <process.h>
#include <program.h>
#include <program_interpreters.h>
#include <program_runtime.h>
#include <program_sources.h>
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
      bool file = binding->info.protocol == PROTOCOL_FILE;
      bool native = binding->info.kind == HANDLE_KIND_NATIVE && !binding->info.transport;
      bool response = binding->info.protocol == PROTOCOL_STREAM && i == ENVIRONMENT_STDIN;
      bool exported_stream = (file || response) && binding->info.kind == HANDLE_KIND_EXPORTED &&
          (binding->info.transport & HANDLE_TRANSPORT_CALL);
      if ((!native && !exported_stream) ||
          (!file && !response && binding->info.protocol != PROTOCOL_PIPE && binding->info.protocol != PROTOCOL_CONSOLE) ||
          !(binding->access & access) || (i == ENVIRONMENT_STDIN && binding->buffered_read) ||
          (binding->append && (i == ENVIRONMENT_STDIN || !file))) {
        return UV_ENOSYS;
      }
      if (binding->append) {
        uint64_t offset;
        size_t written;
        enum call_status status = file_append(binding->handle, NULL, 0, &offset, &written);
        if (status != CALL_OK) {
          return uv__pyxis_status(status);
        }
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

/* The retained list determines bare-name lookup; explicit paths keep their
 * ordinary meaning. The returned selection owns the image and bundle policy. */
static int open_program(const struct path_context *context,
    const struct command_search_capture *search, const char *path,
    struct command_selection **selection)
{
  size_t length = strlen(path);
  if (length > SIZE_MAX - PROGRAM_LOOKUP_EXTRA ||
      context->count > SIZE_MAX - length - PROGRAM_LOOKUP_EXTRA ||
      context->count + length + PROGRAM_LOOKUP_EXTRA > SIZE_MAX / sizeof(handle_t)) {
    return UV_EOVERFLOW;
  }
  size_t capacity = length + PROGRAM_LOOKUP_EXTRA;
  size_t slots = context->count + capacity;
  handle_t *directories = uv__malloc(slots * sizeof(*directories));
  char *component = uv__malloc(capacity);
  if (!directories || !component) {
    uv__free(component);
    uv__free(directories);
    return UV_ENOMEM;
  }
  struct path_workspace workspace = {
    .directories = directories, .directory_capacity = slots,
    .component = component, .component_capacity = capacity,
  };
  environment_capability(startup_environment_handle(), "clock", &workspace.clock);
  enum call_status status = program_open_search(context, search, path, &workspace, selection);
  if (workspace.clock != HANDLE_INVALID) {
    handle_close(workspace.clock);
  }
  uv__free(component);
  uv__free(directories);
  return uv__pyxis_status(status);
}

/* Embeddings keep the established declaration profile. Standalone interpreter
 * contexts opt into the shared source inventory through program_runtime. */
static bool ordinary_bundle_source(const struct environment_value *value)
{
  static const char *const names[] = {
    "memory", "jit", "clock", "threads", "random", "launcher", "pipe", "system_info",
    "udp", "dns_udp", "net_registry", "echo", "profile", "service", "tcp",
  };
  if (!strcmp(value->name, "service") || !strcmp(value->name, "tcp") ||
      !strcmp(value->name, "jit")) {
    return true;
  }
  if (!(value->flags & ENVIRONMENT_EXPORTED)) {
    return false;
  }
  if (!strncmp(value->name, BUNDLE_SOURCE_PREFIX, sizeof(BUNDLE_SOURCE_PREFIX) - 1)) {
    return true;
  }
  for (size_t i = 0; i < ARRAY_SIZE(names); ++i) {
    if (!strcmp(value->name, names[i])) {
      return true;
    }
  }
  return false;
}

static bool resource_named(const uv_pyxis_resource_t *resources, size_t count,
    const char *name)
{
  for (size_t i = 0; i < count; ++i) {
    if (!strcmp(name, resources[i].name)) {
      return true;
    }
  }
  return false;
}

/* The view borrows the source capture and the caller's restricted copies.
 * Explicit replacements must also bound declarations and later continuations. */
static bool background_source(const char *name, bool continuation)
{
  return strcmp(name, "keyboard") && strcmp(name, "pointer") && strcmp(name, "gamepad") &&
      (!continuation || strcmp(name, "input"));
}

static int select_sources(const struct environment_capture *held, bool runtime,
    bool continuation, const uv_pyxis_resource_t *resources, size_t resource_count,
    handle_t child_launcher,
    struct environment_capture *view, handle_t *restricted,
    struct bundle_authority *authority, size_t capacity, size_t *count,
    size_t *ordinary_count)
{
  for (size_t i = 0; i < held->count; ++i) {
    const struct environment_value *value = &held->values[i];
    if (value->kind != ENVIRONMENT_CAP ||
        resource_named(resources, resource_count, value->name) ||
        (runtime && !background_source(value->name, continuation)) ||
        (!strcmp(value->name, BUNDLE_TCP_LISTEN_SOURCE) &&
         resource_named(resources, resource_count, "tcp")) ||
        (!runtime && !ordinary_bundle_source(value))) {
      continue;
    }
    if (runtime && child_launcher == HANDLE_INVALID &&
        (!strcmp(value->name, "launcher") || !strcmp(value->name, "child_launcher"))) {
      continue;
    }
    view->values[view->count] = *value;
    if (runtime && (!strcmp(value->name, "launcher") ||
        !strcmp(value->name, "child_launcher"))) {
      view->values[view->count].handle = child_launcher;
    }
    ++view->count;
  }
  for (size_t i = 0; i < resource_count; ++i) {
    const uv_pyxis_resource_t *resource = &resources[i];
    if (runtime && !background_source(resource->name, continuation)) {
      continue;
    }
    if (strnlen(resource->name, ENVIRONMENT_NAME_MAX + 1) > ENVIRONMENT_NAME_MAX) {
      return UV_EINVAL;
    }
    enum call_status status = handle_copy_restricted(resource->source,
        resource->rights, resource->transport, &restricted[i]);
    if (status != CALL_OK) {
      return uv__pyxis_status(status);
    }
    struct environment_value *value = &view->values[view->count++];
    strcpy(value->name, resource->name);
    value->kind = ENVIRONMENT_CAP;
    value->flags = ENVIRONMENT_EXPORTED;
    value->handle = restricted[i];
    if (!runtime && !ordinary_bundle_source(value)) {
      --view->count;
    }
  }
  enum call_status status = program_sources_from_capture(view, authority, capacity, count);
  struct bundle_authority configured[BUNDLE_SOURCE_LIMIT];
  size_t configured_count = 0;
  if (status == CALL_OK) {
    status = bundle_sources_from_capture(view, configured, ARRAY_SIZE(configured),
        &configured_count);
  }
  if (status == CALL_OK) {
    *ordinary_count = *count - configured_count;
  }
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

static enum call_status stage_configured_sources(const struct environment_capture *capture,
    struct launch_request *request, struct launch_grant *grants, size_t grant_capacity,
    struct launch_environment_entry *entries, size_t entry_capacity)
{
  for (size_t i = 0; i < capture->count; ++i) {
    const struct environment_value *value = &capture->values[i];
    if (strncmp(value->name, BUNDLE_SOURCE_PREFIX, sizeof(BUNDLE_SOURCE_PREFIX) - 1)) {
      continue;
    }
    size_t entry = 0;
    while (entry < request->entry_count &&
        strcmp(value->name, (const char *)(uintptr_t)entries[entry].name)) {
      ++entry;
    }
    if (entry < request->entry_count) {
      if (entries[entry].kind != ENVIRONMENT_CAP) {
        return CALL_ALREADY_EXISTS;
      }
      /* An explicit configured source is already attenuated in this request. */
      entries[entry].flags = 0;
      continue;
    }
    const struct environment_capture one = {.values = (struct environment_value *)value, .count = 1};
    enum call_status status = bundle_sources_forward_capture(&one, grants, grant_capacity,
        &request->grant_count, entries, entry_capacity, &request->entry_count);
    if (status != CALL_OK) {
      return status;
    }
  }
  return CALL_OK;
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
  process->selection = NULL;
  process->exit_reason = 0;
  process->pid = UV_ENOSYS;
  process->exit_cb = options->exit_cb;
  process->admitted = 0;
  if (!options->file || !options->args || !options->args[0] ||
      options->stdio_count < 0 || (options->stdio_count && !options->stdio)) {
    return UV_EINVAL;
  }
  /* Windows presentation flags have no meaning here, as on other non-Windows
   * platforms; detached, setuid and setgid children do not exist. */
  const unsigned windows_only = UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS |
      UV_PROCESS_WINDOWS_HIDE | UV_PROCESS_WINDOWS_HIDE_CONSOLE |
      UV_PROCESS_WINDOWS_HIDE_GUI | UV_PROCESS_WINDOWS_FILE_PATH_EXACT_NAME;
  if ((options->flags & ~windows_only) || options->stdio_count > ENVIRONMENT_STREAM_COUNT) {
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
  handle_t launcher = HANDLE_INVALID;
  handle_t admission_launcher = HANDLE_INVALID, child_launcher = HANDLE_INVALID;
  bool runtime = program_runtime_active();
  struct command_selection *selection = NULL;
  struct program_image *input = NULL;
  bool continuation = false;
  struct command_search_capture search = {0};
  const char *search_path = NULL;
  struct bundle_launch *bundle_launch = NULL;
  struct pyxis_working_snapshot working = {0};
  struct pyxis_environment_snapshot inherited_environment = {0};
  struct environment_capture selected = {0}, private_held = {0};
  struct environment_capture source_view = {0};
  struct bundle_authority *authority = NULL;
  handle_t *restricted = NULL;
  size_t authority_count = 0, ordinary_count = 0;
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
  snapshot_status = runtime ? program_runtime_route(false, HANDLE_INVALID,
      &admission_launcher, &child_launcher) :
      environment_capability(startup_environment_handle(), "launcher", &launcher);
  if (snapshot_status != CALL_OK) {
    error = uv__pyxis_status(snapshot_status);
    goto done;
  }
  if (!runtime) {
    admission_launcher = launcher;
    child_launcher = launcher;
  }
  if (admission_launcher == HANDLE_INVALID) {
    error = UV_EACCES;
    goto done;
  }
  handle_t command_list = HANDLE_INVALID;
  bool explicit_path = false;
  for (size_t i = 0; i < extra_count; ++i) {
    if (!strcmp(extra_resources[i].name, "path")) {
      command_list = extra_resources[i].source;
      explicit_path = true;
      break;
    }
  }
  if (!explicit_path) {
    const struct environment_value *value = environment_capture_find(&selected, "path");
    if (value) {
      if (value->kind != ENVIRONMENT_CAP) {
        error = uv__pyxis_status(CALL_WRONG_TYPE);
        goto done;
      }
      command_list = value->handle;
    }
  }
  if (command_list != HANDLE_INVALID) {
    snapshot_status = command_search_capture_list(command_list, &search);
    if (snapshot_status != CALL_OK) {
      error = uv__pyxis_status(snapshot_status);
      goto done;
    }
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
  const struct program_sources *runtime_sources = program_runtime_sources();
  size_t source_capacity = runtime_sources ? runtime_sources->capture.count : 0;
  size_t capacity = selected.count + extra_count + environment_count + source_capacity + 9;
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
  error = open_program(&working.context, &search, options->file, &selection);
  if (error) {
    goto done;
  }
  snapshot_status = program_image_prepare(selection->image, &input);
  if (snapshot_status != CALL_OK) {
    error = uv__pyxis_status(snapshot_status);
    goto done;
  }
  if (runtime) {
    const struct program_interpreters *interpreters = program_runtime_interpreters();
    handle_t image = selection->image;
    const char *interpreter = program_image_interpreter(input);
    struct command_selection *interpreter_selection = NULL;
    if (interpreter) {
      if (!strcmp(interpreter, "boot://init-shell.pxe")) {
        image = interpreters->records[PROGRAM_INTERPRETER_SHELL - 1].image;
        error = image == HANDLE_INVALID ? UV_ENOENT : 0;
      } else {
        error = open_program(&working.context, &search, interpreter, &interpreter_selection);
        if (!error && interpreter_selection->bundle) {
          error = UV_EINVAL;
        }
        if (!error) {
          image = interpreter_selection->image;
        }
      }
      if (!error) {
        error = uv__pyxis_status(program_image_select_interpreter(input, image));
      }
    }
    if (!error) {
      continuation = program_interpreters_match(interpreters, image) != PROGRAM_INTERPRETER_NONE;
    }
    command_selection_close(interpreter_selection);
    if (error) {
      goto done;
    }
  }
  const char *resource_names[] = {"memory", "clock", "launcher"};
  const uint64_t resource_rights[] = {MEMORY_RIGHT_MANAGE, CLOCK_RIGHTS, LAUNCHER_RIGHT_LAUNCH};
  for (size_t i = 0; i < selected.count; ++i) {
    const struct environment_value *value = &selected.values[i];
    /* Replaced entries do not participate in validation or inheritance. */
    if (resource_named(extra_resources, extra_count, value->name)) {
      continue;
    }
    if (runtime && !strcmp(value->name, "child_launcher")) {
      continue;
    }
    if (continuation && !background_source(value->name, true)) {
      continue;
    }
    if (runtime && child_launcher == HANDLE_INVALID && !strcmp(value->name, "launcher")) {
      continue;
    }
    if (!strcmp(value->name, "path")) {
      enum call_status status = value->kind == ENVIRONMENT_CAP ?
          command_list_validate(value->handle) : CALL_WRONG_TYPE;
      if (status != CALL_OK) {
        error = uv__pyxis_status(status);
        goto done;
      }
    }
    if (value->kind != ENVIRONMENT_CAP) {
      continue;
    }
    struct handle_info info;
    enum call_status status = handle_query(value->handle, &info);
    if (status != CALL_OK) {
      error = uv__pyxis_status(status);
      goto done;
    }
    if (info.protocol == PROTOCOL_JIT) {
      continue;
    }
    if (!strcmp(value->name, "stdin") || !strcmp(value->name, "stdout") ||
        !strcmp(value->name, "stderr") ||
        !strcmp(value->name, "stdin.append") || !strcmp(value->name, "stdout.append") ||
        !strcmp(value->name, "stderr.append") || !strcmp(value->name, "script") ||
        !strncmp(value->name, "source.", sizeof("source.") - 1)) {
      continue;
    }
    if (!(value->flags & ENVIRONMENT_EXPORTED) ||
        (info.kind != HANDLE_KIND_EXPORTED &&
          (!strcmp(value->name, "app") || !strcmp(value->name, "job_work") ||
           !strcmp(value->name, "job_provisioner") || !strcmp(value->name, "staging")))) {
      continue;
    }
    uint64_t rights = info.rights;
    if (info.kind == HANDLE_KIND_NATIVE && info.protocol == PROTOCOL_TCP_SERVICE) {
      rights &= TCP_SERVICE_RIGHT_CONNECT;
      if (!rights) {
        continue;
      }
    }
    bool runtime_resource = false;
    for (size_t role = 0; role < ARRAY_SIZE(resource_names); ++role) {
      if (!strcmp(value->name, resource_names[role])) {
        rights &= resource_rights[role];
        runtime_resource = true;
        break;
      }
    }
    if (runtime_resource && !rights) {
      continue;
    }
    entries[entry_count++] = (struct launch_environment_entry){
      (uintptr_t)value->name, ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, grant_count, 0};
    handle_t source = runtime && !strcmp(value->name, "launcher") ? child_launcher : value->handle;
    grants[grant_count++] = (struct launch_grant){source, rights, info.transport};
  }
  for (size_t i = 0; i < extra_count; ++i) {
    const uv_pyxis_resource_t *resource = &extra_resources[i];
    if (!strcmp(resource->name, "path")) {
      enum call_status status = command_list_validate(resource->source);
      if (status != CALL_OK || !(resource->rights & COMMAND_LIST_RIGHT_READ) || resource->transport) {
        error = uv__pyxis_status(status != CALL_OK ? status : CALL_DENIED);
        goto done;
      }
    }
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
    if (explicit_path && !strcmp(name, "path.skipped")) {
      continue;
    }
    if (!strcmp(name, "PATH")) {
      search_path = value;
    }
    /* Explicit descriptor selection replaces/removes standard names and the
     * stream companions only in this child's entry staging. */
    if (!strcmp(name, "path")) {
      if (!options->env && explicit_path) {
        continue;
      }
      error = uv__pyxis_status(CALL_WRONG_TYPE);
      goto done;
    }
    if (!strcmp(name, "stdin") || !strcmp(name, "stdout") ||
        !strcmp(name, "stderr") ||
        !strcmp(name, "stdin.append") || !strcmp(name, "stdout.append") ||
        !strcmp(name, "stderr.append")) {
      continue;
    }
    /* Environment text never silently replaces selected capability authority. */
    for (size_t j = 0; j < entry_count; ++j) {
      if (!strcmp(name, (const char *)(uintptr_t)entries[j].name)) {
        error = UV_EINVAL;
        goto done;
      }
    }
    entries[entry_count++] = (struct launch_environment_entry){(uintptr_t)name,
      ENVIRONMENT_TEXT, ENVIRONMENT_EXPORTED, (uintptr_t)value, strlen(value)};
  }
  if (!program_is_explicit(options->file)) {
    snapshot_status = program_search_path_validate(search_path, &search);
    if (snapshot_status != CALL_OK) {
      error = uv__pyxis_status(snapshot_status);
      goto done;
    }
  }
  struct launch_request request = {
    .grants = (uintptr_t)grants,
    .entries = (uintptr_t)entries,
    .working_directories = (uintptr_t)directories,
    .working_directory_count = depth,
    .working_path = (uintptr_t)working.path,
    .argv = (uintptr_t)options->args,
  };
  request.image = selection->image;
  while (options->args[request.argc]) {
    ++request.argc;
  }
  if (selection->bundle || continuation) {
    const struct environment_capture *sources;
    if (runtime) {
      sources = &runtime_sources->capture;
    } else {
      snapshot_status = environment_capture_init(startup_environment_handle(),
          ENVIRONMENT_SNAPSHOT_INCLUDE_PRIVATE, &private_held);
      if (snapshot_status != CALL_OK) {
        error = uv__pyxis_status(snapshot_status);
        goto done;
      }
      sources = &private_held;
    }
    size_t count = sources->count + extra_count;
    source_view.values = uv__calloc(count ? count : 1, sizeof(*source_view.values));
    authority = uv__calloc(count ? count : 1, sizeof(*authority));
    restricted = uv__calloc(extra_count ? extra_count : 1, sizeof(*restricted));
    if (!source_view.values || !authority || !restricted) {
      error = UV_ENOMEM;
      goto done;
    }
    error = select_sources(sources, runtime, continuation, extra_resources, extra_count, child_launcher,
        &source_view, restricted, authority, count, &authority_count, &ordinary_count);
    if (error) {
      goto done;
    }
  }
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
      uint64_t rights = protocol == PROTOCOL_FILE ?
          (i == ENVIRONMENT_STDIN ? FILE_RIGHT_READ : FILE_RIGHT_WRITE) :
          protocol == PROTOCOL_PIPE ?
          (i == ENVIRONMENT_STDIN ? PIPE_RIGHT_READ : PIPE_RIGHT_WRITE) :
          protocol == PROTOCOL_STREAM ? STREAM_RIGHT_READ :
          (i == ENVIRONMENT_STDIN ? CONSOLE_RIGHT_READ : CONSOLE_RIGHT_WRITE);
      uint64_t transport = streams[i].inherited.info.kind == HANDLE_KIND_EXPORTED ?
          HANDLE_TRANSPORT_CALL : 0;
      entries[entry_count++] = (struct launch_environment_entry){
        (uintptr_t)(i == ENVIRONMENT_STDIN ? "stdin" : i == ENVIRONMENT_STDOUT ? "stdout" : "stderr"),
        ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, grant_count, 0};
      grants[grant_count++] = (struct launch_grant){streams[i].inherited.handle, rights, transport};
      if (streams[i].inherited.append) {
        entries[entry_count++] = environment_stdio_append_entry(i);
      }
    }
  }
  request.grant_count = grant_count;
  request.entry_count = entry_count;
  if (continuation) {
    if (!selection->bundle) {
      enum call_status status = program_sources_stage(authority, ordinary_count,
          &request, grants, capacity + depth, entries, capacity);
      if (status == CALL_OK) {
        status = stage_configured_sources(&source_view, &request, grants,
            capacity + depth, entries, capacity);
      }
      if (status != CALL_OK) {
        error = uv__pyxis_status(status);
        goto done;
      }
      for (size_t i = 0; i < request.entry_count; ++i) {
        if (entries[i].kind == ENVIRONMENT_CAP &&
            !strcmp((const char *)(uintptr_t)entries[i].name, "input")) {
          grants[entries[i].value].rights &= CONSOLE_RIGHT_READ;
        }
      }
    }
    enum call_status status = program_interpreters_stage(program_runtime_interpreters(),
        &request, grants, capacity + depth, entries, capacity);
    if (status != CALL_OK) {
      error = uv__pyxis_status(status);
      goto done;
    }
  }
  if (selection->bundle) {
    error = uv__pyxis_status(bundle_launch_prepare(selection->bundle, &request,
        authority, authority_count, &bundle_launch));
    if (error) {
      goto done;
    }
  }
  handle_t observer = HANDLE_INVALID;
  enum call_status status = program_launch_prepared(admission_launcher,
      bundle_launch ? bundle_launch_request(bundle_launch) : &request,
      input, &working.context, &observer);
  bundle_launch_admitted(bundle_launch, status, observer);
  if (status != CALL_OK) {
    error = uv__pyxis_status(status);
    goto done;
  }
  process->observer = observer;
  process->selection = selection;
  selection = NULL;
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
  bundle_launch_close(bundle_launch);
  command_selection_close(selection);
  command_search_capture_close(&search);
  if (launcher != HANDLE_INVALID) {
    handle_close(launcher);
  }
  program_image_close(input);
  if (restricted) {
    for (size_t i = 0; i < extra_count; ++i) {
      if (restricted[i] != HANDLE_INVALID) {
        handle_close(restricted[i]);
      }
    }
  }
  uv__free(restricted);
  uv__free(authority);
  uv__free(source_view.values);
  environment_capture_close(&private_held);
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
  process->selection = NULL;
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
    process->exit_cb(process,
        result.kind == PROCESS_EXITED || result.kind == PROCESS_START_FAILED ?
            result.exit_status : -1, 0);
  }
}

const struct command_selection *uv_pyxis_process_selection(const uv_process_t *process)
{
  return process && process->type == UV_PROCESS ? process->selection : NULL;
}

void uv__pyxis_process_close(uv_process_t *process)
{
  command_selection_close(process->selection);
  process->selection = NULL;
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
