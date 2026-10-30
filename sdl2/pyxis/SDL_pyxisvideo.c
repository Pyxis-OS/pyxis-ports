/* Pyxis video driver: one fullscreen window over the space's display session.
 * SDL draws into its own surface; presenting copies the updated rectangles
 * into the display mapping, so the presenter never shows a cleared or partly
 * drawn frame. Rows can still tear, as the single-buffer contract permits. */

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <keyboard.h>
#include <pointer.h>
#include <startup.h>
#include <environment.h>
#include <handle.h>

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
  .clipboard_local = HANDLE_INVALID,
  .clipboard_shared = HANDLE_INVALID,
};

void PYXIS_LockVideo(void)
{
#ifdef PYXIS_USE_OSMESA
  SDL_LockMutex(pyxis_video.mutex);
#endif
}

void PYXIS_UnlockVideo(void)
{
#ifdef PYXIS_USE_OSMESA
  SDL_UnlockMutex(pyxis_video.mutex);
#endif
}

bool PYXIS_PointerGeometry(uint64_t generation, uint64_t width, uint64_t height)
{
  PYXIS_LockVideo();
  bool valid = generation == pyxis_video.generation &&
      width == pyxis_video.buffer.width && height == pyxis_video.buffer.height;
  PYXIS_UnlockVideo();
  return valid;
}

bool PYXIS_HasPresented(void)
{
  PYXIS_LockVideo();
  bool presented = pyxis_video.presented;
  PYXIS_UnlockVideo();
  return presented;
}

void PYXIS_MapPointer(int *x, int *y, bool to_native)
{
#ifdef PYXIS_USE_OSMESA
  PYXIS_LockVideo();
  const SDL_Rect *v = &pyxis_video.viewport;
  if (pyxis_video.canvas_width > 0 && v->w > 0 && v->h > 0) {
    if (to_native) {
      *x = v->x + (int)((int64_t)*x * v->w / pyxis_video.canvas_width);
      *y = v->y + (int)((int64_t)*y * v->h / pyxis_video.canvas_height);
    } else {
      int64_t logical_x = ((int64_t)*x - v->x) * pyxis_video.canvas_width / v->w;
      int64_t logical_y = ((int64_t)*y - v->y) * pyxis_video.canvas_height / v->h;
      *x = (int)SDL_clamp(logical_x, 0, pyxis_video.canvas_width - 1);
      *y = (int)SDL_clamp(logical_y, 0, pyxis_video.canvas_height - 1);
    }
  }
  PYXIS_UnlockVideo();
#else
  (void)x;
  (void)y;
  (void)to_native;
#endif
}

int PYXIS_AcquireDisplay(void)
{
  if (!pyxis_video.display_owned) {
    enum call_status status = display_acquire(pyxis_video.display, &pyxis_video.buffer);
    if (status != CALL_OK) {
      return SDL_SetError("Pyxis display acquisition failed (status %u)", (unsigned)status);
    }
    pyxis_video.display_owned = true;
  }
  return 0;
}

#ifdef PYXIS_USE_OSMESA
void PYXIS_UpdateViewport(void)
{
  int width = (int)pyxis_video.buffer.width;
  int height = (int)pyxis_video.buffer.height;
  int canvas_width = pyxis_video.canvas_width;
  int canvas_height = pyxis_video.canvas_height;
  if (!canvas_width || !canvas_height) {
    pyxis_video.viewport = (SDL_Rect){0};
    return;
  }
  int fit_width = width;
  int fit_height = (int)((int64_t)width * canvas_height / canvas_width);
  if (fit_height > height) {
    fit_height = height;
    fit_width = (int)((int64_t)height * canvas_width / canvas_height);
  }
  pyxis_video.viewport = (SDL_Rect){(width - fit_width) / 2, (height - fit_height) / 2,
      SDL_max(fit_width, 1), SDL_max(fit_height, 1)};
}

int PYXIS_PresentPixels(const void *pixels, int width, int height)
{
  if (!pyxis_video.display_owned) {
    return SDL_SetError("Pyxis window has no display session");
  }
  const struct display_buffer *buffer = &pyxis_video.buffer;
  const SDL_Rect *v = &pyxis_video.viewport;
  uint8_t *slot = (uint8_t *)display_slot_address(buffer, pyxis_video.slot);
  if (v->x != 0 || v->y != 0 || v->w != (int)buffer->width ||
      v->h != (int)buffer->height) {
    for (uint64_t y = 0; y < buffer->height; ++y) {
      SDL_memset(slot + y * buffer->pitch, 0, (size_t)buffer->width * PIXEL_BYTES);
    }
  }
  const unsigned red_shift = buffer->red_shift;
  const unsigned green_shift = buffer->green_shift;
  const unsigned blue_shift = buffer->blue_shift;
  if (width == v->w && height == v->h) {
    for (int y = 0; y < height; ++y) {
      uint32_t *destination = (uint32_t *)(slot + (size_t)(v->y + y) * buffer->pitch) + v->x;
      const uint32_t *source = (const uint32_t *)pixels + (size_t)y * (size_t)width;
      if (red_shift == 16 && green_shift == 8 && blue_shift == 0) {
        /* DISPLAY requires the spare byte to be zero, even for a BGRA canvas. */
        for (int x = 0; x < width; ++x) {
          destination[x] = source[x] & UINT32_C(0x00ffffff);
        }
      } else {
        for (int x = 0; x < width; ++x) {
          uint32_t pixel = source[x];
          destination[x] = ((pixel >> 16) & 255) << red_shift |
              ((pixel >> 8) & 255) << green_shift |
              (pixel & 255) << blue_shift;
        }
      }
    }
  } else {
    for (int y = 0; y < v->h; ++y) {
      uint32_t *destination = (uint32_t *)(slot + (size_t)(v->y + y) * buffer->pitch) + v->x;
      const uint32_t *source = (const uint32_t *)pixels +
          (size_t)((int64_t)y * height / v->h) * (size_t)width;
      for (int x = 0; x < v->w; ++x) {
        uint32_t pixel = source[(int64_t)x * width / v->w];
        destination[x] = ((pixel >> 16) & 255) << red_shift |
            ((pixel >> 8) & 255) << green_shift |
            (pixel & 255) << blue_shift;
      }
    }
  }
  struct display_submit_reply submitted;
  enum call_status status = display_submit(pyxis_video.display, pyxis_video.slot, &submitted);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis display submission failed (status %u)", (unsigned)status);
  }
  pyxis_video.slot = submitted.next;
  pyxis_video.presented = true;
  return 0;
}
#endif

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
  PYXIS_CancelClipboard();
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

  PYXIS_CancelClipboard();
  PYXIS_LockVideo();
  if (pyxis_video.display_owned && pyxis_video.buffer.generation != size.generation) {
    enum call_status status = display_replace(pyxis_video.display, size.generation,
        &pyxis_video.buffer);
    if (status == CALL_BUSY) {
      PYXIS_UnlockVideo();
      return; /* Geometry moved on again; the next pump sees the newer one. */
    }
    if (status != CALL_OK) {
      /* The old mapping stays in use; retry only after another change. */
      pyxis_video.generation = size.generation;
      PYXIS_UnlockVideo();
      return;
    }
    /* Every new slot is held. Resubmit the last frame at the new geometry
     * rather than wait for the program's next update. */
    pyxis_video.slot = 0;
#ifdef PYXIS_USE_OSMESA
    PYXIS_UpdateViewport();
#endif
    SDL_Surface *surface = pyxis_video.window ?
        SDL_GetWindowData(pyxis_video.window, PYXIS_SURFACE) : NULL;
    if (surface && pyxis_video.presented) {
      submit_surface(surface);
    }
  }
  pyxis_video.generation = size.generation;
  PYXIS_UnlockVideo();

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
#ifdef PYXIS_USE_OSMESA
    if (pyxis_video.window->flags & SDL_WINDOW_OPENGL) {
      SDL_SendWindowEvent(pyxis_video.window, SDL_WINDOWEVENT_EXPOSED, 0, 0);
      return; /* Output geometry does not change the application's canvas. */
    }
#endif
    pyxis_video.window->windowed.w = mode.w;
    pyxis_video.window->windowed.h = mode.h;
    SDL_SendWindowEvent(pyxis_video.window, SDL_WINDOWEVENT_RESIZED, mode.w, mode.h);
  }
}

static void release_video_grants(void)
{
  handle_t *grants[] = {&pyxis_video.display, &pyxis_video.keyboard, &pyxis_video.pointer,
      &pyxis_video.clock, &pyxis_video.clipboard_local, &pyxis_video.clipboard_shared};
  for (size_t i = 0; i < sizeof(grants) / sizeof(grants[0]); ++i) {
    if (*grants[i] != HANDLE_INVALID) {
      handle_close(*grants[i]);
      *grants[i] = HANDLE_INVALID;
    }
  }
}

static int PYXIS_VideoInit(SDL_VideoDevice *device)
{
#ifdef PYXIS_USE_OSMESA
  pyxis_video.mutex = SDL_CreateMutex();
  if (!pyxis_video.mutex) {
    return -1;
  }
#endif
  environment_capability(startup_environment_handle(), "display", &pyxis_video.display);
  environment_capability(startup_environment_handle(), "keyboard", &pyxis_video.keyboard);
  environment_capability(startup_environment_handle(), "pointer", &pyxis_video.pointer);
  environment_capability(startup_environment_handle(), "clock", &pyxis_video.clock);
  environment_capability(startup_environment_handle(), "clipboard_local", &pyxis_video.clipboard_local);
  environment_capability(startup_environment_handle(), "clipboard_shared", &pyxis_video.clipboard_shared);
  if (pyxis_video.display == HANDLE_INVALID || pyxis_video.keyboard == HANDLE_INVALID ||
      pyxis_video.clock == HANDLE_INVALID) {
    release_video_grants();
    return SDL_SetError("Pyxis video needs the display, keyboard and clock grants");
  }

  struct display_size_reply size;
  enum call_status status = display_size(pyxis_video.display, &size);
  if (status != CALL_OK) {
    release_video_grants();
    return SDL_SetError("Pyxis display size query failed (status %u)", (unsigned)status);
  }
  SDL_DisplayMode mode;
  if (!display_mode(&size, &mode)) {
    release_video_grants();
    return SDL_SetError("Pyxis display has an unsupported pixel layout");
  }
  pyxis_video.generation = size.generation;
  if (SDL_AddBasicVideoDisplay(&mode) < 0) {
    release_video_grants();
    return -1;
  }
  SDL_AddDisplayMode(&device->displays[0], &mode);
  PYXIS_InitInput();
  PYXIS_InitWakeup();
  return 0;
}

static void PYXIS_VideoQuit(SDL_VideoDevice *device)
{
  (void)device;
  release_input();
  PYXIS_LockVideo();
  release_display();
  pyxis_video.window = NULL;
  PYXIS_UnlockVideo();
  release_video_grants();
}

static int PYXIS_CreateWindow(SDL_VideoDevice *device, SDL_Window *window)
{
#ifdef PYXIS_USE_OSMESA
  int canvas_width = window->windowed.w;
  int canvas_height = window->windowed.h;
  Uint32 fullscreen_flags = window->flags & SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
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
#ifdef PYXIS_USE_OSMESA
  if (window->flags & SDL_WINDOW_OPENGL) {
    /* Fullscreen describes output coverage; canvas dimensions remain chosen
     * by the application, independently of the native session's geometry. */
    window->flags &= ~(Uint32)SDL_WINDOW_FULLSCREEN_DESKTOP;
    window->flags |= fullscreen_flags;
    window->w = window->windowed.w = canvas_width;
    window->h = window->windowed.h = canvas_height;
    pyxis_video.canvas_width = window->w;
    pyxis_video.canvas_height = window->h;
    PYXIS_LockVideo();
    int result = PYXIS_AcquireDisplay();
    PYXIS_UpdateViewport();
    PYXIS_UnlockVideo();
    if (result < 0 || PYXIS_AcquirePointer() < 0) {
      release_input();
      release_display();
      return -1;
    }
  }
#endif
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
  PYXIS_LockVideo();
#ifdef PYXIS_USE_OSMESA
  PYXIS_GL_DestroyWindow(window);
  pyxis_video.canvas_width = pyxis_video.canvas_height = 0;
#endif
  release_display();
  pyxis_video.window = NULL;
  PYXIS_UnlockVideo();
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
  PYXIS_LockVideo();
  int result = PYXIS_AcquireDisplay();
  PYXIS_UnlockVideo();
  if (result < 0) {
    return -1;
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
  PYXIS_LockVideo();
  int result = submit_surface(surface);
  PYXIS_UnlockVideo();
  return result;
}

static void PYXIS_DeleteDevice(SDL_VideoDevice *device)
{
#ifdef PYXIS_USE_OSMESA
  SDL_DestroyMutex(pyxis_video.mutex);
  pyxis_video.mutex = NULL;
#endif
  SDL_free(device);
}

static SDL_VideoDevice *PYXIS_CreateDevice(void)
{
  SDL_VideoDevice *device = SDL_calloc(1, sizeof(*device));
  if (!device) {
    SDL_OutOfMemory();
    return NULL;
  }
  device->SetClipboardText = PYXIS_SetClipboardText;
  device->GetClipboardText = PYXIS_GetClipboardText;
  device->HasClipboardText = PYXIS_HasClipboardText;
  device->VideoInit = PYXIS_VideoInit;
  device->VideoQuit = PYXIS_VideoQuit;
  device->PumpEvents = PYXIS_PumpEvents;
  device->WaitEventTimeout = PYXIS_WaitEventTimeout;
  device->SendWakeupEvent = PYXIS_SendWakeupEvent;
  device->CreateSDLWindow = PYXIS_CreateWindow;
  device->DestroyWindow = PYXIS_DestroyWindow;
  device->CreateWindowFramebuffer = PYXIS_CreateWindowFramebuffer;
  device->UpdateWindowFramebuffer = PYXIS_UpdateWindowFramebuffer;
  device->DestroyWindowFramebuffer = PYXIS_DestroyWindowFramebuffer;
#ifdef PYXIS_USE_OSMESA
  device->SetWindowSize = PYXIS_GL_ResizeWindow;
  PYXIS_GL_InitDevice(device);
#endif
  device->free = PYXIS_DeleteDevice;
  return device;
}

VideoBootStrap PYXIS_bootstrap = {
  PYXIS_DRIVER_NAME, "Pyxis display", PYXIS_CreateDevice, NULL,
};

#endif /* SDL_VIDEO_DRIVER_PYXIS */
