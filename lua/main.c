#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

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

static int run_expression(lua_State *state)
{
  const char *source = lua_touserdata(state, 1);
  open_libraries(state);
  lua_settop(state, 0);

  lua_pushcfunction(state, traceback);
  int status = luaL_loadbufferx(state, source, strlen(source),
      "=(command line)", "t");
  if (status == LUA_OK) {
    status = lua_pcall(state, 0, 0, 1);
  }
  if (status != LUA_OK) {
    return lua_error(state);
  }
  return 0;
}

int main(int argc, char **argv)
{
  if (argc != 3 || strcmp(argv[1], "-e") != 0) {
    fprintf(stderr, "Usage: lua -e 'code'\n");
    return EXIT_FAILURE;
  }

  lua_State *state = luaL_newstate();
  if (state == NULL) {
    fprintf(stderr, "lua: cannot allocate interpreter state\n");
    return EXIT_FAILURE;
  }

  /* Library initialization can allocate and throw, so protect it as well as
   * compilation/execution. The borrowed argument lives through this call. */
  lua_pushcfunction(state, run_expression);
  lua_pushlightuserdata(state, argv[2]);
  int status = lua_pcall(state, 1, 0, 0);
  if (status != LUA_OK) {
    const char *message = lua_type(state, -1) == LUA_TSTRING ?
        lua_tostring(state, -1) : "error object is not a string";
    fprintf(stderr, "lua: %s\n", message);
  }
  lua_close(state);
  return status == LUA_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
