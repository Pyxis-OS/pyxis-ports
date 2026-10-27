#ifndef PYXIS_LUA_CONFIG_H
#define PYXIS_LUA_CONFIG_H


#define l_gmtime(t, r) gmtime_r(t, r)
#define l_localtime(t, r) localtime_r(t, r)
#define LUA_LSUBSEP "/"
#define LUA_CPATH_DEFAULT ""
#define LUA_PATH_DEFAULT "?.lua;?/init.lua;boot://share/lua/?.lua;boot://share/lua/?/init.lua"

/* Like upstream's mkstemp adaptation, tmpname reserves a real empty file. */
#include <unistd.h>
#define lua_stdin_is_tty() isatty(0)
#define LUA_TMPNAMBUFSIZE sizeof("tmp://lua-XXXXXX")
#define lua_tmpnam(b, e) do { \
  strcpy(b, "tmp://lua-XXXXXX"); \
  int descriptor = mkstemp(b); \
  e = descriptor < 0; \
  if (!e) e = close(descriptor) != 0; \
} while (0)

#endif
