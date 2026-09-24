#ifndef PYXIS_LUA_CONFIG_H
#define PYXIS_LUA_CONFIG_H

/* Pyxis has no signals or locale state. Keep Lua's normal numeric types. */
#define l_signalT int
#define lua_getlocaledecpoint() '.'
#define l_strcoll strcmp

#endif
