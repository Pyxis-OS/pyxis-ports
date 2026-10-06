#include "pyxis.h"
#include "lauxlib.h"
#include <abi/clock.h>
#include <abi/console.h>
#include <abi/display.h>
#include <abi/echo.h>
#include <abi/keyboard.h>
#include <abi/memory.h>
#include <abi/namespace.h>
#include <abi/pipe.h>
#include <abi/pointer.h>
#include <abi/profile.h>
#include <abi/random.h>
#include <abi/system_info.h>
#include <abi/tcp.h>
#include <abi/udp.h>
#include <clock.h>
#include <directory.h>
#include <file.h>
#include <handle.h>
#include <launcher.h>
#include <mbedtls/platform.h>
#include <path.h>
#include <process.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <pyxis/stdio.h>
#include <random.h>
#include <startup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GUARD_METATABLE "pyxis.resources"
#define ENTROPY_WAIT_NS UINT64_C(2000000000)
#define HASH_CHUNK_SIZE 4096

enum { RESOURCE_LIMIT = 14 };

struct resources {
  handle_t object, child;
  char **argv;
  char *argument_bytes;
  struct launch_grant *grants;
  uint64_t *directories;
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
}

static bool crypto_init(void)
{
  if (crypto_ready) {
    return true;
  }
  entropy_clock = startup_resource("clock");
  entropy_random = startup_resource("random");
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

/* Path resolution borrows the startup cwd, never duplicates or closes it. */
static enum call_status open_path(const char *path, uint64_t kind, uint64_t rights,
    handle_t *object)
{
  size_t length = strlen(path), depth = startup_working_directory_count();
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
  struct path_workspace workspace = {directories, slots, component, length + 1};
  struct path_context context = {
    .directories = (handle_t *)startup_working_directories(), .count = depth,
  };
  enum call_status status = path_resolve(&context, path, kind, rights, &workspace, object);
  free(directories);
  free(component);
  return status;
}

static enum call_status open_program(const char *command, handle_t *image)
{
  if (!*command) {
    return CALL_BAD_REQUEST;
  }
  if (strchr(command, '/')) {
    return open_path(command, DIRECTORY_KIND_FILE, FILE_RIGHT_READ, image);
  }
  size_t length = strlen(command);
  if (length > SIZE_MAX - sizeof("boot://.pxe")) {
    return CALL_LIMIT;
  }
  char *path = malloc(length + sizeof("boot://.pxe"));
  if (!path) {
    return CALL_NO_MEMORY;
  }
  memcpy(path, "bin://", 6);
  memcpy(path + 6, command, length);
  memcpy(path + 6 + length, ".pxe", sizeof(".pxe"));
  enum call_status status = open_path(path, DIRECTORY_KIND_FILE, FILE_RIGHT_READ, image);
  if (status == CALL_NOT_FOUND) {
    memcpy(path, "boot://", 7);
    memcpy(path + 7, command, length);
    memcpy(path + 7 + length, ".pxe", sizeof(".pxe"));
    status = open_path(path, DIRECTORY_KIND_FILE, FILE_RIGHT_READ, image);
  }
  free(path);
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
  {"tcp", PROTOCOL_TCP_SERVICE, TCP_SERVICE_RIGHT_CONNECT},
  {"random", PROTOCOL_RANDOM, RANDOM_RIGHT_READ},
  {"profile", PROTOCOL_PROFILE, PROFILE_RIGHT_MEMORY | PROFILE_RIGHT_FILE | PROFILE_RIGHT_HOST},
  {"launcher", PROTOCOL_LAUNCHER, LAUNCHER_RIGHT_LAUNCH},
};

static_assert(sizeof(ordinary_resources) / sizeof(ordinary_resources[0]) + 3 <= RESOURCE_LIMIT,
    "ordinary resources and input devices fit launch bindings");

static enum call_status add_resource(struct launch_request *request,
    struct launch_grant *grants, struct launch_binding *bindings,
    const struct resource_policy *policy)
{
  handle_t source = startup_resource(policy->name);
  if (source == HANDLE_INVALID) {
    return CALL_OK;
  }
  struct handle_info info;
  enum call_status status = handle_query(source, &info);
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
  bindings[request->resource_count++] =
      (struct launch_binding){(uintptr_t)policy->name, request->grant_count};
  grants[request->grant_count++] = (struct launch_grant){source, rights, 0};
  return CALL_OK;
}

static enum call_status prepare_grants(struct resources *owned, struct launch_request *request,
    struct launch_binding *bindings, struct launch_binding *roots)
{
  size_t root_count = startup_root_count(), depth = startup_working_directory_count();
  if (root_count > STARTUP_ROOT_LIMIT ||
      depth > SIZE_MAX / sizeof(*owned->grants) - root_count - RESOURCE_LIMIT - 4) {
    return CALL_LIMIT;
  }
  size_t capacity = root_count + depth + RESOURCE_LIMIT + STARTUP_STREAM_COUNT + 1;
  owned->grants = calloc(capacity, sizeof(*owned->grants));
  owned->directories = depth ? malloc(depth * sizeof(*owned->directories)) : NULL;
  if (!owned->grants || (depth && !owned->directories)) {
    return CALL_NO_MEMORY;
  }
  struct launch_grant *grants = owned->grants;
  request->grants = (uintptr_t)grants;
  request->resources = (uintptr_t)bindings;
  request->roots = (uintptr_t)roots;
  request->root_count = root_count;
  request->working_directories = (uintptr_t)owned->directories;
  request->working_directory_count = depth;
  request->working_path = (uintptr_t)startup_working_path();
  request->environment = (uintptr_t)startup_environment_variables();
  request->environment_count = startup_environment_count();
  for (size_t i = 0; i < root_count + depth; ++i) {
    handle_t source = i < root_count ? startup_roots()[i].handle :
        startup_working_directory(i - root_count);
    struct handle_info info;
    enum call_status status = handle_query(source, &info);
    if (status != CALL_OK) {
      return status;
    }
    if (info.protocol != PROTOCOL_DIRECTORY || info.kind != HANDLE_KIND_NATIVE) {
      return CALL_WRONG_TYPE;
    }
    if (i < root_count) {
      roots[i] = (struct launch_binding){startup_roots()[i].name, request->grant_count};
    } else {
      owned->directories[i - root_count] = request->grant_count;
    }
    grants[request->grant_count++] =
        (struct launch_grant){source, info.rights & DIRECTORY_RIGHTS, 0};
  }
  FILE *standard[] = {stdin, stdout, stderr};
  struct startup_stream streams[STARTUP_STREAM_COUNT];
  for (size_t i = 0; i < STARTUP_STREAM_COUNT; ++i) {
    if (pyxis_stdio_stream(standard[i], &streams[i]) != 0) {
      return CALL_BAD_HANDLE;
    }
    if (streams[i].protocol == STARTUP_STREAM_NONE) {
      continue;
    }
    struct handle_info info;
    enum call_status status = handle_query(streams[i].handle, &info);
    if (status != CALL_OK) {
      return status;
    }
    bool input = i == STARTUP_STDIN;
    uint64_t rights = streams[i].protocol == PROTOCOL_FILE ?
        (input ? FILE_RIGHT_READ : FILE_RIGHT_WRITE) :
        streams[i].protocol == PROTOCOL_PIPE ?
        (input ? PIPE_RIGHT_READ : PIPE_RIGHT_WRITE) :
        (input ? CONSOLE_RIGHT_READ : CONSOLE_RIGHT_WRITE);
    if (info.protocol != streams[i].protocol || (info.rights & rights) != rights) {
      return CALL_DENIED;
    }
    if (info.kind != HANDLE_KIND_NATIVE &&
        !(streams[i].protocol == PROTOCOL_FILE && info.kind == HANDLE_KIND_EXPORTED)) {
      return CALL_WRONG_TYPE;
    }
    uint64_t transport = info.kind == HANDLE_KIND_EXPORTED ? HANDLE_TRANSPORT_CALL : 0;
    if ((info.transport & transport) != transport) {
      return CALL_DENIED;
    }
    request->streams[i] = (struct launch_stream){streams[i].protocol, request->grant_count};
    grants[request->grant_count++] = (struct launch_grant){streams[i].handle, rights, transport};
  }
  for (size_t i = 0; i < sizeof(ordinary_resources) / sizeof(ordinary_resources[0]); ++i) {
    enum call_status status = add_resource(request, grants, bindings, &ordinary_resources[i]);
    if (status != CALL_OK) {
      return status;
    }
  }
  bool console_input = streams[STARTUP_STDIN].protocol == PROTOCOL_CONSOLE;
  bool pager_input = streams[STARTUP_STDOUT].protocol == PROTOCOL_CONSOLE &&
      (streams[STARTUP_STDIN].protocol == PROTOCOL_FILE ||
       streams[STARTUP_STDIN].protocol == PROTOCOL_PIPE);
  const struct resource_policy input = {"input", PROTOCOL_CONSOLE, CONSOLE_RIGHT_READ};
  bool named_input = false;
  if (console_input || pager_input) {
    size_t before = request->resource_count;
    enum call_status status = add_resource(request, grants, bindings, &input);
    if (status != CALL_OK) {
      return status;
    }
    named_input = request->resource_count != before;
  }
  if (console_input && named_input) {
    const struct resource_policy devices[] = {
      {"keyboard", PROTOCOL_KEYBOARD, KEYBOARD_RIGHT_INPUT},
      {"pointer", PROTOCOL_POINTER, POINTER_RIGHT_INPUT},
    };
    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i) {
      enum call_status status = add_resource(request, grants, bindings, &devices[i]);
      if (status != CALL_OK) {
        return status;
      }
    }
  }
  handle_t namespace = startup_namespace();
  if (namespace != HANDLE_INVALID) {
    struct handle_info info;
    enum call_status status = handle_query(namespace, &info);
    if (status != CALL_OK) {
      return status;
    }
    if (info.protocol != PROTOCOL_NAMESPACE || info.kind != HANDLE_KIND_NATIVE) {
      return CALL_WRONG_TYPE;
    }
    if (info.rights & NAMESPACE_RIGHT_LOOKUP) {
      request->namespace_grant = request->grant_count + 1;
      grants[request->grant_count++] =
          (struct launch_grant){namespace, NAMESPACE_RIGHT_LOOKUP, 0};
    }
  }
  return CALL_OK;
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
  handle_t launcher = startup_resource("launcher");
  struct handle_info info;
  enum call_status status = launcher == HANDLE_INVALID ? CALL_DENIED : handle_query(launcher, &info);
  if (status == CALL_OK && (info.protocol != PROTOCOL_LAUNCHER ||
      info.kind != HANDLE_KIND_NATIVE)) {
    status = CALL_WRONG_TYPE;
  }
  if (status == CALL_OK && !(info.rights & LAUNCHER_RIGHT_LAUNCH)) {
    status = CALL_DENIED;
  }
  struct launch_request request = {.argv = (uintptr_t)owned->argv, .argc = count};
  struct launch_binding bindings[RESOURCE_LIMIT], roots[STARTUP_ROOT_LIMIT];
  if (status == CALL_OK) {
    status = open_program(owned->argv[0], &owned->object);
  }
  if (status == CALL_OK) {
    request.image = owned->object;
    status = prepare_grants(owned, &request, bindings, roots);
  }
  if (status == CALL_OK) {
    struct path_context interpreters = {
      .directories = (handle_t *)startup_working_directories(),
      .count = startup_working_directory_count(),
    };
    status = program_launch(launcher, &request, &interpreters, &owned->child);
  }
  if (status != CALL_OK) {
    return native_error(state, owned, "run", status);
  }
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
    {"run", run_program}, {"dir", list_directory}, {"sha256", hash_file}, {NULL, NULL},
  };
  luaL_newlib(state, functions);
  return 1;
}
