#include "pyxis.h"
#include "jobs.h"
#include "lauxlib.h"
#include <abi/clock.h>
#include <abi/console.h>
#include <abi/display.h>
#include <abi/echo.h>
#include <abi/keyboard.h>
#include <abi/stream.h>
#include <abi/memory.h>
#include <abi/net_config.h>
#include <abi/pipe.h>
#include <abi/pointer.h>
#include <abi/profile.h>
#include <abi/random.h>
#include <abi/system_info.h>
#include <abi/tcp.h>
#include <abi/udp.h>
#include <bundle.h>
#include <bundle_launch.h>
#include <clock.h>
#include <command_search.h>
#include <directory.h>
#include <file.h>
#include <handle.h>
#include <launcher.h>
#include <mbedtls/platform.h>
#include <path.h>
#include <process.h>
#include <program.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <pyxis/stdio.h>
#include <pyxis/environment.h>
#include <pyxis/working_path.h>
#include <random.h>
#include <startup.h>
#include <environment.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GUARD_METATABLE "pyxis.resources"
#define ENTROPY_WAIT_NS UINT64_C(2000000000)
#define HASH_CHUNK_SIZE 4096

enum { RESOURCE_LIMIT = 16 };

struct resources {
  handle_t object, child;
  struct command_selection *selection;
  struct command_search_capture search;
  struct bundle_launch *launch;
  char **argv;
  char *argument_bytes;
  struct launch_grant *grants;
  struct launch_environment_entry *entries;
  struct environment_capture parent;
  uint64_t *directories;
  struct pyxis_working_snapshot working;
  struct pyxis_environment_snapshot environment;
  char *name;
  psa_hash_operation_t hash;
  bool hashing;
};

static handle_t entropy_clock, entropy_random;
static uint64_t entropy_deadline;
static bool crypto_ready;

/* PSA seeds its configured RNG even for hashing. Borrow only native grants. */
int mbedtls_platform_get_entropy(psa_driver_get_entropy_flags_t flags,
    size_t *estimate_bits, unsigned char *output, size_t output_size)
{
  *estimate_bits = 0;
  if (flags != PSA_DRIVER_GET_ENTROPY_FLAGS_NONE) {
    return PSA_ERROR_NOT_SUPPORTED;
  }
  size_t offset = 0;
  while (offset < output_size) {
    uint64_t now;
    if (clock_now(entropy_clock, &now) != CALL_OK || now >= entropy_deadline) {
      return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }
    size_t count = output_size - offset;
    if (count > RANDOM_MAX_BYTES) {
      count = RANDOM_MAX_BYTES;
    }
    if (random_read(entropy_random, output + offset, count, entropy_deadline) != CALL_OK) {
      return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }
    offset += count;
  }
  *estimate_bits = output_size * 8;
  return PSA_SUCCESS;
}

void pyxis_finish(void)
{
  if (crypto_ready) {
    mbedtls_psa_crypto_free();
    crypto_ready = false;
  }
  if (entropy_clock != HANDLE_INVALID) {
    handle_close(entropy_clock);
    entropy_clock = HANDLE_INVALID;
  }
  if (entropy_random != HANDLE_INVALID) {
    handle_close(entropy_random);
    entropy_random = HANDLE_INVALID;
  }
}

static bool crypto_init(void)
{
  if (crypto_ready) {
    return true;
  }
  pyxis_finish();
  environment_capability(startup_environment_handle(), "clock", &entropy_clock);
  environment_capability(startup_environment_handle(), "random", &entropy_random);
  uint64_t now;
  if (entropy_random == HANDLE_INVALID ||
      clock_now(entropy_clock, &now) != CALL_OK || now > UINT64_MAX - ENTROPY_WAIT_NS) {
    return false;
  }
  entropy_deadline = now + ENTROPY_WAIT_NS;
  mbedtls_platform_set_calloc_free(calloc, free);
  if (psa_crypto_init() != PSA_SUCCESS) {
    mbedtls_psa_crypto_free();
    return false;
  }
  crypto_ready = true;
  return true;
}

static enum call_status release_resources(struct resources *owned)
{
  enum call_status status = CALL_OK;
  if (owned->hashing) {
    if (psa_hash_abort(&owned->hash) != PSA_SUCCESS) {
      status = CALL_IO;
    }
    owned->hashing = false;
  }
  if (owned->launch) {
    bundle_launch_close(owned->launch);
    owned->launch = NULL;
  }
  command_selection_close(owned->selection);
  owned->selection = NULL;
  command_search_capture_close(&owned->search);
  if (owned->object != HANDLE_INVALID) {
    if (handle_close(owned->object) != 0) {
      status = CALL_BAD_HANDLE;
    }
    owned->object = HANDLE_INVALID;
  }
  if (owned->child != HANDLE_INVALID) {
    if (handle_close(owned->child) != 0) {
      status = CALL_BAD_HANDLE;
    }
    owned->child = HANDLE_INVALID;
  }
  free(owned->argv);
  owned->argv = NULL;
  free(owned->argument_bytes);
  owned->argument_bytes = NULL;
  free(owned->grants);
  owned->grants = NULL;
  free(owned->directories);
  owned->directories = NULL;
  pyxis_working_snapshot_close(&owned->working);
  pyxis_environment_snapshot_close(&owned->environment);
  environment_capture_close(&owned->parent);
  free(owned->entries);
  owned->entries = NULL;
  free(owned->name);
  owned->name = NULL;
  return status;
}

static int close_resources(lua_State *state)
{
  struct resources *owned = lua_touserdata(state, 1);
  release_resources(owned);
  return 0;
}

static struct resources *new_resources(lua_State *state)
{
  struct resources *owned = lua_newuserdatauv(state, sizeof(*owned), 0);
  *owned = (struct resources){.hash = PSA_HASH_OPERATION_INIT};
  luaL_setmetatable(state, GUARD_METATABLE);
  /* Keep every native owner reachable across Lua allocation failures. */
  lua_toclose(state, lua_gettop(state));
  return owned;
}

static const char *status_name(enum call_status status)
{
  switch (status) {
  case CALL_DENIED: return "denied";
  case CALL_NOT_FOUND: return "not found";
  case CALL_WRONG_TYPE: return "wrong object type";
  case CALL_BAD_REQUEST: return "invalid request or path";
  case CALL_UNAVAILABLE: return "resource unavailable";
  case CALL_BAD_OPERATION: return "unsupported operation";
  case CALL_NO_MEMORY: return "out of memory";
  case CALL_LIMIT: return "resource limit exceeded";
  case CALL_ENDPOINT_CLOSED: return "provider closed";
  default: return "native call failed";
  }
}

static int native_error(lua_State *state, struct resources *owned,
    const char *operation, enum call_status status)
{
  enum call_status closed = release_resources(owned);
  if (closed != CALL_OK) {
    return luaL_error(state, "pyxis.%s: %s (status %d); cleanup failed (status %d)",
        operation, status_name(status), (int)status, (int)closed);
  }
  return luaL_error(state, "pyxis.%s: %s (status %d)",
      operation, status_name(status), (int)status);
}

static const char *check_text(lua_State *state, int index)
{
  luaL_checktype(state, index, LUA_TSTRING);
  size_t length;
  const char *text = lua_tolstring(state, index, &length);
  if (memchr(text, 0, length)) {
    luaL_error(state, "pyxis: string contains NUL");
  }
  return text;
}

/* Path resolution borrows the shared libc context for this call. */
static enum call_status open_path(const char *path, uint64_t kind, uint64_t rights,
    handle_t *object)
{
  const struct path_context *context;
  enum call_status status = pyxis_working_context(&context);
  if (status != CALL_OK) {
    return status;
  }
  size_t length = strlen(path), depth = context->count;
  if (length == SIZE_MAX || depth > SIZE_MAX - length - 1 ||
      depth + length + 1 > SIZE_MAX / sizeof(handle_t)) {
    return CALL_LIMIT;
  }
  size_t slots = depth + length + 1;
  handle_t *directories = malloc(slots * sizeof(*directories));
  char *component = malloc(length + 1);
  if (!directories || !component) {
    free(directories);
    free(component);
    return CALL_NO_MEMORY;
  }
  struct path_workspace workspace = {
    .directories = directories, .directory_capacity = slots,
    .component = component, .component_capacity = length + 1,
  };
  status = path_resolve(context, path, kind, rights, &workspace, object);
  free(directories);
  free(component);
  return status;
}

static enum call_status open_program(const char *command, struct resources *owned)
{
  if (!*command) {
    return CALL_BAD_REQUEST;
  }
  const struct path_context *context;
  enum call_status status = pyxis_working_context(&context);
  if (status != CALL_OK) {
    return status;
  }
  size_t length = strlen(command), depth = context->count;
  if (length > SIZE_MAX - PROGRAM_LOOKUP_EXTRA) {
    return CALL_LIMIT;
  }
  size_t capacity = length + PROGRAM_LOOKUP_EXTRA;
  if (depth > SIZE_MAX - capacity || depth + capacity > SIZE_MAX / sizeof(handle_t)) {
    return CALL_LIMIT;
  }
  size_t slots = depth + capacity;
  handle_t *directories = malloc(slots * sizeof(*directories));
  char *component = malloc(capacity);
  if (!directories || !component) {
    free(directories);
    free(component);
    return CALL_NO_MEMORY;
  }
  struct path_workspace workspace = {
    .directories = directories, .directory_capacity = slots,
    .component = component, .component_capacity = capacity,
  };
  status = program_open_search(context, &owned->search, command, &workspace,
      &owned->selection);
  free(directories);
  free(component);
  return status;
}

struct resource_policy {
  const char *name;
  uint64_t protocol, rights;
};

/* Match ordinary shell commands, including their attenuated profile. */
static const struct resource_policy ordinary_resources[] = {
  {"output", PROTOCOL_CONSOLE, CONSOLE_RIGHT_WRITE},
  {"memory", PROTOCOL_MEMORY, MEMORY_RIGHT_MANAGE},
  {"display", PROTOCOL_DISPLAY, DISPLAY_RIGHT_DRAW},
  {"clock", PROTOCOL_CLOCK, CLOCK_RIGHTS},
  {"system_info", PROTOCOL_SYSTEM_INFO, SYSTEM_INFO_RIGHT_READ},
  {"echo", PROTOCOL_ECHO, ECHO_RIGHT_SEND},
  {"udp", PROTOCOL_UDP_SERVICE, UDP_SERVICE_RIGHT_OPEN},
  {"dns_udp", PROTOCOL_UDP_SERVICE, UDP_SERVICE_RIGHT_OPEN},
  {"net_registry", PROTOCOL_NET_REGISTRY, NET_REGISTRY_RIGHT_READ},
  {"tcp", PROTOCOL_TCP_SERVICE, TCP_SERVICE_RIGHT_CONNECT},
  {"random", PROTOCOL_RANDOM, RANDOM_RIGHT_READ},
  {"profile", PROTOCOL_PROFILE, PROFILE_RIGHT_MEMORY | PROFILE_RIGHT_HOST},
  {"launcher", PROTOCOL_LAUNCHER, LAUNCHER_RIGHT_LAUNCH},
};

static_assert(sizeof(ordinary_resources) / sizeof(ordinary_resources[0]) + 3 <= RESOURCE_LIMIT,
    "ordinary resources and input devices fit launch bindings");

static bool runtime_resource_name(const char *name)
{
  if (!strcmp(name, "input") || !strcmp(name, "keyboard") || !strcmp(name, "pointer")) {
    return true;
  }
  for (size_t i = 0; i < sizeof(ordinary_resources) / sizeof(ordinary_resources[0]); ++i) {
    if (!strcmp(name, ordinary_resources[i].name)) {
      return true;
    }
  }
  return false;
}

static enum call_status add_resource(struct resources *owned, struct launch_request *request,
    const struct resource_policy *policy)
{
  const struct environment_value *value = environment_capture_find(&owned->parent, policy->name);
  if (!value || value->kind != ENVIRONMENT_CAP) {
    return CALL_OK;
  }
  struct handle_info info;
  enum call_status status = handle_query(value->handle, &info);
  if (status != CALL_OK) {
    return status;
  }
  if (info.protocol != policy->protocol || info.kind != HANDLE_KIND_NATIVE) {
    return CALL_WRONG_TYPE;
  }
  uint64_t rights = info.rights & policy->rights;
  if (!rights) {
    return CALL_OK;
  }
  owned->entries[request->entry_count++] = (struct launch_environment_entry){
    (uintptr_t)policy->name, ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, request->grant_count, 0};
  owned->grants[request->grant_count++] = (struct launch_grant){value->handle, rights, 0};
  return CALL_OK;
}

static enum call_status prepare_grants(struct resources *owned, struct launch_request *request)
{
  enum call_status status = pyxis_working_snapshot_init(&owned->working, NULL);
  if (status == CALL_OK) {
    status = pyxis_environment_snapshot_init(&owned->environment);
  }
  if (status != CALL_OK) {
    return status;
  }
  const struct path_context *context = &owned->working.context;
  size_t depth = context->count;
  size_t capacity = owned->parent.count + owned->environment.capture.count + RESOURCE_LIMIT + 6;
  if (capacity > LAUNCH_CAPTURE_MAX_SIZE / sizeof(*owned->entries) ||
      depth > LAUNCH_CAPTURE_MAX_SIZE / sizeof(*owned->grants) - capacity) {
    return CALL_LIMIT;
  }
  owned->grants = calloc(capacity + depth, sizeof(*owned->grants));
  owned->entries = calloc(capacity, sizeof(*owned->entries));
  owned->directories = depth ? malloc(depth * sizeof(*owned->directories)) : NULL;
  if (!owned->grants || !owned->entries || (depth && !owned->directories)) {
    return CALL_NO_MEMORY;
  }
  request->grants = (uintptr_t)owned->grants;
  request->entries = (uintptr_t)owned->entries;
  request->working_directories = (uintptr_t)owned->directories;
  request->working_directory_count = depth;
  request->working_path = (uintptr_t)owned->working.path;
  for (size_t i = 0; i < owned->parent.count; ++i) {
    const struct environment_value *value = &owned->parent.values[i];
    if (!strcmp(value->name, "commands") && (value->flags & ENVIRONMENT_EXPORTED)) {
      if (value->kind != ENVIRONMENT_CAP) {
        return CALL_WRONG_TYPE;
      }
      status = command_list_validate(value->handle);
      if (status != CALL_OK) {
        return status;
      }
    }
    if (value->kind != ENVIRONMENT_CAP || !(value->flags & ENVIRONMENT_EXPORTED) ||
        runtime_resource_name(value->name) ||
        !strcmp(value->name, "script") ||
        !strcmp(value->name, "app") || !strcmp(value->name, "job_work") ||
        !strcmp(value->name, "job_provisioner") || !strcmp(value->name, "staging") ||
        !strcmp(value->name, "stdin") ||
        !strcmp(value->name, "stdout") || !strcmp(value->name, "stderr") ||
        !strcmp(value->name, "stdin.append") || !strcmp(value->name, "stdout.append") ||
        !strcmp(value->name, "stderr.append") ||
        !strncmp(value->name, BUNDLE_SOURCE_PREFIX, sizeof(BUNDLE_SOURCE_PREFIX) - 1)) {
      continue;
    }
    struct handle_info info;
    status = handle_query(value->handle, &info);
    if (status != CALL_OK) {
      return status;
    }
    owned->entries[request->entry_count++] = (struct launch_environment_entry){
      (uintptr_t)value->name, ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, request->grant_count, 0};
    owned->grants[request->grant_count++] = (struct launch_grant){value->handle, info.rights, info.transport};
  }
  for (size_t i = 0; i < depth; ++i) {
    struct handle_info info;
    status = handle_query(context->directories[i], &info);
    if (status != CALL_OK) {
      return status;
    }
    if (info.protocol != PROTOCOL_DIRECTORY || info.kind != HANDLE_KIND_NATIVE) {
      return CALL_WRONG_TYPE;
    }
    owned->directories[i] = request->grant_count;
    owned->grants[request->grant_count++] = (struct launch_grant){
      context->directories[i], info.rights & DIRECTORY_RIGHTS, 0};
  }
  /* Live descriptor selection replaces all standard names in child staging. */
  for (size_t i = 0; i < owned->environment.capture.count; ++i) {
    const struct environment_value *value = &owned->environment.capture.values[i];
    if (!strcmp(value->name, "commands")) {
      return CALL_WRONG_TYPE;
    }
    if (!strcmp(value->name, "stdin") || !strcmp(value->name, "stdout") ||
        !strcmp(value->name, "stderr") ||
        !strcmp(value->name, "stdin.append") || !strcmp(value->name, "stdout.append") ||
        !strcmp(value->name, "stderr.append")) {
      continue;
    }
    owned->entries[request->entry_count++] = (struct launch_environment_entry){
      (uintptr_t)value->name, ENVIRONMENT_TEXT, value->flags, (uintptr_t)value->text, value->length};
  }
  FILE *standard[] = {stdin, stdout, stderr};
  const char *stream_names[] = {"stdin", "stdout", "stderr"};
  struct pyxis_stdio_binding streams[ENVIRONMENT_STREAM_COUNT];
  for (size_t i = 0; i < ENVIRONMENT_STREAM_COUNT; ++i) {
    if (pyxis_stdio_stream(standard[i], &streams[i]) != 0) {
      return CALL_BAD_HANDLE;
    }
    if (!streams[i].protocol) {
      continue;
    }
    struct handle_info info;
    status = handle_query(streams[i].handle, &info);
    if (status != CALL_OK) {
      return status;
    }
    bool input = i == ENVIRONMENT_STDIN;
    if (streams[i].append && (input || streams[i].protocol != PROTOCOL_FILE)) {
      return CALL_BAD_REQUEST;
    }
    bool response = streams[i].protocol == PROTOCOL_STREAM;
    if (response && !input) {
      return CALL_WRONG_TYPE;
    }
    uint64_t rights = streams[i].protocol == PROTOCOL_FILE ?
        (input ? FILE_RIGHT_READ : FILE_RIGHT_WRITE) :
        streams[i].protocol == PROTOCOL_PIPE ?
        (input ? PIPE_RIGHT_READ : PIPE_RIGHT_WRITE) :
        response ? STREAM_RIGHT_READ :
        (input ? CONSOLE_RIGHT_READ : CONSOLE_RIGHT_WRITE);
    if (info.protocol != streams[i].protocol || (info.rights & rights) != rights) {
      return CALL_DENIED;
    }
    if (info.kind != HANDLE_KIND_NATIVE &&
        !((streams[i].protocol == PROTOCOL_FILE || response) && info.kind == HANDLE_KIND_EXPORTED)) {
      return CALL_WRONG_TYPE;
    }
    uint64_t transport = info.kind == HANDLE_KIND_EXPORTED ? HANDLE_TRANSPORT_CALL : 0;
    if ((info.transport & transport) != transport) {
      return CALL_DENIED;
    }
    owned->entries[request->entry_count++] = (struct launch_environment_entry){
      (uintptr_t)stream_names[i], ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, request->grant_count, 0};
    owned->grants[request->grant_count++] = (struct launch_grant){streams[i].handle, rights, transport};
    if (streams[i].append) {
      owned->entries[request->entry_count++] = environment_stdio_append_entry(i);
    }
  }
  for (size_t i = 0; i < sizeof(ordinary_resources) / sizeof(ordinary_resources[0]); ++i) {
    status = add_resource(owned, request, &ordinary_resources[i]);
    if (status != CALL_OK) {
      return status;
    }
  }
  bool console_input = streams[ENVIRONMENT_STDIN].protocol == PROTOCOL_CONSOLE;
  bool pager_input = streams[ENVIRONMENT_STDOUT].protocol == PROTOCOL_CONSOLE &&
      (streams[ENVIRONMENT_STDIN].protocol == PROTOCOL_FILE ||
       streams[ENVIRONMENT_STDIN].protocol == PROTOCOL_PIPE);
  const struct resource_policy input = {"input", PROTOCOL_CONSOLE, CONSOLE_RIGHT_READ};
  bool named_input = false;
  if (console_input || pager_input) {
    size_t before = request->entry_count;
    status = add_resource(owned, request, &input);
    if (status != CALL_OK) {
      return status;
    }
    named_input = request->entry_count != before;
  }
  if (console_input && named_input) {
    const struct resource_policy devices[] = {
      {"keyboard", PROTOCOL_KEYBOARD, KEYBOARD_RIGHT_INPUT},
      {"pointer", PROTOCOL_POINTER, POINTER_RIGHT_INPUT},
    };
    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i) {
      status = add_resource(owned, request, &devices[i]);
      if (status != CALL_OK) {
        return status;
      }
    }
  }
  return CALL_OK;
}

static enum call_status prepare_bundle_launch(struct resources *owned,
    const struct launch_request *request)
{
  static const char *names[] = {
    "input", "output", "memory", "display", "clock", "system_info", "echo",
    "udp", "dns_udp", "net_registry", "tcp", "random", "profile", "launcher", "audio", "screen_capture",
    "threads", "pipe", "keyboard", "pointer",
  };
  struct bundle_authority inventory[sizeof(names) / sizeof(names[0]) + 1 + BUNDLE_SOURCE_LIMIT];
  size_t count = 0;
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    const struct environment_value *value = environment_capture_find(&owned->parent, names[i]);
    handle_t source = value && value->kind == ENVIRONMENT_CAP ? value->handle : HANDLE_INVALID;
    if (source != HANDLE_INVALID) {
      struct handle_info info;
      enum call_status status = handle_query(source, &info);
      if (status != CALL_OK) {
        return status;
      }
      inventory[count++] = (struct bundle_authority){names[i], source, info.protocol};
    }
  }
  const struct environment_value *service = environment_capture_find(&owned->parent, "service");
  if (service && service->kind == ENVIRONMENT_CAP) {
    inventory[count++] = (struct bundle_authority){"service", service->handle, PROTOCOL_ENDPOINT_SERVICE};
  }
  size_t source_count;
  enum call_status status = bundle_sources_from_capture(&owned->parent, inventory + count,
      BUNDLE_SOURCE_LIMIT, &source_count);
  if (status != CALL_OK) {
    return status;
  }
  return bundle_launch_prepare(owned->selection->bundle, request, inventory, count + source_count,
      &owned->launch);
}

static int run_program(lua_State *state)
{
  luaL_checktype(state, 1, LUA_TTABLE);
  size_t count = lua_rawlen(state, 1);
  if (!count || count > LAUNCH_CAPTURE_MAX_SIZE / sizeof(char *)) {
    return luaL_error(state, "pyxis.run: expected a nonempty argument list within launch limits");
  }
  struct resources *owned = new_resources(state);
  size_t bytes = 0;
  for (size_t i = 0; i < count; ++i) {
    lua_rawgeti(state, 1, (lua_Integer)i + 1);
    const char *argument = check_text(state, -1);
    size_t length = strlen(argument);
    if (length >= LAUNCH_CAPTURE_MAX_SIZE || bytes > LAUNCH_CAPTURE_MAX_SIZE - length - 1) {
      return luaL_error(state, "pyxis.run: argument strings exceed launch limits");
    }
    bytes += length + 1;
    lua_pop(state, 1);
  }
  lua_pushnil(state);
  while (lua_next(state, 1)) {
    lua_Integer index = lua_tointeger(state, -2);
    if (!lua_isinteger(state, -2) || index < 1 || (lua_Unsigned)index > count) {
      return luaL_error(state, "pyxis.run: expected only integer argument keys from 1 through the list length");
    }
    lua_pop(state, 1);
  }
  owned->argv = malloc(count * sizeof(*owned->argv));
  owned->argument_bytes = malloc(bytes);
  if (!owned->argv || !owned->argument_bytes) {
    return native_error(state, owned, "run", CALL_NO_MEMORY);
  }
  char *destination = owned->argument_bytes;
  for (size_t i = 0; i < count; ++i) {
    lua_rawgeti(state, 1, (lua_Integer)i + 1);
    const char *argument = check_text(state, -1);
    size_t length = strlen(argument) + 1;
    memcpy(destination, argument, length);
    owned->argv[i] = destination;
    destination += length;
    lua_pop(state, 1);
  }
  enum call_status capture_status = environment_capture_init(startup_environment_handle(),
      ENVIRONMENT_SNAPSHOT_INCLUDE_PRIVATE, &owned->parent);
  const struct environment_value *launcher_value = environment_capture_find(&owned->parent, "launcher");
  handle_t launcher = launcher_value && launcher_value->kind == ENVIRONMENT_CAP ?
      launcher_value->handle : HANDLE_INVALID;
  struct handle_info info;
  enum call_status status = capture_status != CALL_OK ? capture_status :
      launcher == HANDLE_INVALID ? CALL_DENIED : handle_query(launcher, &info);
  if (status == CALL_OK && (info.protocol != PROTOCOL_LAUNCHER ||
      info.kind != HANDLE_KIND_NATIVE)) {
    status = CALL_WRONG_TYPE;
  }
  if (status == CALL_OK && !(info.rights & LAUNCHER_RIGHT_LAUNCH)) {
    status = CALL_DENIED;
  }
  struct launch_request request = {.argv = (uintptr_t)owned->argv, .argc = count};
  if (status == CALL_OK && !program_is_explicit(owned->argv[0])) {
    const struct environment_value *commands = environment_capture_find(&owned->parent, "commands");
    if (commands) {
      status = commands->kind == ENVIRONMENT_CAP ?
          command_search_capture_list(commands->handle, &owned->search) : CALL_WRONG_TYPE;
    }
  }
  if (status == CALL_OK) {
    status = open_program(owned->argv[0], owned);
  }
  if (status == CALL_OK) {
    request.image = owned->selection->image;
    status = prepare_grants(owned, &request);
  }
  if (status == CALL_OK && owned->selection->bundle) {
    status = prepare_bundle_launch(owned, &request);
  }
  if (status == CALL_OK) {
    const struct launch_request *prepared = owned->launch ?
        bundle_launch_request(owned->launch) : &request;
    status = program_launch(launcher, prepared, &owned->working.context, &owned->child);
  }
  if (status != CALL_OK) {
    return native_error(state, owned, "run", status);
  }
  /* Captures can retain standard pipe grants: release admission storage before
   * waiting for child completion. The observer alone owns collection. */
  bundle_launch_close(owned->launch);
  owned->launch = NULL;
  command_selection_close(owned->selection);
  owned->selection = NULL;
  command_search_capture_close(&owned->search);
  environment_capture_close(&owned->parent);
  pyxis_environment_snapshot_close(&owned->environment);
  pyxis_working_snapshot_close(&owned->working);
  /* No Lua allocation occurs while a child is launched or being collected. */
  struct process_result result;
  status = process_wait(owned->child, &result);
  if (status != CALL_OK) {
    return native_error(state, owned, "run wait", status);
  }
  status = release_resources(owned);
  if (status != CALL_OK) {
    return native_error(state, owned, "run close", status);
  }
  if (result.kind == PROCESS_START_FAILED) {
    return luaL_error(state, "pyxis.run: child did not start: %s: %s",
        process_start_step_name((uint32_t)result.exit_status),
        process_start_status_name((uint32_t)result.exit_status));
  }
  if (result.kind != PROCESS_EXITED) {
    return luaL_error(state, "pyxis.run: child %s",
        result.kind == PROCESS_FAULTED ? "faulted" : "terminated");
  }
  lua_pushinteger(state, result.exit_status);
  return 1;
}

static int list_directory(lua_State *state)
{
  const char *path = check_text(state, 1);
  struct resources *owned = new_resources(state);
  enum call_status status = open_path(path, DIRECTORY_KIND_DIRECTORY,
      DIRECTORY_RIGHT_ENUMERATE, &owned->object);
  if (status != CALL_OK) {
    return native_error(state, owned, "dir", status);
  }
  size_t capacity = 256;
  owned->name = malloc(capacity);
  if (!owned->name) {
    return native_error(state, owned, "dir", CALL_NO_MEMORY);
  }
  lua_newtable(state);
  struct directory_cursor cursor = {0};
  lua_Integer index = 0;
  for (;;) {
    struct directory_enumerate_reply reply;
    status = directory_enumerate(owned->object, &cursor, owned->name, capacity, &reply);
    if (status != CALL_OK) {
      return native_error(state, owned, "dir", status);
    }
    if (reply.outcome == DIRECTORY_END) {
      break;
    }
    if (reply.outcome == DIRECTORY_CHANGED) {
      release_resources(owned);
      return luaL_error(state, "pyxis.dir: directory changed during enumeration; retry explicitly");
    }
    if (reply.outcome == DIRECTORY_BUFFER_TOO_SMALL) {
      if (reply.name_size <= capacity || reply.name_size > SIZE_MAX) {
        return native_error(state, owned, "dir", CALL_BAD_REQUEST);
      }
      char *larger = realloc(owned->name, (size_t)reply.name_size);
      if (!larger) {
        return native_error(state, owned, "dir", CALL_NO_MEMORY);
      }
      owned->name = larger;
      capacity = (size_t)reply.name_size;
      continue;
    }
    if (reply.kind != DIRECTORY_KIND_FILE && reply.kind != DIRECTORY_KIND_DIRECTORY) {
      release_resources(owned);
      return luaL_error(state, "pyxis.dir: entry has unsupported kind");
    }
    if (index == LUA_MAXINTEGER) {
      return native_error(state, owned, "dir", CALL_LIMIT);
    }
    lua_createtable(state, 0, 2);
    lua_pushstring(state, owned->name);
    lua_setfield(state, -2, "name");
    lua_pushstring(state, reply.kind == DIRECTORY_KIND_FILE ? "file" : "directory");
    lua_setfield(state, -2, "kind");
    lua_rawseti(state, -2, ++index);
    cursor = reply.cursor;
  }
  status = release_resources(owned);
  return status == CALL_OK ? 1 : native_error(state, owned, "dir close", status);
}

static int sleep_seconds(lua_State *state)
{
  struct timespec duration;
  if (lua_isinteger(state, 1)) {
    lua_Integer seconds = lua_tointeger(state, 1);
    luaL_argcheck(state, seconds >= 0, 1, "negative duration");
    duration = (struct timespec){(time_t)seconds, 0};
  } else {
    lua_Number seconds = luaL_checknumber(state, 1);
    luaL_argcheck(state, __builtin_isfinite(seconds), 1, "duration is not finite");
    luaL_argcheck(state, seconds >= 0, 1, "negative duration");
    if (seconds >= 0x1p63) {
      duration = (struct timespec){INT64_MAX, 0};
    } else {
      time_t whole = (time_t)seconds;
      lua_Number fraction = (seconds - (lua_Number)whole) * 1e9;
      long nanoseconds = (long)fraction;
      /* Round up so the sleep never ends early. */
      if ((lua_Number)nanoseconds < fraction) {
        ++nanoseconds;
      }
      if (nanoseconds >= 1000000000L) {
        nanoseconds -= 1000000000L;
        ++whole;
      }
      duration = (struct timespec){whole, nanoseconds};
    }
  }
  if (nanosleep(&duration, NULL)) {
    return luaL_error(state, "pyxis.sleep: %s", strerror(errno));
  }
  return 0;
}

static int hash_file(lua_State *state)
{
  const char *path = check_text(state, 1);
  struct resources *owned = new_resources(state);
  if (!crypto_init()) {
    return luaL_error(state, "pyxis.sha256: crypto initialization failed; clock and random grants required");
  }
  enum call_status status = open_path(path, DIRECTORY_KIND_FILE, FILE_RIGHT_READ, &owned->object);
  if (status != CALL_OK) {
    return native_error(state, owned, "sha256", status);
  }
  owned->hashing = true;
  psa_status_t crypto = psa_hash_setup(&owned->hash, PSA_ALG_SHA_256);
  unsigned char bytes[HASH_CHUNK_SIZE], digest[32];
  uint64_t offset = 0;
  while (crypto == PSA_SUCCESS) {
    size_t count;
    status = file_read(owned->object, offset, bytes, sizeof(bytes), &count);
    if (status != CALL_OK || !count) {
      break;
    }
    if (offset > UINT64_MAX - count) {
      status = CALL_LIMIT;
      break;
    }
    crypto = psa_hash_update(&owned->hash, bytes, count);
    offset += count;
  }
  size_t length = 0;
  if (status == CALL_OK && crypto == PSA_SUCCESS) {
    crypto = psa_hash_finish(&owned->hash, digest, sizeof(digest), &length);
  }
  enum call_status closed = release_resources(owned);
  if (status != CALL_OK) {
    return native_error(state, owned, "sha256 read", status);
  }
  if (closed != CALL_OK) {
    return native_error(state, owned, "sha256 close", closed);
  }
  if (crypto != PSA_SUCCESS || length != sizeof(digest)) {
    return luaL_error(state, "pyxis.sha256: hashing failed (PSA status %d)", (int)crypto);
  }
  static const char hex[] = "0123456789abcdef";
  char result[65];
  for (size_t i = 0; i < sizeof(digest); ++i) {
    result[i * 2] = hex[digest[i] >> 4];
    result[i * 2 + 1] = hex[digest[i] & 15];
  }
  result[64] = 0;
  lua_pushlstring(state, result, 64);
  return 1;
}

static int monotonic_ns(lua_State *state)
{
  uint64_t value;
  handle_t clock = HANDLE_INVALID;
  enum call_status status = environment_capability(startup_environment_handle(), "clock", &clock);
  if (status == CALL_OK) {
    status = clock_now(clock, &value);
    handle_close(clock);
  }
  if (status != CALL_OK) {
    return luaL_error(state, "pyxis.monotonic_ns: native clock read failed (status %d)", status);
  }
  if (value > (uint64_t)LUA_MAXINTEGER) {
    return luaL_error(state, "pyxis.monotonic_ns: timestamp does not fit a Lua integer");
  }
  lua_pushinteger(state, (lua_Integer)value);
  return 1;
}

int luaopen_pyxis(lua_State *state)
{
  if (luaL_newmetatable(state, GUARD_METATABLE)) {
    lua_pushcfunction(state, close_resources);
    lua_setfield(state, -2, "__close");
    lua_pushcfunction(state, close_resources);
    lua_setfield(state, -2, "__gc");
  }
  lua_pop(state, 1);
  const luaL_Reg functions[] = {
    {"run", run_program}, {"dir", list_directory}, {"sha256", hash_file},
    {"monotonic_ns", monotonic_ns}, {"sleep", sleep_seconds}, {NULL, NULL},
  };
  luaL_newlib(state, functions);
  pyxis_jobs_register(state);
  return 1;
}
