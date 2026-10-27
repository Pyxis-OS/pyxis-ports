#ifndef PYXIS_LUALS_LUA_CONFIG_H
#define PYXIS_LUALS_LUA_CONFIG_H

#define l_gmtime(t, r) gmtime_r(t, r)
#define l_localtime(t, r) localtime_r(t, r)
#define LUA_LSUBSEP "/"
#define LUA_PATH_DEFAULT "app://share/lua-language-server/script/?.lua;app://share/lua-language-server/script/?/init.lua"
#define LUA_CPATH_DEFAULT ""

#include <unistd.h>
#define LUA_TMPNAMBUFSIZE sizeof("tmp://luals-XXXXXX")
#define lua_tmpnam(b, e) do { \
  strcpy(b, "tmp://luals-XXXXXX"); \
  int descriptor = mkstemp(b); \
  e = descriptor < 0; \
  if (!e) e = close(descriptor) != 0; \
} while (0)

#endif
