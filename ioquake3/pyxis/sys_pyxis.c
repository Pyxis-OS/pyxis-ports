/* ioquake3's native Pyxis platform adapter. SPDX-License-Identifier: GPL-2.0-or-later */

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "sys_local.h"

#include <SDL.h>
#include <dirent.h>
#include <errno.h>
#include <fenv.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_FOUND_FILES 4096

static char home_path[MAX_OSPATH];

static char *home_directory(void)
{
  if (!home_path[0]) {
    char *path = SDL_GetPrefPath(NULL, "ioquake3");
    if (!path)
      Sys_Error("Cannot open ioquake3 preferences: %s", SDL_GetError());
    if (strlen(path) >= sizeof(home_path)) {
      SDL_free(path);
      Sys_Error("ioquake3 preference path is too long");
    }
    Q_strncpyz(home_path, path, sizeof(home_path));
    SDL_free(path);
    size_t length = strlen(home_path);
    if (length && home_path[length - 1] == '/')
      home_path[length - 1] = '\0';
  }
  return home_path;
}

char *Sys_DefaultHomeConfigPath(void)
{
  return home_directory();
}

char *Sys_DefaultHomeDataPath(void)
{
  return home_directory();
}

char *Sys_DefaultHomeStatePath(void)
{
  return home_directory();
}

char *Sys_SteamPath(void)
{
  return "";
}

char *Sys_GogPath(void)
{
  return "";
}

char *Sys_MicrosoftStorePath(void)
{
  return "";
}

int Sys_Milliseconds(void)
{
  static uint64_t origin;
  static qboolean initialized;
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    Sys_Error("Monotonic clock unavailable: %s", strerror(errno));
  uint64_t milliseconds = (uint64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
  if (!initialized) {
    origin = milliseconds;
    initialized = qtrue;
  }
  return (int)(uint32_t)(milliseconds - origin);
}

qboolean Sys_RandomBytes(byte *bytes, int length)
{
  /* No entropy authority is declared. The offline engine's explicit weak
   * fallback is used; this must be revisited before external networking. */
  (void)bytes;
  (void)length;
  return qfalse;
}

char *Sys_GetCurrentUser(void)
{
  /* An initial player name, not an assertion about a native identity. */
  return "player";
}

qboolean Sys_LowPhysicalMemory(void)
{
  /* This advisory platform hint is unused by the current engine. */
  return qfalse;
}

const char *Sys_Basename(char *path)
{
  return basename(path);
}

const char *Sys_Dirname(char *path)
{
  return dirname(path);
}

FILE *Sys_FOpen(const char *path, const char *mode)
{
  struct stat status;
  if (stat(path, &status) == 0 && S_ISDIR(status.st_mode)) {
    errno = EISDIR;
    return NULL;
  }
  return fopen(path, mode);
}

qboolean Sys_Mkdir(const char *path)
{
  if (mkdir(path, 0) == 0)
    return qtrue;
  if (errno == EEXIST) {
    struct stat status;
    return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
  }
  return qfalse;
}

FILE *Sys_Mkfifo(const char *path)
{
  (void)path;
  errno = ENOTSUP;
  return NULL;
}

char *Sys_Cwd(void)
{
  static char cwd[MAX_OSPATH];
  return getcwd(cwd, sizeof(cwd));
}

char *Sys_BinaryPathRelative(const char *relative)
{
  /* The bundle's executable path is not a directory capability. */
  (void)relative;
  errno = ENOTSUP;
  return NULL;
}

static qboolean join_path(char *output, size_t size, const char *base,
                          const char *name)
{
  int length = snprintf(output, size, "%s%s%s", base,
                        *base && base[strlen(base) - 1] != '/' ? "/" : "", name);
  return length >= 0 && (size_t)length < size;
}

static void list_filtered_files(const char *base, const char *subdir,
                                char *filter, char **list, int *count)
{
  char search[MAX_OSPATH];
  if (*count >= MAX_FOUND_FILES - 1 || !*base ||
      !join_path(search, sizeof(search), base, subdir))
    return;
  DIR *directory = opendir(search);
  if (!directory)
    return;
  struct dirent *entry;
  while (*count < MAX_FOUND_FILES - 1 && (entry = readdir(directory))) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;
    char path[MAX_OSPATH], relative[MAX_OSPATH];
    struct stat status;
    if (!join_path(path, sizeof(path), search, entry->d_name) ||
        !join_path(relative, sizeof(relative), subdir, entry->d_name) ||
        stat(path, &status) != 0)
      continue;
    if (S_ISDIR(status.st_mode))
      list_filtered_files(base, relative, filter, list, count);
    if (*count < MAX_FOUND_FILES - 1 && Com_FilterPath(filter, relative, qfalse))
      list[(*count)++] = CopyString(relative);
  }
  closedir(directory);
}

char **Sys_ListFiles(const char *path, const char *extension, char *filter,
                     int *numfiles, qboolean wantsubs)
{
  char *files[MAX_FOUND_FILES];
  int count = 0;
  *numfiles = 0;
  if (filter) {
    list_filtered_files(path, "", filter, files, &count);
  } else {
    if (!*path)
      return NULL;
    DIR *directory = opendir(path);
    if (!directory)
      return NULL;
    if (!extension)
      extension = "";
    if (!strcmp(extension, "/")) {
      extension = "";
      wantsubs = qtrue;
    }
    size_t extension_length = strlen(extension);
    struct dirent *entry;
    while (count < MAX_FOUND_FILES - 1 && (entry = readdir(directory))) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      char fullpath[MAX_OSPATH];
      struct stat status;
      if (!join_path(fullpath, sizeof(fullpath), path, entry->d_name) ||
          stat(fullpath, &status) != 0 ||
          (wantsubs != (S_ISDIR(status.st_mode) ? qtrue : qfalse)))
        continue;
      size_t name_length = strlen(entry->d_name);
      if (name_length < extension_length ||
          Q_stricmp(entry->d_name + name_length - extension_length, extension))
        continue;
      files[count++] = CopyString(entry->d_name);
    }
    closedir(directory);
  }
  *numfiles = count;
  if (!count)
    return NULL;
  char **list = Z_Malloc((count + 1) * sizeof(*list));
  memcpy(list, files, count * sizeof(*list));
  list[count] = NULL;
  return list;
}

void Sys_FreeFileList(char **list)
{
  if (!list)
    return;
  for (int i = 0; list[i]; i++)
    Z_Free(list[i]);
  Z_Free(list);
}

void Sys_Sleep(int milliseconds)
{
  if (!milliseconds)
    return;
  /* The passive console has no waitable input. Match upstream's bounded
   * wait for requests that would otherwise block indefinitely. */
  if (milliseconds < 0)
    milliseconds = 10;
  struct timespec delay = { milliseconds / 1000,
                            (milliseconds % 1000) * 1000000L };
  if (nanosleep(&delay, NULL) != 0)
    Sys_Error("Clock sleep failed: %s", strerror(errno));
}

void Sys_ErrorDialog(const char *error)
{
  /* stderr remains usable even before the home path or filesystem initializes. */
  Sys_Print(error);
  Sys_Print("\n");
}

dialogResult_t Sys_Dialog(dialogType_t type, const char *message, const char *title)
{
  Sys_Print(va("%s: %s\n", title, message));
  if (type == DT_YES_NO)
    return DR_NO;
  if (type == DT_OK_CANCEL)
    return DR_CANCEL;
  return DR_OK;
}

void Sys_GLimpSafeInit(void)
{
}

void Sys_GLimpInit(void)
{
}

void Sys_PlatformInit(void)
{
  if (fesetround(FE_TONEAREST) != 0)
    Sys_Error("Cannot set floating point rounding mode");
}

void Sys_PlatformExit(void)
{
}

void Sys_SetEnv(const char *name, const char *value)
{
  int result = value && *value ? setenv(name, value, 1) : unsetenv(name);
  if (result != 0)
    Com_Printf("Cannot change environment variable %s: %s\n", name, strerror(errno));
}

qboolean Sys_DllExtension(const char *name)
{
  (void)name;
  return qfalse;
}

qboolean Sys_OpenFolderInPlatformFileManager(const char *path)
{
  Com_Printf("Opening folders is unavailable on Pyxis: %s\n", path);
  return qfalse;
}

qboolean Sys_SetMaxFileLimit(void)
{
  return qfalse;
}
