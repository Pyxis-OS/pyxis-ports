/* SDL paths on Pyxis. Preferences live in home://APP/; the organisation name
 * is not used. There is no reliable executable path, so SDL_GetBasePath is
 * unsupported and ports set their read-only paths explicitly. The current
 * directory and file operations come from upstream's POSIX code over libc. */

#include "SDL_internal.h"

#ifdef SDL_FILESYSTEM_PYXIS

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "filesystem/SDL_sysfilesystem.h"

#define HOME_ROOT "home://"

char *SDL_SYS_GetBasePath(void)
{
  SDL_Unsupported();
  return NULL;
}

char *SDL_SYS_GetPrefPath(const char *org, const char *app)
{
  (void)org;
  if (!app || !*app) {
    SDL_InvalidParamError("app");
    return NULL;
  }
  size_t length = SDL_strlen(HOME_ROOT) + SDL_strlen(app) + 2;
  char *path = SDL_malloc(length);
  if (!path) {
    return NULL;
  }
  SDL_snprintf(path, length, HOME_ROOT "%s", app);
  if (mkdir(path, 0700) != 0 && errno != EEXIST) {
    SDL_SetError("Couldn't create directory '%s': %s", path, strerror(errno));
    SDL_free(path);
    return NULL;
  }
  SDL_strlcat(path, "/", length);
  return path;
}

/* home:// is the only user folder Pyxis has. */
char *SDL_SYS_GetUserFolder(SDL_Folder folder)
{
  if (folder != SDL_FOLDER_HOME) {
    SDL_Unsupported();
    return NULL;
  }
  return SDL_strdup(HOME_ROOT);
}

#endif /* SDL_FILESYSTEM_PYXIS */
