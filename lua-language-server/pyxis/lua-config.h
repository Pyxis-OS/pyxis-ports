#ifndef PYXIS_LUALS_LUA_CONFIG_H
#define PYXIS_LUALS_LUA_CONFIG_H

#define l_signalT int
#define lua_getlocaledecpoint() '.'
#define LUA_FILE_BUFFER_SIZE 512
#define l_gmtime(t, r) gmtime_r(t, r)
#define l_localtime(t, r) localtime_r(t, r)
#define LUA_LSUBSEP "/"
#define LUA_PATH_DEFAULT ""
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
