#include <stdio.h>
#include <startup.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

struct invocation {
  int argc;
  char **argv;
  int script_index;
  const char *expression;
};

static int traceback(lua_State *state)
{
  const char *message = lua_tostring(state, 1);
  if (message == NULL) {
    if (luaL_callmeta(state, 1, "__tostring") &&
        lua_type(state, -1) == LUA_TSTRING) {
      return 1;
    }
    message = lua_pushfstring(state, "(error object is a %s value)",
        luaL_typename(state, 1));
  }
  luaL_traceback(state, state, message, 1);
  return 1;
}

static void open_libraries(lua_State *state)
{
  luaL_requiref(state, LUA_GNAME, luaopen_base, 1);
  lua_pop(state, 1);
  luaL_requiref(state, LUA_COLIBNAME, luaopen_coroutine, 1);
  lua_pop(state, 1);
  luaL_requiref(state, LUA_TABLIBNAME, luaopen_table, 1);
  lua_pop(state, 1);
  luaL_requiref(state, LUA_STRLIBNAME, luaopen_string, 1);
  lua_pop(state, 1);
  luaL_requiref(state, LUA_UTF8LIBNAME, luaopen_utf8, 1);
  lua_pop(state, 1);
}

static void set_arguments(lua_State *state, const struct invocation *invocation)
{
  int script = invocation->script_index;
  lua_createtable(state, invocation->argc - script - 1, script + 1);
  for (int i = 0; i < invocation->argc; ++i) {
    lua_pushstring(state, invocation->argv[i]);
    lua_rawseti(state, -2, i - script);
  }
  lua_setglobal(state, "arg");
}

static int run_program(lua_State *state)
{
  const struct invocation *invocation = lua_touserdata(state, 1);
  open_libraries(state);
  set_arguments(state, invocation);
  lua_settop(state, 0);

  lua_pushcfunction(state, traceback);
  int status, arguments = 0;
  if (invocation->expression != NULL) {
    status = luaL_loadbufferx(state, invocation->expression,
        strlen(invocation->expression), "=(command line)", "t");
  } else {
    status = luaL_loadfilex(state, invocation->argv[invocation->script_index], "bt");
    if (status == LUA_OK) {
      arguments = invocation->argc - invocation->script_index - 1;
      luaL_checkstack(state, arguments, "too many script arguments");
      for (int i = invocation->script_index + 1; i < invocation->argc; ++i) {
        lua_pushstring(state, invocation->argv[i]);
      }
    }
  }
  if (status == LUA_OK) {
    status = lua_pcall(state, arguments, 0, 1);
  }
  if (status != LUA_OK) {
    return lua_error(state);
  }
  return 0;
}

int main(int argc, char **argv)
{
  struct invocation invocation = {.argc = argc, .argv = argv};
  if (argc == 3 && strcmp(argv[1], "-e") == 0) {
    invocation.expression = argv[2];
  } else if (argc >= 3 && strcmp(argv[1], "--") == 0) {
    invocation.script_index = 2;
  } else if (argc >= 2 && argv[1][0] != '-') {
    invocation.script_index = 1;
  } else {
    fprintf(stderr, "Usage: lua -e 'code' | lua [--] file.lua [args...]\n");
    return EXIT_FAILURE;
  }

  /* Shebang launches hand off an open file object, not a path to reopen. */
  if (startup_resource("script") != HANDLE_INVALID) {
    fprintf(stderr, "lua: script handoff is not supported; use lua file.lua\n");
    return EXIT_FAILURE;
  }

  lua_State *state = luaL_newstate();
  if (state == NULL) {
    fprintf(stderr, "lua: cannot allocate interpreter state\n");
    return EXIT_FAILURE;
  }

  /* Library initialization can allocate and throw, so protect it as well as
   * compilation/execution. The borrowed arguments live through this call. */
  lua_pushcfunction(state, run_program);
  lua_pushlightuserdata(state, &invocation);
  int status = lua_pcall(state, 1, 0, 0);
  if (status != LUA_OK) {
    const char *message = lua_type(state, -1) == LUA_TSTRING ?
        lua_tostring(state, -1) : "error object is not a string";
    fprintf(stderr, "lua: %s\n", message);
  }
  lua_close(state);
  return status == LUA_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
