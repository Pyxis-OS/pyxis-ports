#ifndef PYXIS_LUA_NATIVE_H
#define PYXIS_LUA_NATIVE_H

#include "lua.h"

int luaopen_pyxis(lua_State *state);
int lua_pyxis_loadscript(lua_State *state, const char *filename);

#endif
