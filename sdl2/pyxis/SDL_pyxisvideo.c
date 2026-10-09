/* Pyxis video driver: one fullscreen window over the space's display session.
 * SDL draws into its own surface; presenting copies the updated rectangles
 * into the display mapping, so the presenter never shows a cleared or partly
 * drawn frame. Rows can still tear, as the single-buffer contract permits. */

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <keyboard.h>
#include <pointer.h>
#include <startup.h>

#include "SDL_pyxisvideo.h"
#include "events/SDL_events_c.h"
#include "events/SDL_windowevents_c.h"

#define PYXIS_DRIVER_NAME "pyxis"
#define PYXIS_SURFACE "_SDL_PyxisSurface"
#define PIXEL_BYTES 4

struct pyxis_video pyxis_video = {
  .display = HANDLE_INVALID,
  .keyboard = HANDLE_INVALID,
  .pointer = HANDLE_INVALID,
  .clock = HANDLE_INVALID,
};

static bool display_mode(const struct display_size_reply *size, SDL_DisplayMode *mode)
{
  SDL_zerop(mode);
  mode->format = SDL_MasksToPixelFormatEnum(32, UINT32_C(0xff) << size->red_shift,
      UINT32_C(0xff) << size->green_shift, UINT32_C(0xff) << size->blue_shift, 0);
  mode->w = (int)size->width;
  mode->h = (int)size->height;
  return mode->format != SDL_PIXELFORMAT_UNKNOWN && mode->w > 0 && mode->h > 0;
}

/* Copy SDL's whole surface into the held slot, clipped to both, and submit
 * it. Slots rotate and each keeps an older frame, so dirty rectangles alone
 * would leave stale areas behind. Pixels outside the surface stay zero. */
static int submit_surface(const SDL_Surface *surface)
{
  const struct display_buffer *buffer = &pyxis_video.buffer;
  int width = SDL_min(surface->w, (int)buffer->width);
  int height = SDL_min(surface->h, (int)buffer->height);
  size_t bytes = (size_t)SDL_max(width, 0) * PIXEL_BYTES;
  uintptr_t slot = display_slot_address(buffer, pyxis_video.slot);
  for (int y = 0; y < height; ++y) {
    uint8_t *destination = (uint8_t *)slot + (size_t)y * buffer->pitch;
    const uint8_t *source = (const uint8_t *)surface->pixels +
        (size_t)y * (size_t)surface->pitch;
    SDL_memcpy(destination, source, bytes);
  }
  struct display_submit_reply submitted;
  enum call_status status = display_submit(pyxis_video.display, pyxis_video.slot,
      &submitted);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis display submission failed (status %u)", (unsigned)status);
  }
  pyxis_video.slot = submitted.next;
  pyxis_video.presented = true;
  return 0;
}

static void release_display(void)
{
  if (pyxis_video.display_owned) {
    display_release(pyxis_video.display);
    pyxis_video.display_owned = false;
    pyxis_video.presented = false;
    pyxis_video.slot = 0;
  }
}

static void release_input(void)
{
  if (pyxis_video.pointer_owned) {
    pointer_release(pyxis_video.pointer);
    pyxis_video.pointer_owned = false;
  }
  if (pyxis_video.keyboard_owned) {
    keyboard_release(pyxis_video.keyboard);
    pyxis_video.keyboard_owned = false;
  }
}

void PYXIS_FollowDisplaySize(SDL_VideoDevice *device)
{
  struct display_size_reply size;
  if (display_size(pyxis_video.display, &size) != CALL_OK ||
      size.generation == pyxis_video.generation) {
    return;
  }

  if (pyxis_video.display_owned && pyxis_video.buffer.generation != size.generation) {
    enum call_status status = display_replace(pyxis_video.display, size.generation,
        &pyxis_video.buffer);
    if (status == CALL_BUSY) {
      return; /* Geometry moved on again; the next pump sees the newer one. */
    }
    if (status != CALL_OK) {
      /* The old mapping stays in use; retry only after another change. */
      pyxis_video.generation = size.generation;
      return;
    }
    /* Every new slot is held. Resubmit the last frame at the new geometry
     * rather than wait for the program's next update. */
    pyxis_video.slot = 0;
    SDL_Surface *surface = pyxis_video.window ?
        SDL_GetWindowData(pyxis_video.window, PYXIS_SURFACE) : NULL;
    if (surface && pyxis_video.presented) {
      submit_surface(surface);
    }
  }
  pyxis_video.generation = size.generation;

  SDL_DisplayMode mode;
  if (!display_mode(&size, &mode)) {
    return;
  }
  SDL_VideoDisplay *display = &device->displays[0];
  SDL_ResetDisplayModes(0);
  SDL_AddDisplayMode(display, &mode);
  SDL_SetDesktopDisplayMode(display, &mode);
  SDL_SetCurrentDisplayMode(display, &mode);
  if (pyxis_video.window) {
    pyxis_video.window->windowed.w = mode.w;
    pyxis_video.window->windowed.h = mode.h;
    SDL_SendWindowEvent(pyxis_video.window, SDL_WINDOWEVENT_RESIZED, mode.w, mode.h);
  }
}

static int PYXIS_VideoInit(SDL_VideoDevice *device)
{
  pyxis_video.display = startup_resource("display");
  pyxis_video.keyboard = startup_resource("keyboard");
  pyxis_video.pointer = startup_resource("pointer");
  pyxis_video.clock = startup_resource("clock");
  if (pyxis_video.display == HANDLE_INVALID || pyxis_video.keyboard == HANDLE_INVALID ||
      pyxis_video.clock == HANDLE_INVALID) {
    return SDL_SetError("Pyxis video needs the display, keyboard and clock grants");
  }

  struct display_size_reply size;
  enum call_status status = display_size(pyxis_video.display, &size);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis display size query failed (status %u)", (unsigned)status);
  }
  SDL_DisplayMode mode;
  if (!display_mode(&size, &mode)) {
    return SDL_SetError("Pyxis display has an unsupported pixel layout");
  }
  pyxis_video.generation = size.generation;
  if (SDL_AddBasicVideoDisplay(&mode) < 0) {
    return -1;
  }
  SDL_AddDisplayMode(&device->displays[0], &mode);
  PYXIS_InitInput();
  return 0;
}

static void PYXIS_VideoQuit(SDL_VideoDevice *device)
{
  (void)device;
  release_input();
  release_display();
  pyxis_video.window = NULL;
}

static int PYXIS_CreateWindow(SDL_VideoDevice *device, SDL_Window *window)
{
  if (pyxis_video.window) {
    return SDL_SetError("Pyxis supports one window");
  }

  enum call_status status = keyboard_acquire(pyxis_video.keyboard);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis keyboard acquisition failed (status %u)", (unsigned)status);
  }
  pyxis_video.keyboard_owned = true;
  /* The window always covers the display; focus arrives as input events. */
  const SDL_DisplayMode *mode = &device->displays[0].desktop_mode;
  window->flags |= SDL_WINDOW_FULLSCREEN;
  window->flags &= ~(Uint32)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS);
  window->x = window->windowed.x = 0;
  window->y = window->windowed.y = 0;
  window->w = window->windowed.w = mode->w;
  window->h = window->windowed.h = mode->h;
  pyxis_video.window = window;
  return 0;
}

static void PYXIS_DestroyWindow(SDL_VideoDevice *device, SDL_Window *window)
{
  (void)device;
  if (window != pyxis_video.window) {
    return;
  }
  PYXIS_ResetInput();
  release_input();
  release_display();
  pyxis_video.window = NULL;
}

static void PYXIS_DestroyWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window)
{
  (void)device;
  SDL_FreeSurface(SDL_SetWindowData(window, PYXIS_SURFACE, NULL));
}

static int PYXIS_CreateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window,
    Uint32 *format, void **pixels, int *pitch)
{
  PYXIS_DestroyWindowFramebuffer(device, window);
  if (!pyxis_video.display_owned) {
    enum call_status status = display_acquire(pyxis_video.display, &pyxis_video.buffer);
    if (status != CALL_OK) {
      return SDL_SetError("Pyxis display acquisition failed (status %u)", (unsigned)status);
    }
    pyxis_video.display_owned = true;
    /* A change since VideoInit is followed on the next event pump. */
  }

  if (PYXIS_AcquirePointer() < 0) {
    return -1;
  }

  int width, height;
  SDL_GetWindowSizeInPixels(window, &width, &height);
  *format = device->displays[0].desktop_mode.format;
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, *format);
  if (!surface) {
    return -1;
  }
  SDL_SetWindowData(window, PYXIS_SURFACE, surface);
  *pixels = surface->pixels;
  *pitch = surface->pitch;
  return 0;
}

static int PYXIS_UpdateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window,
    const SDL_Rect *rects, int numrects)
{
  (void)device;
  (void)rects;
  (void)numrects;
  SDL_Surface *surface = SDL_GetWindowData(window, PYXIS_SURFACE);
  if (!surface || !pyxis_video.display_owned) {
    return SDL_SetError("Pyxis window has no framebuffer");
  }
  /* The first submitted frame shows graphics. */
  return submit_surface(surface);
}

static void PYXIS_DeleteDevice(SDL_VideoDevice *device)
{
  SDL_free(device);
}

static SDL_VideoDevice *PYXIS_CreateDevice(void)
{
  SDL_VideoDevice *device = SDL_calloc(1, sizeof(*device));
  if (!device) {
    SDL_OutOfMemory();
    return NULL;
  }
  device->VideoInit = PYXIS_VideoInit;
  device->VideoQuit = PYXIS_VideoQuit;
  device->PumpEvents = PYXIS_PumpEvents;
  device->WaitEventTimeout = PYXIS_WaitEventTimeout;
  device->CreateSDLWindow = PYXIS_CreateWindow;
  device->DestroyWindow = PYXIS_DestroyWindow;
  device->CreateWindowFramebuffer = PYXIS_CreateWindowFramebuffer;
  device->UpdateWindowFramebuffer = PYXIS_UpdateWindowFramebuffer;
  device->DestroyWindowFramebuffer = PYXIS_DestroyWindowFramebuffer;
  device->free = PYXIS_DeleteDevice;
  return device;
}

VideoBootStrap PYXIS_bootstrap = {
  PYXIS_DRIVER_NAME, "Pyxis display", PYXIS_CreateDevice, NULL,
};

#endif /* SDL_VIDEO_DRIVER_PYXIS */
