#include <abi/file.h>
#include <directory.h>
#include <handle.h>
#include <startup.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomgeneric_pyxis.h"
#include "m_misc.h"

static const char *save_iwad;
static char *save_directory;

char *DG_SaveGameDir(const char *iwad_name)
{
  save_iwad = iwad_name;
  save_directory = M_StringJoin("home://doom/saves/", iwad_name, "/", NULL);
  printf("Doom saves: %s\n", save_directory);
  return save_directory;
}

static enum call_status open_save_directory(handle_t *directory)
{
  const char *names[] = {"doom", "saves", save_iwad};
  uint64_t rights = DIRECTORY_RIGHT_LOOKUP | DIRECTORY_RIGHT_CREATE |
    DIRECTORY_RIGHT_READ_FILES | DIRECTORY_RIGHT_WRITE_FILES | DIRECTORY_RIGHT_REMOVE;
  handle_t parent = startup_root("home");
  bool owned = false;
  enum call_status status;

  *directory = HANDLE_INVALID;
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
    handle_t child;
    status = directory_lookup(parent, names[i], DIRECTORY_KIND_DIRECTORY, rights, &child);
    if (status == CALL_NOT_FOUND) {
      status = directory_create(parent, names[i], DIRECTORY_KIND_DIRECTORY, rights, &child);
      if (status == CALL_ALREADY_EXISTS) {
        status = directory_lookup(parent, names[i], DIRECTORY_KIND_DIRECTORY, rights, &child);
      }
    }
    if (owned) {
      handle_close(parent);
    }
    if (status != CALL_OK) {
      return status;
    }
    parent = child;
    owned = true;
  }
  *directory = parent;
  return CALL_OK;
}

char *DG_TempSaveGameFile(void)
{
  handle_t directory;
  enum call_status status = open_save_directory(&directory);
  if (status != CALL_OK) {
    fprintf(stderr, "Doom: open save directory failed (status %u)\n", status);
    return NULL;
  }

  /* Exclusive creation keeps simultaneous saves from sharing temp.dsg. Names
   * left by an interrupted process are skipped; no clock or process ID needed. */
  static uint64_t next_temp;
  uint64_t first = next_temp;
  char name[40];
  size_t capacity = strlen(save_directory) + sizeof(name);
  char *path = malloc(capacity);
  if (!path) {
    handle_close(directory);
    fprintf(stderr, "Doom: no memory for temporary save path\n");
    return NULL;
  }
  handle_t file;
  do {
    snprintf(name, sizeof(name), "temp-%llu.dsg", (unsigned long long)next_temp++);
    status = directory_create(directory, name, DIRECTORY_KIND_FILE, FILE_RIGHT_WRITE, &file);
  } while (status == CALL_ALREADY_EXISTS && next_temp != first);

  if (status != CALL_OK) {
    fprintf(stderr, "Doom: reserve temporary save failed (status %u)\n", status);
    handle_close(directory);
    free(path);
    return NULL;
  }
  handle_close(file);

  snprintf(path, capacity, "%s%s", save_directory, name);
  handle_close(directory);
  return path;
}
