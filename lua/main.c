#include <stdio.h>
#include <startup.h>
#include <stdlib.h>
#include <string.h>
#include <term.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

struct invocation {
  int argc;
  char **argv;
  int script_index;
  const char *expression;
};

struct repl {
  struct terminal terminal;
  bool finished;
  bool failed;
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

static bool read_repl_line(lua_State *state, struct repl *repl, bool first)
{
  char line[1024];
  struct term_line_result read = term_read_line(&repl->terminal,
      first ? "> " : ">> ", line, sizeof(line));
  switch (read.status) {
  case TERM_LINE_EOF:
    repl->finished = true;
    return false;
  case TERM_LINE_CANCELLED:
    return false;
  case TERM_LINE_INPUT_LOST:
    luaL_error(state, "input lost; pending chunk discarded");
    return false;
  case TERM_LINE_ERROR:
    repl->failed = true;
    luaL_error(state, "terminal failure (status %d)", (int)read.error);
    return false;
  case TERM_LINE_OK:
    if (read.limit_reached) {
      luaL_error(state, "line limit reached; pending chunk discarded");
      return false;
    }
    lua_pushlstring(state, line, read.length);
    return true;
  }
  return false;
}

static bool incomplete_chunk(lua_State *state, int status)
{
  if (status != LUA_ERRSYNTAX) {
    return false;
  }
  /* The pinned Lua parser marks an unfinished statement at end-of-input.
   * Match its CLI rather than guessing completeness from braces or keywords. */
  const char marker[] = "<eof>";
  size_t length;
  const char *message = lua_tolstring(state, -1, &length);
  return length >= sizeof(marker) - 1 &&
      strcmp(message + length - (sizeof(marker) - 1), marker) == 0;
}

static int repl_chunk(lua_State *state)
{
  struct repl *repl = lua_touserdata(state, 1);
  lua_settop(state, 0);
  if (!read_repl_line(state, repl, true)) {
    return 0;
  }

  /* Like Lua's CLI, try a returned expression before a statement. Keep the
   * source on Lua's stack so allocation errors cannot leak native buffers. */
  const char *source = lua_tostring(state, 1);
  const char *expression = lua_pushfstring(state, "return %s;", source);
  int status = luaL_loadbufferx(state, expression, strlen(expression), "=stdin", "t");
  if (status == LUA_OK) {
    lua_remove(state, 2);
  } else {
    if (status != LUA_ERRSYNTAX) {
      repl->failed = true;
      return lua_error(state);
    }
    lua_pop(state, 2);
    for (;;) {
      size_t length;
      source = lua_tolstring(state, 1, &length);
      status = luaL_loadbufferx(state, source, length, "=stdin", "t");
      if (!incomplete_chunk(state, status)) {
        break;
      }
      lua_pop(state, 1);
      if (!read_repl_line(state, repl, false)) {
        return 0;
      }
      lua_pushliteral(state, "\n");
      lua_insert(state, 2);
      lua_concat(state, 3);
    }
  }
  if (status != LUA_OK) {
    /* Rethrowing a loader error changes its status to LUA_ERRRUN. */
    repl->failed = status == LUA_ERRMEM;
    return lua_error(state);
  }
  lua_remove(state, 1);
  lua_call(state, 0, LUA_MULTRET);

  int results = lua_gettop(state);
  if (results != 0) {
    luaL_checkstack(state, LUA_MINSTACK, "too many results to print");
    lua_getglobal(state, "print");
    lua_insert(state, 1);
    lua_call(state, results, 0);
  }
  return 0;
}

static int report_error(lua_State *state)
{
  /* Do not allocate while reporting a possibly exhausted Lua heap. */
  const char *message = lua_type(state, -1) == LUA_TSTRING ?
      lua_tostring(state, -1) : "error object is not a string";
  return fprintf(stderr, "lua: %s\n", message);
}

static int run_repl(lua_State *state)
{
  struct repl repl = {
    .terminal = {startup_resource("input"), startup_resource("output")},
  };
  lua_pushcfunction(state, traceback);
  /* Keep the handler below each protected iteration, including input assembly
   * and result printing. Each iteration releases its source and results. */
  while (!repl.finished) {
    lua_pushcfunction(state, repl_chunk);
    lua_pushlightuserdata(state, &repl);
    int status = lua_pcall(state, 1, 0, 1);
    if (status != LUA_OK) {
      if (repl.failed || status == LUA_ERRMEM || status == LUA_ERRERR) {
        return lua_error(state);
      }
      if (report_error(state) < 0) {
        return luaL_error(state, "cannot write error output");
      }
      lua_pop(state, 1);
    }
  }
  return 0;
}

static int run_program(lua_State *state)
{
  const struct invocation *invocation = lua_touserdata(state, 1);
  open_libraries(state);
  set_arguments(state, invocation);
  lua_settop(state, 0);

  if (invocation->argc == 1) {
    return run_repl(state);
  }
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
  } else if (argc != 1) {
    fprintf(stderr, "Usage: lua | lua -e 'code' | lua [--] file.lua [args...]\n");
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
    report_error(state);
  }
  lua_close(state);
  return status == LUA_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
