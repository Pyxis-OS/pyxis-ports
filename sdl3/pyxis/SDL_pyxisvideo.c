/* Pyxis video driver: one window over the space's display session. SDL draws
 * into its own surface; presenting copies the whole surface into a held
 * display slot and submits it, so the presenter shows only complete frames.
 * OpenGL windows keep the application's logical canvas, aspect-fitted to the
 * output (SDL_pyxisopengl.c). */

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <environment.h>
#include <handle.h>
#include <keyboard.h>
#include <pointer.h>
#include <startup.h>

#include "SDL_pyxisvideo.h"
#include "events/SDL_keyboard_c.h"
#include "events/SDL_mouse_c.h"
#include "events/SDL_windowevents_c.h"

#define PYXIS_DRIVER_NAME "pyxis"
#define PIXEL_BYTES 4

struct pyxis_video pyxis_video = {
  .display = HANDLE_INVALID,
  .keyboard = HANDLE_INVALID,
  .pointer = HANDLE_INVALID,
  .clock = HANDLE_INVALID,
};

static SDL_DisplayID display_id;
static SDL_Surface *window_surface; /* SDL's drawing surface for a framebuffer window. */

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

void PYXIS_MapPointer(float *x, float *y, bool to_native)
{
#ifdef PYXIS_USE_OSMESA
  PYXIS_LockVideo();
  const SDL_Rect *v = &pyxis_video.viewport;
  if (pyxis_video.canvas_width > 0 && v->w > 0 && v->h > 0) {
    if (to_native) {
      *x = (float)v->x + *x * (float)v->w / (float)pyxis_video.canvas_width;
      *y = (float)v->y + *y * (float)v->h / (float)pyxis_video.canvas_height;
    } else {
      float logical_x = (*x - (float)v->x) * (float)pyxis_video.canvas_width / (float)v->w;
      float logical_y = (*y - (float)v->y) * (float)pyxis_video.canvas_height / (float)v->h;
      *x = SDL_clamp(logical_x, 0.0f, (float)(pyxis_video.canvas_width - 1));
      *y = SDL_clamp(logical_y, 0.0f, (float)(pyxis_video.canvas_height - 1));
    }
  }
  PYXIS_UnlockVideo();
#else
  (void)x;
  (void)y;
  (void)to_native;
#endif
}

bool PYXIS_AcquireDisplay(void)
{
  if (!pyxis_video.display_owned) {
    enum call_status status = display_acquire(pyxis_video.display, &pyxis_video.buffer);
    if (status != CALL_OK) {
      return SDL_SetError("Pyxis display acquisition failed (status %u)", (unsigned)status);
    }
    pyxis_video.display_owned = true;
  }
  return true;
}

static bool submit_slot(void)
{
  struct display_submit_reply submitted;
  enum call_status status = display_submit(pyxis_video.display, pyxis_video.slot, &submitted);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis display submission failed (status %u)", (unsigned)status);
  }
  pyxis_video.slot = submitted.next;
  pyxis_video.presented = true;
  return true;
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

/* Scale the top-down BGRA canvas into the viewport of the held slot. */
bool PYXIS_PresentPixels(const void *pixels, int width, int height)
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
  return submit_slot();
}
#endif

static bool display_mode(const struct display_size_reply *size, SDL_DisplayMode *mode)
{
  SDL_zerop(mode);
  mode->format = SDL_GetPixelFormatForMasks(32, UINT32_C(0xff) << size->red_shift,
      UINT32_C(0xff) << size->green_shift, UINT32_C(0xff) << size->blue_shift, 0);
  mode->w = (int)size->width;
  mode->h = (int)size->height;
  return mode->format != SDL_PIXELFORMAT_UNKNOWN && mode->w > 0 && mode->h > 0;
}

/* Copy SDL's whole surface into the held slot, clipped to both, and submit
 * it. Slots rotate and each keeps an older frame, so dirty rectangles alone
 * would leave stale areas behind. Pixels outside the surface stay zero. */
static bool submit_surface(const SDL_Surface *surface)
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
  return submit_slot();
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
  (void)device;
  struct display_size_reply size;
  if (display_size(pyxis_video.display, &size) != CALL_OK ||
      size.generation == pyxis_video.generation) {
    return;
  }

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
    if (window_surface && pyxis_video.presented) {
      submit_surface(window_surface);
    }
  }
  pyxis_video.generation = size.generation;
  PYXIS_UnlockVideo();

  SDL_DisplayMode mode;
  SDL_VideoDisplay *display = SDL_GetVideoDisplay(display_id);
  if (!display || !display_mode(&size, &mode)) {
    return;
  }
  SDL_ResetFullscreenDisplayModes(display);
  SDL_AddFullscreenDisplayMode(display, &mode);
  SDL_SetDesktopDisplayMode(display, &mode);
  SDL_SetCurrentDisplayMode(display, &mode);
  if (pyxis_video.window) {
#ifdef PYXIS_USE_OSMESA
    if (pyxis_video.window->flags & SDL_WINDOW_OPENGL) {
      SDL_SendWindowEvent(pyxis_video.window, SDL_EVENT_WINDOW_EXPOSED, 0, 0);
      return; /* Output geometry does not change the application's canvas. */
    }
#endif
    SDL_SendWindowEvent(pyxis_video.window, SDL_EVENT_WINDOW_RESIZED, mode.w, mode.h);
  }
}

static void release_video_grants(void)
{
  handle_t *grants[] = {&pyxis_video.display, &pyxis_video.keyboard, &pyxis_video.pointer,
      &pyxis_video.clock};
  for (size_t i = 0; i < SDL_arraysize(grants); ++i) {
    if (*grants[i] != HANDLE_INVALID) {
      handle_close(*grants[i]);
      *grants[i] = HANDLE_INVALID;
    }
  }
}

static bool PYXIS_VideoInit(SDL_VideoDevice *device)
{
  (void)device;
#ifdef PYXIS_USE_OSMESA
  pyxis_video.mutex = SDL_CreateMutex();
  if (!pyxis_video.mutex) {
    return false;
  }
#endif
  environment_capability(startup_environment_handle(), "display", &pyxis_video.display);
  environment_capability(startup_environment_handle(), "keyboard", &pyxis_video.keyboard);
  environment_capability(startup_environment_handle(), "pointer", &pyxis_video.pointer);
  environment_capability(startup_environment_handle(), "clock", &pyxis_video.clock);
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
  display_id = SDL_AddBasicVideoDisplay(&mode);
  if (!display_id) {
    release_video_grants();
    return false;
  }
  SDL_AddKeyboard(SDL_DEFAULT_KEYBOARD_ID, NULL, false);
  if (pyxis_video.pointer != HANDLE_INVALID) {
    SDL_AddMouse(SDL_DEFAULT_MOUSE_ID, NULL, false);
  }
  PYXIS_InitMouse();
  PYXIS_InitWakeup();
  return true;
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
#ifdef PYXIS_USE_OSMESA
  SDL_DestroyMutex(pyxis_video.mutex);
  pyxis_video.mutex = NULL;
#endif
}

static bool PYXIS_CreateWindow(SDL_VideoDevice *device, SDL_Window *window, SDL_PropertiesID props)
{
  (void)device;
  (void)props;
  if (pyxis_video.window) {
    return SDL_SetError("Pyxis supports one window");
  }

  enum call_status status = keyboard_acquire(pyxis_video.keyboard);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis keyboard acquisition failed (status %u)", (unsigned)status);
  }
  pyxis_video.keyboard_owned = true;
  /* The window always covers the display; focus arrives as input events. */
  window->x = window->y = 0;
#ifdef PYXIS_USE_OSMESA
  if (window->flags & SDL_WINDOW_OPENGL) {
    /* The application's size is its canvas, independently of the output. */
    pyxis_video.canvas_width = window->w;
    pyxis_video.canvas_height = window->h;
    PYXIS_LockVideo();
    bool acquired = PYXIS_AcquireDisplay();
    PYXIS_UpdateViewport();
    PYXIS_UnlockVideo();
    if (!acquired || !PYXIS_AcquirePointer()) {
      release_input();
      release_display();
      return false;
    }
    pyxis_video.window = window;
    return true;
  }
#endif
  const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(display_id);
  window->w = mode->w;
  window->h = mode->h;
  pyxis_video.window = window;
  return true;
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

/* Framebuffer windows always cover the output; an OpenGL window's size is its
 * logical canvas. Either way the result arrives as SDL's resize event. */
static void PYXIS_SetWindowSize(SDL_VideoDevice *device, SDL_Window *window)
{
#ifdef PYXIS_USE_OSMESA
  if (window->flags & SDL_WINDOW_OPENGL) {
    PYXIS_GL_ResizeWindow(device, window);
    return;
  }
#endif
  (void)device;
  const SDL_DisplayMode *mode = SDL_GetDesktopDisplayMode(display_id);
  SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_RESIZED, mode->w, mode->h);
}

/* The window already covers the output, so fullscreen changes nothing. */
static SDL_FullscreenResult PYXIS_SetWindowFullscreen(SDL_VideoDevice *device,
    SDL_Window *window, SDL_VideoDisplay *display, SDL_FullscreenOp fullscreen)
{
  (void)device;
  (void)window;
  (void)display;
  (void)fullscreen;
  return SDL_FULLSCREEN_SUCCEEDED;
}

static void PYXIS_DestroyWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window)
{
  (void)device;
  (void)window;
  SDL_DestroySurface(window_surface);
  window_surface = NULL;
}

static bool PYXIS_CreateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window,
    SDL_PixelFormat *format, void **pixels, int *pitch)
{
  PYXIS_DestroyWindowFramebuffer(device, window);
  PYXIS_LockVideo();
  bool acquired = PYXIS_AcquireDisplay();
  PYXIS_UnlockVideo();
  if (!acquired || !PYXIS_AcquirePointer()) {
    return false;
  }

  int width, height;
  SDL_GetWindowSizeInPixels(window, &width, &height);
  *format = SDL_GetDesktopDisplayMode(display_id)->format;
  window_surface = SDL_CreateSurface(width, height, *format);
  if (!window_surface) {
    return false;
  }
  *pixels = window_surface->pixels;
  *pitch = window_surface->pitch;
  return true;
}

static bool PYXIS_UpdateWindowFramebuffer(SDL_VideoDevice *device, SDL_Window *window,
    const SDL_Rect *rects, int numrects)
{
  (void)device;
  (void)window;
  (void)rects;
  (void)numrects;
  if (!window_surface || !pyxis_video.display_owned) {
    return SDL_SetError("Pyxis window has no framebuffer");
  }
  /* The first submitted frame shows graphics. */
  PYXIS_LockVideo();
  bool result = submit_surface(window_surface);
  PYXIS_UnlockVideo();
  return result;
}

static void PYXIS_DeleteDevice(SDL_VideoDevice *device)
{
  SDL_free(device);
}

static SDL_VideoDevice *PYXIS_CreateDevice(void)
{
  SDL_VideoDevice *device = SDL_calloc(1, sizeof(*device));
  if (!device) {
    return NULL;
  }
  device->VideoInit = PYXIS_VideoInit;
  device->VideoQuit = PYXIS_VideoQuit;
  device->PumpEvents = PYXIS_PumpEvents;
  device->WaitEventTimeout = PYXIS_WaitEventTimeout;
  device->SendWakeupEvent = PYXIS_SendWakeupEvent;
  device->CreateSDLWindow = PYXIS_CreateWindow;
  device->DestroyWindow = PYXIS_DestroyWindow;
  device->SetWindowSize = PYXIS_SetWindowSize;
  device->SetWindowFullscreen = PYXIS_SetWindowFullscreen;
  device->CreateWindowFramebuffer = PYXIS_CreateWindowFramebuffer;
  device->UpdateWindowFramebuffer = PYXIS_UpdateWindowFramebuffer;
  device->DestroyWindowFramebuffer = PYXIS_DestroyWindowFramebuffer;
#ifdef PYXIS_USE_OSMESA
  PYXIS_GL_InitDevice(device);
#endif
  /* Sizes are reported by the backend: the output for framebuffer windows,
   * the canvas for OpenGL windows. */
  device->device_caps = VIDEO_DEVICE_CAPS_SENDS_FULLSCREEN_DIMENSIONS;
  device->free = PYXIS_DeleteDevice;
  return device;
}

VideoBootStrap PYXIS_bootstrap = {
  PYXIS_DRIVER_NAME, "Pyxis display", PYXIS_CreateDevice, NULL, false,
};

#endif /* SDL_VIDEO_DRIVER_PYXIS */
