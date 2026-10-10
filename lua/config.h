#ifndef PYXIS_LUA_CONFIG_H
#define PYXIS_LUA_CONFIG_H

/* Pyxis has no signals or locale state; strcoll compares in byte order.
 * Keep Lua's normal numeric types. */
#define l_signalT int
#define lua_getlocaledecpoint() '.'

/* Lua's loader buffer is independent of libc input read-ahead. */
#define LUA_FILE_BUFFER_SIZE 512

#define l_gmtime(t, r) gmtime_r(t, r)
#define l_localtime(t, r) localtime_r(t, r)
#define LUA_LSUBSEP "/"
#define LUA_PATH_DEFAULT "boot://share/lua/?.lua;boot://share/lua/?/init.lua"

/* Like upstream's mkstemp adaptation, tmpname reserves a real empty file. */
#include <unistd.h>
#define LUA_TMPNAMBUFSIZE sizeof("tmp://lua-XXXXXX")
#define lua_tmpnam(b, e) do { \
  strcpy(b, "tmp://lua-XXXXXX"); \
  int descriptor = mkstemp(b); \
  e = descriptor < 0; \
  if (!e) e = close(descriptor) != 0; \
} while (0)

#endif
