#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>

#include "doomgeneric_pyxis.h"
#include "m_misc.h"

char *DG_SaveGameDir(const char *iwad_name)
{
  char *directory = M_StringJoin("home://doom/saves/", iwad_name, "/", NULL);
  mkdir("home://doom", 0777);
  mkdir("home://doom/saves", 0777);
  mkdir(directory, 0777);
  return directory;
}
