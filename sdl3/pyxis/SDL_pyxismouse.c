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
  if (!cursor || !image || !SDL_LockSurface(surface)) {
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
  cursor->internal = (SDL_CursorData *)image;
  return cursor;
}

static void PYXIS_FreeCursor(SDL_Cursor *cursor)
{
  SDL_free(cursor->internal);
  SDL_free(cursor);
}

static bool PYXIS_ShowCursor(SDL_Cursor *cursor)
{
  SDL_Mouse *mouse = SDL_GetMouse();
  if (!pyxis_video.pointer_owned) {
    return true; /* Apply SDL's saved preference after native acquisition. */
  }
  /* SDL passes NULL for effective lock hiding. Keep its saved show preference;
   * native lock independently hides the cursor and restores it on unlock. */
  if (!cursor && mouse->relative_mode && mouse->cursor_shown) {
    cursor = mouse->cur_cursor ? mouse->cur_cursor : mouse->def_cursor;
  }
  enum call_status status = CALL_OK;
  if (cursor) {
    const struct pyxis_cursor *image = (const struct pyxis_cursor *)cursor->internal;
    status = image ? pointer_set_image(pyxis_video.pointer, image->pixels,
        image->width, image->height, image->hotspot_x, image->hotspot_y) :
        pointer_default_image(pyxis_video.pointer);
  }
  if (status == CALL_OK) {
    status = pointer_set_visible(pyxis_video.pointer, mouse->cursor_shown);
  }
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis cursor update failed (status %u)", (unsigned)status);
  }
  return true;
}

static bool PYXIS_WarpMouse(SDL_Window *window, float x, float y)
{
  if (window != pyxis_video.window || !pyxis_video.pointer_owned) {
    return SDL_SetError("Pyxis warp needs the owning pointer session");
  }
  struct pointer_geometry geometry;
  enum call_status status = pointer_geometry(pyxis_video.pointer, &geometry);
  if (status == CALL_OK) {
    PYXIS_MapPointer(&x, &y, true);
    status = pointer_warp(pyxis_video.pointer, (int64_t)x, (int64_t)y, geometry.generation,
        geometry.mapping_identity);
  }
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis pointer warp refused (status %u)", (unsigned)status);
  }
  /* Success is delivered as native ordinary input; never synthesize position. */
  return true;
}

static bool PYXIS_SetRelativeMouseMode(bool enabled)
{
  if (!pyxis_video.pointer_owned || (enabled && !PYXIS_HasPresented())) {
    return SDL_SetError("Pyxis relative mode needs a presented pointer surface");
  }
  enum call_status status = enabled ? pointer_lock(pyxis_video.pointer) :
      pointer_unlock(pyxis_video.pointer);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis pointer %s refused (status %u)",
        enabled ? "lock" : "unlock", (unsigned)status);
  }
  return true;
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

bool PYXIS_AcquirePointer(void)
{
  if (pyxis_video.pointer_owned || pyxis_video.pointer == HANDLE_INVALID) {
    return true;
  }
  enum call_status status = pointer_acquire(pyxis_video.pointer);
  if (status == CALL_UNAVAILABLE) {
    return true;
  }
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis pointer acquisition failed (status %u)", (unsigned)status);
  }
  pyxis_video.pointer_owned = true;
  SDL_Mouse *mouse = SDL_GetMouse();
  if (!PYXIS_ShowCursor(mouse->cur_cursor ? mouse->cur_cursor : mouse->def_cursor)) {
    pointer_release(pyxis_video.pointer);
    pyxis_video.pointer_owned = false;
    return false;
  }
  return true;
}

#endif /* SDL_VIDEO_DRIVER_PYXIS */
