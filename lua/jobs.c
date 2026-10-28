#include "jobs.h"
#include "lauxlib.h"
#include <abi/clock.h>
#include <abi/endpoint.h>
#include <abi/jit.h>
#include <abi/memory.h>
#include <abi/net_config.h>
#include <abi/pipe.h>
#include <abi/random.h>
#include <abi/system_info.h>
#include <abi/tcp.h>
#include <abi/thread.h>
#include <abi/udp.h>
#include <clock.h>
#include <program.h>
#include <bundle_launch.h>
#include <directory.h>
#include <file.h>
#include <handle.h>
#include <pyxis/pipeline.h>
#include <startup.h>
#include <environment.h>
#include <stdlib.h>
#include <string.h>

#define JOB_METATABLE "pyxis.job"
#define PREPARATION_METATABLE "pyxis.job.preparation"
#define NS_PER_MS UINT64_C(1000000)

static char registry_key;

struct owned_job {
  struct pyxis_pipeline *pipeline;
};

struct selected_root {
  const char *name;
  handle_t handle;
};

struct preparation {
  struct pyxis_job_options stages[LAUNCH_BATCH_MAX];
  struct command_selection *selections[LAUNCH_BATCH_MAX];
  struct bundle_launch *launches[LAUNCH_BATCH_MAX];
  struct command_search_capture search;
  struct bundle_authority *authority;
  size_t authority_count, authority_owned_count;
  const char **argv[LAUNCH_BATCH_MAX];
  struct selected_root *roots;
  size_t root_count;
  struct launch_environment_entry *entries;
  struct launch_grant *grants;
  size_t grant_count, entry_count;
  bool explicit_resources, tcp_selected;
  struct environment_capture parent;
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
  {"tcp", PROTOCOL_TCP_SERVICE, TCP_SERVICE_RIGHT_CONNECT},
  {"udp", PROTOCOL_UDP_SERVICE, UDP_SERVICE_RIGHT_OPEN},
  {"dns_udp", PROTOCOL_UDP_SERVICE, UDP_SERVICE_RIGHT_OPEN},
  {"net_registry", PROTOCOL_NET_REGISTRY, NET_REGISTRY_RIGHT_READ},
  {"pipe", PROTOCOL_PIPE_SERVICE, PIPE_SERVICE_RIGHT_CREATE},
  {"system_info", PROTOCOL_SYSTEM_INFO, SYSTEM_INFO_RIGHT_READ},
  {"service", PROTOCOL_ENDPOINT_SERVICE, ENDPOINT_SERVICE_RIGHT_CREATE},
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
    bundle_launch_close(owned->launches[i]);
    owned->launches[i] = NULL;
    command_selection_close(owned->selections[i]);
    owned->selections[i] = NULL;
  }
  command_search_capture_close(&owned->search);
  for (size_t i = 0; i < owned->authority_owned_count; ++i) {
    handle_close(owned->authority[i].source);
  }
  free(owned->authority);
  owned->authority = NULL;
  owned->authority_count = owned->authority_owned_count = 0;
  for (size_t i = 0; i < owned->root_count; ++i) {
    handle_close(owned->roots[i].handle);
  }
  free(owned->roots);
  owned->roots = NULL;
  owned->root_count = 0;
  free(owned->grants);
  owned->grants = NULL;
  free(owned->entries);
  owned->entries = NULL;
  owned->grant_count = owned->entry_count = 0;
  environment_capture_close(&owned->parent);
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

static enum call_status stage_capability(struct preparation *owned,
    const char *name, handle_t source, uint64_t rights, uint64_t transport)
{
  /* Selection is explicit: a text/capability collision refuses admission. */
  for (size_t i = 0; i < owned->entry_count; ++i) {
    if (!strcmp(name, (const char *)(uintptr_t)owned->entries[i].name)) {
      return CALL_BAD_REQUEST;
    }
  }
  if (owned->entry_count >= LAUNCH_CAPTURE_MAX_SIZE / sizeof(*owned->entries) ||
      owned->grant_count >= LAUNCH_CAPTURE_MAX_SIZE / sizeof(*owned->grants)) {
    return CALL_LIMIT;
  }
  struct launch_grant *grants = realloc(owned->grants,
      (owned->grant_count + 1) * sizeof(*grants));
  if (!grants) {
    return CALL_NO_MEMORY;
  }
  owned->grants = grants;
  struct launch_environment_entry *entries = realloc(owned->entries,
      (owned->entry_count + 1) * sizeof(*entries));
  if (!entries) {
    return CALL_NO_MEMORY;
  }
  owned->entries = entries;
  entries[owned->entry_count++] = (struct launch_environment_entry){
    (uintptr_t)name, ENVIRONMENT_CAP, ENVIRONMENT_EXPORTED, owned->grant_count, 0};
  grants[owned->grant_count++] = (struct launch_grant){source, rights, transport};
  return CALL_OK;
}

static enum call_status add_role(struct preparation *owned,
    const struct resource_role *role, bool required)
{
  const struct environment_value *value = environment_capture_find(&owned->parent, role->name);
  if (!value || value->kind != ENVIRONMENT_CAP) {
    return required ? CALL_UNAVAILABLE : CALL_OK;
  }
  struct handle_info info;
  enum call_status status = handle_query(value->handle, &info);
  if (status != CALL_OK) {
    return status;
  }
  if (info.protocol != role->protocol) {
    return CALL_WRONG_TYPE;
  }
  uint64_t rights = info.rights & role->rights;
  if (!rights) {
    /* A selected LISTEN-only source can still satisfy a bundle declaration;
     * raw stages receive no listener authority. */
    if (role->protocol == PROTOCOL_TCP_SERVICE && (info.rights & TCP_SERVICE_RIGHT_LISTEN)) {
      return CALL_OK;
    }
    return required ? CALL_DENIED : CALL_OK;
  }
  uint64_t transport = info.kind == HANDLE_KIND_EXPORTED ? HANDLE_TRANSPORT_CALL : 0;
  status = stage_capability(owned, role->name, value->handle, rights, transport);
  if (status == CALL_OK && role->protocol == PROTOCOL_ENDPOINT_SERVICE) {
    owned->entries[owned->entry_count - 1].flags = 0;
  }
  return status;
}

static enum call_status select_resources(lua_State *state, int options,
    struct preparation *owned)
{
  field(state, options, "resources");
  owned->explicit_resources = !lua_isnil(state, -1);
  enum call_status status = CALL_OK;
  if (!owned->explicit_resources) {
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
      owned->tcp_selected |= roles[role].protocol == PROTOCOL_TCP_SERVICE;
      if (status == CALL_OK) {
        status = add_role(owned, &roles[role], true);
      }
      lua_pop(state, 1);
    }
  }
  lua_pop(state, 1);
  return status;
}

static handle_t root_source(const struct preparation *owned, const char *name)
{
  const struct environment_value *value = environment_capture_find(&owned->parent, name);
  return value && value->kind == ENVIRONMENT_CAP ? value->handle : HANDLE_INVALID;
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
  owned->roots[i] = (struct selected_root){name, copy};
  return stage_capability(owned, name, copy, info.rights & mask, 0);
}

static enum call_status select_roots(lua_State *state, int options,
    struct preparation *owned, bool *explicit)
{
  field(state, options, "roots");
  *explicit = !lua_isnil(state, -1);
  size_t count = *explicit ? dense_array(state, -1,
      LAUNCH_CAPTURE_MAX_SIZE / sizeof(struct launch_environment_entry), false) : owned->parent.count;
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
      const struct environment_value *value = &owned->parent.values[i];
      struct handle_info info;
      if (!(value->flags & ENVIRONMENT_EXPORTED) || value->kind != ENVIRONMENT_CAP ||
          !strncmp(value->name, BUNDLE_SOURCE_PREFIX, sizeof(BUNDLE_SOURCE_PREFIX) - 1) ||
          !strcmp(value->name, "job_work") || !strcmp(value->name, "job_provisioner") ||
          !strcmp(value->name, "staging")) {
        continue;
      }
      status = handle_query(value->handle, &info);
      if (status != CALL_OK) {
        break;
      }
      if (info.protocol != PROTOCOL_DIRECTORY) {
        continue;
      }
      name = value->name;
    }
    if (status == CALL_OK) {
      status = add_root(owned, name, root_source(owned, name), read_only);
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
    if (status == CALL_OK && resolved.path) {
      const char *separator = strstr(resolved.path, "://");
      handle_t root = HANDLE_INVALID;
      if (separator) {
        size_t length = (size_t)(separator - resolved.path);
        for (size_t i = 0; i < owned->root_count; ++i) {
          if (strlen(owned->roots[i].name) == length &&
              !memcmp(owned->roots[i].name, resolved.path, length)) {
            root = owned->roots[i].handle;
            break;
          }
        }
      }
      if (!root) {
        status = CALL_NOT_FOUND;
      } else {
        handle_t *storage = malloc(sizeof(*storage));
        if (!storage) {
          status = CALL_NO_MEMORY;
        } else {
          status = path_context_init(&owned->working.context, storage, 1, &root, 1);
          if (status != CALL_OK) {
            free(storage);
          } else if (separator[3]) {
            status = change_working(&owned->working, separator + 3);
          }
        }
        if (status == CALL_OK) {
          owned->working.path = resolved.path;
          resolved.path = NULL;
        }
      }
    }
    pyxis_working_snapshot_close(&resolved);
  }
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
    if (++count > LAUNCH_CAPTURE_MAX_SIZE / sizeof(struct launch_environment_entry)) {
      luaL_error(state, "pyxis.job: environment exceeds native limits");
    }
    lua_pop(state, 1);
  }
  struct environment_capture *capture = &owned->environment.capture;
  capture->values = count ? calloc(count, sizeof(*capture->values)) : NULL;
  if (count && !capture->values) {
    lua_pop(state, 1);
    return CALL_NO_MEMORY;
  }
  enum call_status status = CALL_OK;
  lua_pushnil(state);
  while (lua_next(state, table)) {
    struct environment_value *variable = &capture->values[capture->count++];
    variable->kind = ENVIRONMENT_TEXT;
    variable->flags = ENVIRONMENT_EXPORTED;
    if (!environment_name(variable->name, text(state, -2))) {
      status = CALL_BAD_REQUEST;
    }
    variable->text = strdup(text(state, -1));
    if (!variable->text) {
      status = CALL_NO_MEMORY;
    } else {
      variable->length = strlen(variable->text);
    }
    lua_pop(state, 1);
  }
  lua_pop(state, 1);
  return status;
}

/* Resolve with the captured caller search, independently of the leaf's roots. */
static enum call_status native_program(const struct path_context *context,
    const struct command_search_capture *search, const char *command,
    struct command_selection **selection)
{
  size_t length = strlen(command);
  if (!length || length > SIZE_MAX - PROGRAM_LOOKUP_EXTRA) {
    return CALL_BAD_REQUEST;
  }
  size_t capacity = length + PROGRAM_LOOKUP_EXTRA;
  if (context->count > SIZE_MAX - capacity ||
      context->count + capacity > SIZE_MAX / sizeof(handle_t)) {
    return CALL_LIMIT;
  }
  size_t slots = context->count + capacity;
  struct path_workspace workspace = {
    .directories = malloc(slots * sizeof(handle_t)), .directory_capacity = slots,
    .component = malloc(capacity), .component_capacity = capacity,
  };
  enum call_status status = workspace.directories && workspace.component ?
      program_open_search(context, search, command, &workspace, selection) : CALL_NO_MEMORY;
  free(workspace.directories);
  free(workspace.component);
  return status;
}

static enum call_status bundle_authority(struct preparation *owned)
{
  size_t capacity = owned->entry_count + 3 + BUNDLE_SOURCE_LIMIT;
  owned->authority = calloc(capacity, sizeof(*owned->authority));
  if (!owned->authority) {
    return CALL_NO_MEMORY;
  }
  for (size_t i = 0; i < owned->entry_count; ++i) {
    const struct launch_environment_entry *entry = &owned->entries[i];
    if (entry->kind != ENVIRONMENT_CAP ||
        !strcmp((const char *)(uintptr_t)entry->name, "tcp")) {
      continue;
    }
    const struct launch_grant *grant = &owned->grants[entry->value];
    struct handle_info info;
    enum call_status status = handle_query(grant->source, &info);
    if (status != CALL_OK) {
      return status;
    }
    handle_t source;
    status = handle_copy_restricted(grant->source, grant->rights, grant->transport, &source);
    if (status != CALL_OK) {
      return status;
    }
    owned->authority[owned->authority_count++] = (struct bundle_authority){
      (const char *)(uintptr_t)entry->name, source, info.protocol};
    ++owned->authority_owned_count;
  }
  if (!owned->explicit_resources || owned->tcp_selected) {
    const struct environment_value *tcp = environment_capture_find(&owned->parent, "tcp");
    if (tcp && tcp->kind == ENVIRONMENT_CAP) {
      struct handle_info info;
      enum call_status status = handle_query(tcp->handle, &info);
      if (status != CALL_OK) {
        return status;
      }
      owned->authority[owned->authority_count++] = (struct bundle_authority){
        "tcp", tcp->handle, info.protocol};
    }
  }
  if (!owned->explicit_resources) {
    const struct environment_value *service = environment_capture_find(&owned->parent, "service");
    if (service && service->kind == ENVIRONMENT_CAP) {
      owned->authority[owned->authority_count++] = (struct bundle_authority){
        "service", service->handle, PROTOCOL_ENDPOINT_SERVICE};
    }
  }
  const struct environment_value *jit = environment_capture_find(&owned->parent, "jit");
  if (jit && jit->kind == ENVIRONMENT_CAP) {
    struct handle_info info;
    enum call_status status = handle_query(jit->handle, &info);
    if (status != CALL_OK) {
      return status;
    }
    owned->authority[owned->authority_count++] = (struct bundle_authority){
      "jit", jit->handle, info.protocol};
  }
  size_t sources;
  enum call_status status = bundle_sources_from_capture(&owned->parent,
      owned->authority + owned->authority_count, BUNDLE_SOURCE_LIMIT, &sources);
  if (status == CALL_OK) {
    owned->authority_count += sources;
  }
  return status;
}

static enum call_status prepare_bundle(struct preparation *owned, size_t index)
{
  if (!owned->authority) {
    enum call_status status = bundle_authority(owned);
    if (status != CALL_OK) {
      return status;
    }
  }
  struct pyxis_job_options *stage = &owned->stages[index];
  struct launch_request source = {
    .image = stage->image, .argv = (uintptr_t)stage->argv, .argc = stage->argc,
    .grants = (uintptr_t)stage->grants, .grant_count = stage->grant_count,
    .entries = (uintptr_t)stage->entries, .entry_count = stage->entry_count,
  };
  enum call_status status = bundle_launch_prepare(owned->selections[index]->bundle,
      &source, owned->authority, owned->authority_count, &owned->launches[index]);
  if (status != CALL_OK) {
    return status;
  }
  const struct launch_request *request = bundle_launch_request(owned->launches[index]);
  stage->image = request->image;
  stage->grants = (const void *)(uintptr_t)request->grants;
  stage->grant_count = request->grant_count;
  stage->entries = (const void *)(uintptr_t)request->entries;
  stage->entry_count = request->entry_count;
  stage->initial_stack_bytes = request->initial_stack_bytes;
  return CALL_OK;
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
  struct preparation *owned = lua_newuserdatauv(state, sizeof(*owned), 0);
  *owned = (struct preparation){0};
  luaL_setmetatable(state, PREPARATION_METATABLE);
  lua_toclose(state, lua_gettop(state));
  enum call_status status = environment_capture_init(startup_environment_handle(),
      ENVIRONMENT_SNAPSHOT_INCLUDE_PRIVATE, &owned->parent);
  handle_t launcher = root_source(owned, "launcher");
  handle_t clock = root_source(owned, "clock");
  handle_t pipe = root_source(owned, "pipe");
  handle_t private_root = root_source(owned, "job_work");
  uint64_t now, deadline = 0;
  if (status == CALL_OK && (!launcher || !clock || !pipe || !private_root)) {
    status = CALL_UNAVAILABLE;
  }
  if (status == CALL_OK) {
    status = clock_now(clock, &now);
    if (status == CALL_OK && now > UINT64_MAX - timeout * NS_PER_MS) {
      status = CALL_LIMIT;
    }
    if (status == CALL_OK) {
      deadline = now + timeout * NS_PER_MS;
    }
  }
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
  field(state, 2, "providers");
  if (!lua_isnil(state, -1)) {
    luaL_checktype(state, -1, LUA_TBOOLEAN);
  }
  bool providers = lua_toboolean(state, -1);
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
    status = select_roots(state, 2, owned, &explicit_roots);
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
  if (status == CALL_OK && providers) {
    for (size_t i = 0; i < owned->parent.count && status == CALL_OK; ++i) {
      const struct environment_value *value = &owned->parent.values[i];
      if (value->kind != ENVIRONMENT_CAP || !(value->flags & ENVIRONMENT_EXPORTED) ||
          !strcmp(value->name, "stdin") || !strcmp(value->name, "stdout") ||
          !strcmp(value->name, "stderr") ||
          !strncmp(value->name, BUNDLE_SOURCE_PREFIX, sizeof(BUNDLE_SOURCE_PREFIX) - 1)) {
        continue;
      }
      struct handle_info info;
      status = handle_query(value->handle, &info);
      if (status == CALL_OK && info.protocol == PROTOCOL_JIT) {
        continue;
      }
      if (status == CALL_OK && info.kind == HANDLE_KIND_EXPORTED) {
        bool selected = false;
        for (size_t j = 0; j < owned->entry_count; ++j) {
          selected |= !strcmp(value->name, (const char *)(uintptr_t)owned->entries[j].name);
        }
        if (!selected) {
          status = stage_capability(owned, value->name, value->handle, info.rights, info.transport);
        }
      }
    }
  }
  if (status == CALL_OK) {
    size_t extra = owned->environment.capture.count;
    struct launch_environment_entry *entries = realloc(owned->entries,
        (owned->entry_count + extra) * sizeof(*entries));
    if (extra && !entries) {
      status = CALL_NO_MEMORY;
    } else {
      if (entries) {
        owned->entries = entries;
      }
      for (size_t i = 0; i < extra; ++i) {
        const struct environment_value *value = &owned->environment.capture.values[i];
        for (size_t j = 0; j < owned->entry_count; ++j) {
          if (!strcmp(value->name, (const char *)(uintptr_t)owned->entries[j].name)) {
            status = CALL_BAD_REQUEST;
            break;
          }
        }
        if (status != CALL_OK) {
          break;
        }
        owned->entries[owned->entry_count++] = (struct launch_environment_entry){
          (uintptr_t)value->name, ENVIRONMENT_TEXT, value->flags, (uintptr_t)value->text, value->length};
      }
    }
  }
  bool bare = false;
  for (size_t i = 0; i < count; ++i) {
    bare |= !program_is_explicit(owned->stages[i].argv[0]);
  }
  if (status == CALL_OK && bare) {
    const struct environment_value *path = environment_capture_find(&owned->parent, "path");
    if (path) {
      status = path->kind == ENVIRONMENT_CAP ?
          command_search_capture_list(path->handle, &owned->search) : CALL_WRONG_TYPE;
    }
  }
  for (size_t i = 0; i < count && status == CALL_OK; ++i) {
    struct pyxis_job_options *stage = &owned->stages[i];
    /* Image selection uses held caller authority; only the final child
     * inventory uses the selected content roots and independent cwd. */
    status = native_program(current, &owned->search, stage->argv[0], &owned->selections[i]);
    if (status == CALL_OK) {
      stage->image = owned->selections[i]->image;
    }
    stage->grants = owned->grants;
    stage->grant_count = owned->grant_count;
    stage->entries = owned->entries;
    stage->entry_count = owned->entry_count;
    stage->working = &owned->working;
    if (status == CALL_OK && owned->selections[i]->bundle) {
      status = prepare_bundle(owned, i);
    }
    for (size_t stream = 0; stream < ENVIRONMENT_STREAM_COUNT; ++stream) {
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
  enum environment_stream_index stream;
  if (!strcmp(name, "stdout")) {
    stream = ENVIRONMENT_STDOUT;
  } else if (!strcmp(name, "stderr")) {
    stream = ENVIRONMENT_STDERR;
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
  lua_pushinteger(state, PROCESS_START_FAILED);
  lua_setfield(state, module, "PROCESS_START_FAILED");
}
