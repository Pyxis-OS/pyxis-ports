#ifndef PYXIS_LUA_NATIVE_H
#define PYXIS_LUA_NATIVE_H

#include "lua.h"

int luaopen_pyxis(lua_State *state);
/* Call after lua_close so all per-call hash operations have been released. */
void pyxis_finish(void);

#endif
