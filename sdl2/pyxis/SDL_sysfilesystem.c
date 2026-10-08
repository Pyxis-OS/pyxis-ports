/* SDL paths on Pyxis. Preferences live in home://APP/; the organisation name
 * is not used. There is no reliable executable path, so SDL_GetBasePath is
 * unsupported and ports set their read-only paths explicitly. */

#include "SDL_internal.h"

#ifdef SDL_FILESYSTEM_PYXIS

#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "SDL_error.h"
#include "SDL_filesystem.h"

#define PREF_ROOT "home://"

char *SDL_GetBasePath(void)
{
  SDL_Unsupported();
  return NULL;
}

char *SDL_GetPrefPath(const char *org, const char *app)
{
  (void)org;
  if (!app || !*app) {
    SDL_InvalidParamError("app");
    return NULL;
  }

  size_t length = SDL_strlen(PREF_ROOT) + SDL_strlen(app) + 2;
  char *path = SDL_malloc(length);
  if (!path) {
    SDL_OutOfMemory();
    return NULL;
  }
  SDL_snprintf(path, length, PREF_ROOT "%s", app);
  if (mkdir(path, 0700) != 0 && errno != EEXIST) {
    SDL_SetError("Couldn't create directory '%s': %s", path, strerror(errno));
    SDL_free(path);
    return NULL;
  }
  SDL_strlcat(path, "/", length);
  return path;
}

#endif /* SDL_FILESYSTEM_PYXIS */
