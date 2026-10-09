/* Native pointer position, surface cursors and explicit relative lock. */

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <pointer.h>

#include "SDL_pyxisvideo.h"
#include "events/SDL_mouse_c.h"

struct pyxis_cursor {
  uint32_t width, height, hotspot_x, hotspot_y;
  uint8_t pixels[];
};

static SDL_Cursor *PYXIS_CreateCursor(SDL_Surface *surface, int hot_x, int hot_y)
{
  if (surface->w < 1 || surface->h < 1 || surface->w > POINTER_IMAGE_MAX ||
      surface->h > POINTER_IMAGE_MAX) {
    SDL_SetError("Pyxis cursor dimensions must be 1..64");
    return NULL;
  }
  SDL_Cursor *cursor = SDL_calloc(1, sizeof(*cursor));
  size_t row_bytes = (size_t)surface->w * 4;
  struct pyxis_cursor *image = SDL_malloc(sizeof(*image) + row_bytes * (size_t)surface->h);
  if (!cursor || !image) {
    SDL_free(cursor);
    SDL_free(image);
    SDL_OutOfMemory();
    return NULL;
  }
  if (SDL_LockSurface(surface) < 0) {
    SDL_free(cursor);
    SDL_free(image);
    return NULL;
  }
  image->width = (uint32_t)surface->w;
  image->height = (uint32_t)surface->h;
  image->hotspot_x = (uint32_t)hot_x;
  image->hotspot_y = (uint32_t)hot_y;
  /* SDL supplies ARGB8888: on Pyxis x86-64 its bytes are straight-alpha BGRA. */
  for (int y = 0; y < surface->h; ++y) {
    SDL_memcpy(image->pixels + (size_t)y * row_bytes,
        (const uint8_t *)surface->pixels + (size_t)y * (size_t)surface->pitch, row_bytes);
  }
  SDL_UnlockSurface(surface);
  cursor->driverdata = image;
  return cursor;
}

static void PYXIS_FreeCursor(SDL_Cursor *cursor)
{
  SDL_free(cursor->driverdata);
  SDL_free(cursor);
}

static int PYXIS_ShowCursor(SDL_Cursor *cursor)
{
  SDL_Mouse *mouse = SDL_GetMouse();
  if (!pyxis_video.pointer_owned) {
    return 0; /* Apply SDL's saved preference after native acquisition. */
  }
  /* SDL passes NULL for effective lock hiding. Keep its saved show preference;
   * native lock independently hides the cursor and restores it on unlock. */
  if (!cursor && mouse->relative_mode && mouse->cursor_shown) {
    cursor = mouse->cur_cursor ? mouse->cur_cursor : mouse->def_cursor;
  }
  enum call_status status = CALL_OK;
  if (cursor) {
    const struct pyxis_cursor *image = cursor->driverdata;
    status = image ? pointer_set_image(pyxis_video.pointer, image->pixels,
        image->width, image->height, image->hotspot_x, image->hotspot_y) :
        pointer_default_image(pyxis_video.pointer);
  }
  if (status == CALL_OK) {
    status = pointer_set_visible(pyxis_video.pointer, mouse->cursor_shown != SDL_FALSE);
  }
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis cursor update failed (status %u)", (unsigned)status);
  }
  return 0;
}

static void PYXIS_WarpMouse(SDL_Window *window, int x, int y)
{
  if (window != pyxis_video.window || !pyxis_video.pointer_owned) {
    SDL_SetError("Pyxis warp needs the owning pointer session");
    return;
  }
  struct pointer_geometry geometry;
  enum call_status status = pointer_geometry(pyxis_video.pointer, &geometry);
  if (status == CALL_OK) {
    status = pointer_warp(pyxis_video.pointer, x, y, geometry.generation,
        geometry.mapping_identity);
  }
  if (status != CALL_OK) {
    SDL_SetError("Pyxis pointer warp refused (status %u)", (unsigned)status);
  }
  /* Success is delivered as native ordinary input; never synthesize position. */
}

static int PYXIS_SetRelativeMouseMode(SDL_bool enabled)
{
  if (!pyxis_video.pointer_owned || (enabled && !pyxis_video.presented)) {
    return SDL_SetError("Pyxis relative mode needs a presented pointer surface");
  }
  enum call_status status = enabled ? pointer_lock(pyxis_video.pointer) :
      pointer_unlock(pyxis_video.pointer);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis pointer %s refused (status %u)",
        enabled ? "lock" : "unlock", (unsigned)status);
  }
  return 0;
}

void PYXIS_InitMouse(void)
{
  SDL_Mouse *mouse = SDL_GetMouse();
  mouse->CreateCursor = PYXIS_CreateCursor;
  mouse->FreeCursor = PYXIS_FreeCursor;
  mouse->ShowCursor = PYXIS_ShowCursor;
  mouse->WarpMouse = PYXIS_WarpMouse;
  mouse->SetRelativeMouseMode = PYXIS_SetRelativeMouseMode;
  /* A cursor with no driver data selects the native default arrow. */
  SDL_Cursor *cursor = SDL_calloc(1, sizeof(*cursor));
  if (cursor) {
    SDL_SetDefaultCursor(cursor);
  }
}

int PYXIS_AcquirePointer(void)
{
  if (pyxis_video.pointer_owned || pyxis_video.pointer == HANDLE_INVALID) {
    return 0;
  }
  enum call_status status = pointer_acquire(pyxis_video.pointer);
  if (status == CALL_UNAVAILABLE) {
    return 0;
  }
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis pointer acquisition failed (status %u)", (unsigned)status);
  }
  pyxis_video.pointer_owned = true;
  SDL_Mouse *mouse = SDL_GetMouse();
  if (PYXIS_ShowCursor(mouse->cur_cursor ? mouse->cur_cursor : mouse->def_cursor) < 0) {
    pointer_release(pyxis_video.pointer);
    pyxis_video.pointer_owned = false;
    return -1;
  }
  return 0;
}

#endif /* SDL_VIDEO_DRIVER_PYXIS */
