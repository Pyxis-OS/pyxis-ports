#include "jobs.h"
#include "lauxlib.h"
#include <abi/clock.h>
#include <abi/memory.h>
#include <abi/namespace.h>
#include <abi/pipe.h>
#include <abi/random.h>
#include <abi/system_info.h>
#include <abi/tcp.h>
#include <abi/thread.h>
#include <abi/udp.h>
#include <clock.h>
#include <directory.h>
#include <file.h>
#include <handle.h>
#include <pyxis/pipeline.h>
#include <startup.h>
#include <stdlib.h>
#include <string.h>

#define JOB_METATABLE "pyxis.job"
#define PREPARATION_METATABLE "pyxis.job.preparation"
#define NS_PER_MS UINT64_C(1000000)

static char registry_key;

struct owned_job {
  struct pyxis_pipeline *pipeline;
};

struct preparation {
  struct pyxis_job_options stages[LAUNCH_BATCH_MAX];
  const char **argv[LAUNCH_BATCH_MAX];
  struct path_root *roots;
  size_t root_count;
  struct launch_binding root_bindings[STARTUP_ROOT_LIMIT];
  struct launch_binding resources[8];
  struct launch_grant grants[STARTUP_ROOT_LIMIT + 8];
  size_t grant_count, resource_count;
  struct pyxis_working_snapshot working;
  struct pyxis_environment_snapshot environment;
  struct pyxis_pipeline_options options;
};

struct resource_role {
  const char *name;
  uint64_t protocol, rights;
};

static const struct resource_role roles[] = {
  {"memory", PROTOCOL_MEMORY, MEMORY_RIGHT_MANAGE},
  {"threads", PROTOCOL_THREAD_SERVICE, THREAD_RIGHT_MANAGE},
  {"clock", PROTOCOL_CLOCK, CLOCK_RIGHTS},
  {"random", PROTOCOL_RANDOM, RANDOM_RIGHT_READ},
  {"tcp", PROTOCOL_TCP_SERVICE, TCP_SERVICE_RIGHTS},
  {"udp", PROTOCOL_UDP_SERVICE, UDP_SERVICE_RIGHT_OPEN},
  {"pipe", PROTOCOL_PIPE_SERVICE, PIPE_SERVICE_RIGHT_CREATE},
  {"system_info", PROTOCOL_SYSTEM_INFO, SYSTEM_INFO_RIGHT_READ},
};

static void field(lua_State *state, int index, const char *name)
{
  index = lua_absindex(state, index);
  lua_pushstring(state, name);
  lua_rawget(state, index);
}

static const char *text(lua_State *state, int index)
{
  luaL_checktype(state, index, LUA_TSTRING);
  size_t length;
  const char *value = lua_tolstring(state, index, &length);
  if (memchr(value, 0, length)) {
    luaL_error(state, "pyxis.job: text contains NUL");
  }
  return value;
}

static uint64_t unsigned_integer(lua_State *state, int index)
{
  lua_Integer value = luaL_checkinteger(state, index);
  if (value < 0) {
    luaL_error(state, "pyxis.job: expected a nonnegative integer");
  }
  return (uint64_t)value;
}

static size_t dense_array(lua_State *state, int index, size_t limit, bool nonempty)
{
  luaL_checktype(state, index, LUA_TTABLE);
  index = lua_absindex(state, index);
  size_t count = lua_rawlen(state, index), entries = 0;
  if ((nonempty && !count) || count > limit) {
    luaL_error(state, "pyxis.job: array exceeds native limits or is empty");
  }
  lua_pushnil(state);
  while (lua_next(state, index)) {
    if (!lua_isinteger(state, -2) || lua_tointeger(state, -2) < 1 ||
        (uint64_t)lua_tointeger(state, -2) > count) {
      luaL_error(state, "pyxis.job: expected a dense array");
    }
    ++entries;
    lua_pop(state, 1);
  }
  if (entries != count) {
    luaL_error(state, "pyxis.job: expected a dense array");
  }
  return count;
}

static int discard_preparation(lua_State *state)
{
  struct preparation *owned = lua_touserdata(state, 1);
  for (size_t i = 0; i < LAUNCH_BATCH_MAX; ++i) {
    free(owned->argv[i]);
    owned->argv[i] = NULL;
    if (owned->stages[i].image != HANDLE_INVALID) {
      handle_close(owned->stages[i].image);
      owned->stages[i].image = HANDLE_INVALID;
    }
  }
  for (size_t i = 0; i < owned->root_count; ++i) {
    handle_close(owned->roots[i].handle);
  }
  free(owned->roots);
  owned->roots = NULL;
  owned->root_count = 0;
  pyxis_working_snapshot_close(&owned->working);
  pyxis_environment_snapshot_close(&owned->environment);
  return 0;
}

static void registry(lua_State *state)
{
  lua_rawgetp(state, LUA_REGISTRYINDEX, &registry_key);
}

static void register_job(lua_State *state, int index, bool present)
{
  index = lua_absindex(state, index);
  registry(state);
  lua_pushvalue(state, index);
  if (present) {
    lua_pushboolean(state, 1);
  } else {
    lua_pushnil(state);
  }
  lua_rawset(state, -3);
  lua_pop(state, 1);
}

static enum call_status add_role(struct preparation *owned,
    const struct resource_role *role, bool required)
{
  handle_t source = startup_resource(role->name);
  if (source == HANDLE_INVALID) {
    return required ? CALL_UNAVAILABLE : CALL_OK;
  }
  struct handle_info info;
  enum call_status status = handle_query(source, &info);
  if (status != CALL_OK) {
    return status;
  }
  if (info.protocol != role->protocol) {
    return CALL_WRONG_TYPE;
  }
  uint64_t rights = info.rights & role->rights;
  if (!rights) {
    return required ? CALL_DENIED : CALL_OK;
  }
  uint64_t transport = info.kind == HANDLE_KIND_EXPORTED ? HANDLE_TRANSPORT_CALL : 0;
  owned->resources[owned->resource_count++] =
      (struct launch_binding){(uintptr_t)role->name, owned->grant_count};
  owned->grants[owned->grant_count++] = (struct launch_grant){source, rights, transport};
  return CALL_OK;
}

static enum call_status select_resources(lua_State *state, int options,
    struct preparation *owned)
{
  field(state, options, "resources");
  enum call_status status = CALL_OK;
  if (lua_isnil(state, -1)) {
    for (size_t i = 0; i < 4 && status == CALL_OK; ++i) {
      status = add_role(owned, &roles[i], false);
    }
  } else {
    size_t count = dense_array(state, -1, sizeof(roles) / sizeof(roles[0]), false);
    bool selected[sizeof(roles) / sizeof(roles[0])] = {0};
    for (size_t i = 0; i < count; ++i) {
      lua_rawgeti(state, -1, (lua_Integer)i + 1);
      const char *name = text(state, -1);
      size_t role;
      for (role = 0; role < sizeof(roles) / sizeof(roles[0]); ++role) {
        if (!strcmp(name, roles[role].name)) {
          break;
        }
      }
      if (role == sizeof(roles) / sizeof(roles[0]) || selected[role]) {
        luaL_error(state, "pyxis.job: unknown or duplicate resource selector");
      }
      selected[role] = true;
      if (status == CALL_OK) {
        status = add_role(owned, &roles[role], true);
      }
      lua_pop(state, 1);
    }
  }
  lua_pop(state, 1);
  return status;
}

static handle_t root_source(const struct path_context *current, const char *name)
{
  if (!current->roots) {
    return startup_root(name);
  }
  for (size_t i = 0; i < current->root_count; ++i) {
    if (!strcmp(current->roots[i].name, name)) {
      return current->roots[i].handle;
    }
  }
  return HANDLE_INVALID;
}

static enum call_status add_root(struct preparation *owned, const char *name,
    handle_t source, bool read_only)
{
  if (source == HANDLE_INVALID) {
    return CALL_NOT_FOUND;
  }
  for (size_t i = 0; i < owned->root_count; ++i) {
    if (!strcmp(owned->roots[i].name, name)) {
      return CALL_BAD_REQUEST;
    }
  }
  struct handle_info info;
  enum call_status status = handle_query(source, &info);
  if (status != CALL_OK) {
    return status;
  }
  if (info.protocol != PROTOCOL_DIRECTORY || info.kind != HANDLE_KIND_NATIVE) {
    return CALL_WRONG_TYPE;
  }
  uint64_t mask = read_only ? DIRECTORY_RIGHT_LOOKUP | DIRECTORY_RIGHT_ENUMERATE |
      DIRECTORY_RIGHT_READ_FILES : DIRECTORY_RIGHTS;
  handle_t copy;
  status = handle_copy_restricted(source, info.rights & mask, 0, &copy);
  if (status != CALL_OK) {
    return status;
  }
  size_t i = owned->root_count++;
  owned->roots[i] = (struct path_root){name, copy};
  owned->root_bindings[i] = (struct launch_binding){(uintptr_t)name, owned->grant_count};
  owned->grants[owned->grant_count++] = (struct launch_grant){copy, info.rights & mask, 0};
  return CALL_OK;
}

static enum call_status select_roots(lua_State *state, int options,
    const struct path_context *current, struct preparation *owned, bool *explicit)
{
  field(state, options, "roots");
  *explicit = !lua_isnil(state, -1);
  size_t count = *explicit ? dense_array(state, -1, STARTUP_ROOT_LIMIT, false) :
      current->roots ? current->root_count : startup_root_count();
  if (count > STARTUP_ROOT_LIMIT) {
    lua_pop(state, 1);
    return CALL_LIMIT;
  }
  /* A non-NULL empty set must not fall back to ambient startup roots. */
  owned->roots = calloc(count + 1, sizeof(*owned->roots));
  if (!owned->roots) {
    lua_pop(state, 1);
    return CALL_NO_MEMORY;
  }
  enum call_status status = CALL_OK;
  for (size_t i = 0; i < count; ++i) {
    const char *name;
    bool read_only = false;
    if (*explicit) {
      lua_rawgeti(state, -1, (lua_Integer)i + 1);
      luaL_checktype(state, -1, LUA_TTABLE);
      field(state, -1, "name");
      name = text(state, -1);
      lua_pop(state, 1);
      field(state, -1, "access");
      const char *access = text(state, -1);
      if (!strcmp(access, "read")) {
        read_only = true;
      } else if (strcmp(access, "read-write")) {
        luaL_error(state, "pyxis.job: root access must be read or read-write");
      }
      lua_pop(state, 2);
    } else {
      name = current->roots ? current->roots[i].name :
          (const char *)(uintptr_t)startup_roots()[i].name;
    }
    if (status == CALL_OK) {
      status = add_root(owned, name, root_source(current, name), read_only);
    }
  }
  lua_pop(state, 1);
  return status;
}

static enum call_status change_working(struct pyxis_working_snapshot *working,
    const char *path)
{
  size_t length = strlen(path), slots = working->context.count + length + 1;
  if (slots < length || slots > SIZE_MAX / sizeof(handle_t)) {
    return CALL_LIMIT;
  }
  handle_t *storage = realloc(working->context.directories, slots * sizeof(handle_t));
  if (!storage) {
    return CALL_NO_MEMORY;
  }
  working->context.directories = storage;
  working->context.capacity = slots;
  struct path_workspace workspace = {
    .directories = malloc(slots * sizeof(handle_t)), .directory_capacity = slots,
    .component = malloc(length + 1), .component_capacity = length + 1,
  };
  enum call_status status = workspace.directories && workspace.component ?
      path_change(&working->context, path, &workspace) : CALL_NO_MEMORY;
  free(workspace.directories);
  free(workspace.component);
  return status;
}

static enum call_status select_working(lua_State *state, int options,
    struct preparation *owned, bool explicit_roots)
{
  field(state, options, "cwd");
  bool absent = lua_isboolean(state, -1) && !lua_toboolean(state, -1);
  const char *override = lua_isnil(state, -1) || absent ? NULL : text(state, -1);
  enum call_status status = CALL_OK;
  if (!absent && !explicit_roots) {
    status = pyxis_working_snapshot_init(&owned->working, override);
  } else if (!absent) {
    struct pyxis_working_snapshot resolved = {0};
    status = pyxis_working_snapshot_init(&resolved, override);
    if (status == CALL_OK && resolved.context.count && !resolved.path) {
      status = CALL_UNAVAILABLE;
    }
    if (status == CALL_OK) {
      owned->working.path = resolved.path;
      resolved.path = NULL;
      owned->working.context.roots = owned->roots;
      owned->working.context.root_count = owned->root_count;
      if (owned->working.path) {
        status = change_working(&owned->working, owned->working.path);
      }
    }
    pyxis_working_snapshot_close(&resolved);
  }
  owned->working.context.roots = owned->roots;
  owned->working.context.root_count = owned->root_count;
  lua_pop(state, 1);
  return status;
}

static enum call_status select_environment(lua_State *state, int options,
    struct preparation *owned)
{
  field(state, options, "environment");
  if (lua_isnil(state, -1)) {
    lua_pop(state, 1);
    return pyxis_environment_snapshot_init(&owned->environment);
  }
  luaL_checktype(state, -1, LUA_TTABLE);
  int table = lua_gettop(state);
  size_t count = 0;
  lua_pushnil(state);
  while (lua_next(state, table)) {
    text(state, -2);
    text(state, -1);
    if (++count > LAUNCH_CAPTURE_MAX_SIZE / sizeof(struct startup_variable)) {
      luaL_error(state, "pyxis.job: environment exceeds native limits");
    }
    lua_pop(state, 1);
  }
  owned->environment.variables = count ? calloc(count, sizeof(struct startup_variable)) : NULL;
  if (count && !owned->environment.variables) {
    lua_pop(state, 1);
    return CALL_NO_MEMORY;
  }
  enum call_status status = CALL_OK;
  lua_pushnil(state);
  while (lua_next(state, table)) {
    struct startup_variable *variable = &owned->environment.variables[owned->environment.count++];
    variable->name = (uintptr_t)strdup(text(state, -2));
    variable->value = (uintptr_t)strdup(text(state, -1));
    if (!variable->name || !variable->value) {
      status = CALL_NO_MEMORY;
    }
    lua_pop(state, 1);
  }
  lua_pop(state, 1);
  return status;
}

static enum call_status native_program(const struct path_context *context,
    const char *command, handle_t *image)
{
  size_t length = strlen(command);
  if (!length || length > SIZE_MAX - sizeof("boot://.pxe")) {
    return CALL_BAD_REQUEST;
  }
  size_t capacity = length + sizeof("boot://.pxe"), slots = context->count + capacity;
  if (slots < capacity || slots > SIZE_MAX / sizeof(handle_t)) {
    return CALL_LIMIT;
  }
  struct path_workspace workspace = {
    .directories = malloc(slots * sizeof(handle_t)), .directory_capacity = slots,
    .component = malloc(capacity), .component_capacity = capacity,
  };
  char *path = malloc(capacity);
  if (!workspace.directories || !workspace.component || !path) {
    free(workspace.directories);
    free(workspace.component);
    free(path);
    return CALL_NO_MEMORY;
  }
  enum call_status status;
  if (strchr(command, '/') || strchr(command, ':') ||
      (length >= 4 && !strcmp(command + length - 4, ".pxe"))) {
    status = path_resolve_native(context, command, DIRECTORY_KIND_FILE, FILE_RIGHT_READ, &workspace, image);
  } else {
    memcpy(path, "bin://", 6);
    memcpy(path + 6, command, length);
    memcpy(path + 6 + length, ".pxe", 5);
    status = path_resolve_native(context, path, DIRECTORY_KIND_FILE, FILE_RIGHT_READ, &workspace, image);
    if (status == CALL_NOT_FOUND) {
      memcpy(path, "boot://", 7);
      memcpy(path + 7, command, length);
      memcpy(path + 7 + length, ".pxe", 5);
      status = path_resolve_native(context, path, DIRECTORY_KIND_FILE, FILE_RIGHT_READ, &workspace, image);
    }
  }
  free(workspace.directories);
  free(workspace.component);
  free(path);
  return status;
}

static void arguments(lua_State *state, int index, struct preparation *owned, size_t stage)
{
  size_t count = dense_array(state, index, LAUNCH_CAPTURE_MAX_SIZE / sizeof(char *), true);
  index = lua_absindex(state, index);
  owned->argv[stage] = calloc(count, sizeof(char *));
  if (!owned->argv[stage]) {
    luaL_error(state, "pyxis.job: out of memory");
  }
  size_t bytes = 0;
  for (size_t i = 0; i < count; ++i) {
    lua_rawgeti(state, index, (lua_Integer)i + 1);
    const char *argument = text(state, -1);
    size_t length = strlen(argument);
    if (length >= LAUNCH_CAPTURE_MAX_SIZE || bytes > LAUNCH_CAPTURE_MAX_SIZE - length - 1) {
      luaL_error(state, "pyxis.job: arguments exceed native limits");
    }
    bytes += length + 1;
    owned->argv[stage][i] = argument;
    lua_pop(state, 1);
  }
  owned->stages[stage].argv = owned->argv[stage];
  owned->stages[stage].argc = count;
}

static int start(lua_State *state, bool pipeline)
{
  luaL_checktype(state, 1, LUA_TTABLE);
  luaL_checktype(state, 2, LUA_TTABLE);
  field(state, 2, "timeout_ms");
  uint64_t timeout = unsigned_integer(state, -1);
  if (!timeout || timeout > UINT64_MAX / NS_PER_MS) {
    return luaL_error(state, "pyxis.job: expected a positive timeout within native limits");
  }
  lua_pop(state, 1);
  handle_t launcher = startup_resource("launcher");
  handle_t clock = startup_resource("clock");
  handle_t pipe = startup_resource("pipe");
  handle_t private_root = startup_resource("job_work");
  enum call_status status = CALL_OK;
  uint64_t now, deadline = 0;
  if (!launcher || !clock || !pipe || !private_root) {
    status = CALL_UNAVAILABLE;
  } else {
    status = clock_now(clock, &now);
    if (status == CALL_OK && now > UINT64_MAX - timeout * NS_PER_MS) {
      status = CALL_LIMIT;
    }
    if (status == CALL_OK) {
      deadline = now + timeout * NS_PER_MS;
    }
  }
  struct preparation *owned = lua_newuserdatauv(state, sizeof(*owned), 0);
  *owned = (struct preparation){0};
  luaL_setmetatable(state, PREPARATION_METATABLE);
  lua_toclose(state, lua_gettop(state));
  size_t count = pipeline ? dense_array(state, 1, LAUNCH_BATCH_MAX, true) : 1;
  for (size_t i = 0; i < count; ++i) {
    if (pipeline) {
      lua_rawgeti(state, 1, (lua_Integer)i + 1);
    }
    arguments(state, pipeline ? -1 : 1, owned, i);
    if (pipeline) {
      lua_pop(state, 1);
    }
  }
  field(state, 2, "input");
  owned->options.has_input = !lua_isnil(state, -1);
  if (owned->options.has_input) {
    luaL_checktype(state, -1, LUA_TSTRING);
    owned->options.input = lua_tolstring(state, -1, &owned->options.input_size);
  }
  lua_pop(state, 1);
  field(state, 2, "stdout_limit");
  owned->options.stdout_limit = lua_isnil(state, -1) ? 0 : unsigned_integer(state, -1);
  lua_pop(state, 1);
  field(state, 2, "stderr_limit");
  owned->options.stderr_limit = lua_isnil(state, -1) ? 0 : unsigned_integer(state, -1);
  lua_pop(state, 1);
  field(state, 2, "namespace");
  if (!lua_isnil(state, -1)) {
    luaL_checktype(state, -1, LUA_TBOOLEAN);
  }
  bool namespace = lua_toboolean(state, -1);
  lua_pop(state, 1);
  owned->options.clock = clock;
  owned->options.pipe_service = pipe;
  owned->options.private_root = private_root;
  owned->options.deadline_ns = deadline;
  const struct path_context *current = NULL;
  if (status == CALL_OK) {
    status = pyxis_working_context(&current);
  }
  bool explicit_roots = false;
  if (status == CALL_OK) {
    status = select_roots(state, 2, current, owned, &explicit_roots);
  }
  if (status == CALL_OK) {
    status = select_working(state, 2, owned, explicit_roots);
  }
  if (status == CALL_OK) {
    status = select_environment(state, 2, owned);
  }
  if (status == CALL_OK) {
    status = select_resources(state, 2, owned);
  }
  handle_t namespace_handle = HANDLE_INVALID;
  if (status == CALL_OK && namespace) {
    namespace_handle = current->namespace_handle ? current->namespace_handle : startup_namespace();
    if (!namespace_handle) {
      status = CALL_UNAVAILABLE;
    }
  }
  for (size_t i = 0; i < count && status == CALL_OK; ++i) {
    struct pyxis_job_options *stage = &owned->stages[i];
    /* Image selection uses held caller authority; only the final child
     * inventory uses the selected content roots and independent cwd. */
    status = native_program(current, stage->argv[0], &stage->image);
    stage->grants = owned->grants;
    stage->grant_count = owned->grant_count;
    stage->resources = owned->resources;
    stage->resource_count = owned->resource_count;
    stage->roots = owned->root_bindings;
    stage->root_count = owned->root_count;
    stage->working = &owned->working;
    stage->environment = &owned->environment;
    stage->namespace_handle = namespace_handle;
    for (size_t stream = 0; stream < STARTUP_STREAM_COUNT; ++stream) {
      stage->streams[stream] = -1;
    }
  }
  if (status != CALL_OK) {
    lua_pushnil(state);
    lua_pushinteger(state, status);
    return 2;
  }
  owned->options.stages = owned->stages;
  owned->options.stage_count = count;
  struct owned_job *job = lua_newuserdatauv(state, sizeof(*job), 0);
  *job = (struct owned_job){0};
  luaL_setmetatable(state, JOB_METATABLE);
  int job_index = lua_gettop(state);
  /* Registration precedes every possible admission and survives Lua longjmp. */
  register_job(state, job_index, true);
  status = pyxis_pipeline_start(launcher, &owned->options, &job->pipeline);
  if (!job->pipeline) {
    register_job(state, job_index, false);
    lua_pop(state, 1);
    lua_pushnil(state);
  }
  lua_pushinteger(state, status);
  return 2;
}

static int start_job(lua_State *state)
{
  return start(state, false);
}

static int start_pipeline(lua_State *state)
{
  return start(state, true);
}

static struct owned_job *check_job(lua_State *state)
{
  return luaL_checkudata(state, 1, JOB_METATABLE);
}

static void boolean_field(lua_State *state, const char *name, bool value)
{
  lua_pushboolean(state, value);
  lua_setfield(state, -2, name);
}

static void integer_field(lua_State *state, const char *name, uint64_t value)
{
  lua_pushinteger(state, (lua_Integer)value);
  lua_setfield(state, -2, name);
}

static void output_result(lua_State *state, const struct pyxis_capture_output *output)
{
  lua_createtable(state, 0, 4);
  boolean_field(state, "valid", output->valid);
  boolean_field(state, "truncated", output->truncated);
  integer_field(state, "size", output->size);
  integer_field(state, "returned_size", output->returned_size);
}

static void push_result(lua_State *state, const struct pyxis_pipeline_result *result)
{
  lua_createtable(state, 0, 22);
  boolean_field(state, "complete", result->complete);
  boolean_field(state, "cleanup_unconfirmed", result->cleanup_unconfirmed);
  integer_field(state, "start_status", result->start_status);
  integer_field(state, "error_status", result->error_status);
  integer_field(state, "failed_stage", result->failed_stage);
  integer_field(state, "stop", result->stop);
  integer_field(state, "stage_count", result->stage_count);
  lua_createtable(state, (int)result->stage_count, 0);
  for (size_t i = 0; i < result->stage_count; ++i) {
    lua_pushboolean(state, result->stage_valid[i]);
    lua_rawseti(state, -2, (lua_Integer)i + 1);
  }
  lua_setfield(state, -2, "stage_valid");
  lua_createtable(state, (int)result->stage_count, 0);
  for (size_t i = 0; i < result->stage_count; ++i) {
    lua_createtable(state, 0, 2);
    integer_field(state, "kind", result->stages[i].kind);
    lua_pushinteger(state, (lua_Integer)result->stages[i].exit_status);
    lua_setfield(state, -2, "exit_status");
    lua_rawseti(state, -2, (lua_Integer)i + 1);
  }
  lua_setfield(state, -2, "stages");
  boolean_field(state, "has_input", result->has_input);
  boolean_field(state, "input_closed", result->input_closed);
  boolean_field(state, "input_peer_closed", result->input_peer_closed);
  boolean_field(state, "input_uncertain", result->input_uncertain);
  integer_field(state, "input_size", result->input_size);
  integer_field(state, "input_accepted", result->input_accepted);
  integer_field(state, "input_unsent", result->input_unsent);
  output_result(state, &result->stdout_output);
  lua_setfield(state, -2, "stdout_output");
  output_result(state, &result->stderr_output);
  lua_setfield(state, -2, "stderr_output");
  boolean_field(state, "stdout_incomplete", result->stdout_incomplete);
  boolean_field(state, "stderr_incomplete", result->stderr_incomplete);
}

static int observe(lua_State *state, bool wait)
{
  struct owned_job *job = check_job(state);
  struct pyxis_pipeline_result result = {0};
  enum call_status status = wait ? pyxis_pipeline_wait(job->pipeline, &result) :
      pyxis_pipeline_step(job->pipeline, &result);
  push_result(state, &result);
  lua_pushinteger(state, status);
  return 2;
}

static int step_job(lua_State *state)
{
  return observe(state, false);
}

static int wait_job(lua_State *state)
{
  return observe(state, true);
}

static int cancel_job(lua_State *state)
{
  struct owned_job *job = check_job(state);
  lua_pushinteger(state, pyxis_pipeline_cancel(job->pipeline));
  return 1;
}

static int read_job(lua_State *state)
{
  struct owned_job *job = check_job(state);
  const char *name = text(state, 2);
  enum startup_stream_index stream;
  if (!strcmp(name, "stdout")) {
    stream = STARTUP_STDOUT;
  } else if (!strcmp(name, "stderr")) {
    stream = STARTUP_STDERR;
  } else {
    return luaL_error(state, "pyxis.job: expected stdout or stderr");
  }
  uint64_t offset = unsigned_integer(state, 3), count = unsigned_integer(state, 4);
  if (count > FILE_READ_MAX_BYTES) {
    return luaL_error(state, "pyxis.job: read exceeds native transfer limit");
  }
  char bytes[FILE_READ_MAX_BYTES];
  size_t read = 0;
  enum call_status status = pyxis_pipeline_read(job->pipeline, stream, offset, bytes, count, &read);
  if (status == CALL_OK) {
    lua_pushlstring(state, bytes, read);
  } else {
    lua_pushnil(state);
  }
  lua_pushinteger(state, status);
  return 2;
}

static int close_job(lua_State *state)
{
  struct owned_job *job = check_job(state);
  enum call_status status = pyxis_pipeline_close(&job->pipeline);
  if (!job->pipeline) {
    register_job(state, 1, false);
  }
  lua_pushinteger(state, status);
  return 1;
}

static int list_jobs(lua_State *state)
{
  lua_newtable(state);
  int output = lua_gettop(state);
  registry(state);
  size_t count = 0;
  lua_pushnil(state);
  while (lua_next(state, -2)) {
    lua_pushvalue(state, -2);
    lua_rawseti(state, output, (lua_Integer)++count);
    lua_pop(state, 1);
  }
  lua_pop(state, 1);
  return 1;
}

static int describe_job(lua_State *state)
{
  struct owned_job *job = check_job(state);
  lua_pushstring(state, job->pipeline ? "pyxis.job (owned)" : "pyxis.job (closed)");
  return 1;
}

void pyxis_jobs_register(lua_State *state)
{
  int module = lua_absindex(state, -1);
  if (luaL_newmetatable(state, PREPARATION_METATABLE)) {
    lua_pushcfunction(state, discard_preparation);
    lua_setfield(state, -2, "__close");
    lua_pushcfunction(state, discard_preparation);
    lua_setfield(state, -2, "__gc");
  }
  lua_pop(state, 1);
  if (luaL_newmetatable(state, JOB_METATABLE)) {
    const luaL_Reg methods[] = {
      {"step", step_job}, {"wait", wait_job}, {"cancel", cancel_job},
      {"read", read_job}, {"close", close_job}, {NULL, NULL},
    };
    luaL_newlib(state, methods);
    lua_setfield(state, -2, "__index");
    lua_pushliteral(state, "owned native job");
    lua_setfield(state, -2, "__metatable");
    lua_pushcfunction(state, describe_job);
    lua_setfield(state, -2, "__tostring");
    /* No GC/close hook: explicit retirement owns all native effects. */
  }
  lua_pop(state, 1);
  registry(state);
  if (lua_isnil(state, -1)) {
    lua_pop(state, 1);
    lua_newtable(state);
    lua_rawsetp(state, LUA_REGISTRYINDEX, &registry_key);
  } else {
    lua_pop(state, 1);
  }
  const luaL_Reg functions[] = {
    {"job", start_job}, {"pipeline", start_pipeline}, {"jobs", list_jobs}, {NULL, NULL},
  };
  luaL_setfuncs(state, functions, 0);
  lua_pushinteger(state, CALL_OK);
  lua_setfield(state, module, "CALL_OK");
  lua_pushinteger(state, CALL_BUSY);
  lua_setfield(state, module, "CALL_BUSY");
  lua_pushinteger(state, CALL_IO);
  lua_setfield(state, module, "CALL_IO");
  lua_pushinteger(state, CALL_UNAVAILABLE);
  lua_setfield(state, module, "CALL_UNAVAILABLE");
  lua_pushinteger(state, FILE_READ_MAX_BYTES);
  lua_setfield(state, module, "READ_MAX_BYTES");
  lua_pushinteger(state, -1);
  lua_setfield(state, module, "LAUNCH_NO_STAGE");
  lua_pushinteger(state, PROCESS_EXITED);
  lua_setfield(state, module, "PROCESS_EXITED");
  lua_pushinteger(state, PROCESS_FAULTED);
  lua_setfield(state, module, "PROCESS_FAULTED");
  lua_pushinteger(state, PROCESS_TERMINATED);
  lua_setfield(state, module, "PROCESS_TERMINATED");
}
