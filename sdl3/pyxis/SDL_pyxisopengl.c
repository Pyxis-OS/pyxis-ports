/* Static OSMesa contexts render into retained, top-down BGRA canvases. Native
 * output slots are only touched under the video lock, never by Mesa workers. */

#include "SDL_internal.h"

#if defined(SDL_VIDEO_DRIVER_PYXIS) && defined(PYXIS_USE_OSMESA)

#include <GL/osmesa.h>

#include "SDL_pyxisvideo.h"
#include "events/SDL_windowevents_c.h"

struct pyxis_gl_context {
  OSMesaContext mesa;
  SDL_Window *window; /* Cleared on window destruction, independently of Mesa. */
  SDL_ThreadID owner; /* Zero when detached. */
  void *pixels;
  int width, height;
  bool double_buffer;
  struct pyxis_gl_context *next;
};

static struct pyxis_gl_context *contexts;
static _Thread_local struct pyxis_gl_context *current_context;

static bool PYXIS_GL_LoadLibrary(SDL_VideoDevice *device, const char *path)
{
  (void)device;
  if (path && *path) {
    return SDL_SetError("Pyxis OpenGL is statically linked; library paths are unsupported");
  }
  return true;
}

static void PYXIS_GL_UnloadLibrary(SDL_VideoDevice *device)
{
  (void)device;
}

static SDL_FunctionPointer PYXIS_GL_GetProcAddress(SDL_VideoDevice *device, const char *name)
{
  (void)device;
  SDL_FunctionPointer address = (SDL_FunctionPointer)OSMesaGetProcAddress(name);
  if (!address) {
    SDL_SetError("OSMesa has no OpenGL procedure %s", name);
  }
  return address;
}

/* Binding may flush the previous Mesa framebuffer. Its CPU storage remains
 * alive until the successful bind, and explicit detach finishes before unbind. */
static bool bind_context(struct pyxis_gl_context *context, int width, int height)
{
  bool resized = context->width != width || context->height != height;
  void *pixels = context->pixels;
  if (resized) {
    if (width <= 0 || height <= 0 || (size_t)width > SIZE_MAX / 4 / (size_t)height) {
      return SDL_SetError("Pyxis OpenGL canvas dimensions overflow");
    }
    pixels = SDL_calloc((size_t)width * (size_t)height, 4);
    if (!pixels) {
      return false;
    }
  }
  if (current_context) {
    glFinish();
  }
  if (!OSMesaMakeCurrent(context->mesa, pixels, GL_UNSIGNED_BYTE, width, height)) {
    if (resized) {
      SDL_free(pixels);
    }
    return SDL_SetError("OSMesa could not bind the OpenGL canvas");
  }
  OSMesaPixelStore(OSMESA_Y_UP, 0);
  if (current_context && current_context != context) {
    current_context->owner = 0;
  }
  if (resized) {
    SDL_free(context->pixels);
    context->pixels = pixels;
    context->width = width;
    context->height = height;
  }
  context->owner = SDL_GetCurrentThreadID();
  current_context = context;
  return true;
}

static bool PYXIS_GL_MakeCurrent(SDL_VideoDevice *device, SDL_Window *window,
    SDL_GLContext opaque)
{
  (void)device;
  struct pyxis_gl_context *context = (struct pyxis_gl_context *)opaque;
  PYXIS_LockVideo();
  bool result = true;
  if (!context) {
    if (current_context) {
      glFinish();
      if (!OSMesaMakeCurrent(NULL, NULL, GL_UNSIGNED_BYTE, 0, 0)) {
        result = SDL_SetError("OSMesa could not detach the current context");
      } else {
        current_context->owner = 0;
        current_context = NULL;
      }
    }
  } else if (!window || window != pyxis_video.window) {
    result = SDL_SetError("Pyxis OpenGL needs a live window");
  } else if (context->owner && context->owner != SDL_GetCurrentThreadID()) {
    result = SDL_SetError("Pyxis OpenGL context is current on another thread");
  } else {
    result = bind_context(context, pyxis_video.canvas_width, pyxis_video.canvas_height);
    if (result) {
      context->window = window;
    }
  }
  PYXIS_UnlockVideo();
  return result;
}

static bool supported_attributes(SDL_VideoDevice *device)
{
  if (device->gl_config.stereo || device->gl_config.multisamplebuffers ||
      device->gl_config.multisamplesamples || device->gl_config.floatbuffers ||
      device->gl_config.flags || device->gl_config.no_error ||
      device->gl_config.framebuffer_srgb_capable || device->gl_config.reset_notification ||
      device->gl_config.release_behavior != SDL_GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH) {
    return SDL_SetError("OSMesa does not support the requested stereo, multisample, floating, sRGB or context options");
  }
  if (device->gl_config.profile_mask != 0 &&
      device->gl_config.profile_mask != SDL_GL_CONTEXT_PROFILE_CORE &&
      device->gl_config.profile_mask != SDL_GL_CONTEXT_PROFILE_COMPATIBILITY) {
    return SDL_SetError("OSMesa supports desktop OpenGL core or compatibility profiles");
  }
  if (device->gl_config.accelerated > 0) {
    return SDL_SetError("Pyxis OSMesa provides software OpenGL only");
  }
  if (device->gl_config.red_size > 8 || device->gl_config.green_size > 8 ||
      device->gl_config.blue_size > 8 || device->gl_config.alpha_size > 8 ||
      device->gl_config.buffer_size > 32) {
    return SDL_SetError("Pyxis OpenGL uses an eight-bit BGRA canvas");
  }
  if (device->gl_config.depth_size > 32 || device->gl_config.stencil_size > 8 ||
      device->gl_config.accum_red_size > 16 || device->gl_config.accum_green_size > 16 ||
      device->gl_config.accum_blue_size > 16 || device->gl_config.accum_alpha_size > 16) {
    return SDL_SetError("OSMesa cannot provide the requested depth, stencil or accumulation precision");
  }
  return true;
}

static SDL_GLContext PYXIS_GL_CreateContext(SDL_VideoDevice *device, SDL_Window *window)
{
  if (!supported_attributes(device)) {
    return NULL;
  }
  struct pyxis_gl_context *context = SDL_calloc(1, sizeof(*context));
  if (!context) {
    return NULL;
  }
  int accum = SDL_max(SDL_max(device->gl_config.accum_red_size, device->gl_config.accum_green_size),
      SDL_max(device->gl_config.accum_blue_size, device->gl_config.accum_alpha_size));
  const int attributes[] = {
    OSMESA_FORMAT, OSMESA_BGRA,
    OSMESA_DEPTH_BITS, device->gl_config.depth_size,
    OSMESA_STENCIL_BITS, device->gl_config.stencil_size,
    OSMESA_ACCUM_BITS, accum,
    OSMESA_PROFILE, device->gl_config.profile_mask == SDL_GL_CONTEXT_PROFILE_CORE ?
        OSMESA_CORE_PROFILE : OSMESA_COMPAT_PROFILE,
    OSMESA_CONTEXT_MAJOR_VERSION, device->gl_config.major_version,
    OSMESA_CONTEXT_MINOR_VERSION, device->gl_config.minor_version,
    OSMESA_DOUBLEBUFFER, device->gl_config.double_buffer != 0,
    0
  };
  PYXIS_LockVideo();
  OSMesaContext shared = NULL;
  if (device->gl_config.share_with_current_context) {
    if (!current_context) {
      PYXIS_UnlockVideo();
      SDL_free(context);
      SDL_SetError("OSMesa sharing needs a current context on this thread");
      return NULL;
    }
    shared = current_context->mesa;
  }
  context->mesa = OSMesaCreateContextAttribs(attributes, shared);
  context->double_buffer = device->gl_config.double_buffer != 0;
  if (!context->mesa) {
    PYXIS_UnlockVideo();
    SDL_free(context);
    SDL_SetError("OSMesa could not create the requested OpenGL context");
    return NULL;
  }
  bool result = bind_context(context, pyxis_video.canvas_width, pyxis_video.canvas_height);
  if (result) {
    context->window = window;
    context->next = contexts;
    contexts = context;
  }
  PYXIS_UnlockVideo();
  if (!result) {
    OSMesaDestroyContext(context->mesa);
    SDL_free(context->pixels);
    SDL_free(context);
    return NULL;
  }
  return (SDL_GLContext)context;
}

static bool PYXIS_GL_DestroyContext(SDL_VideoDevice *device, SDL_GLContext opaque)
{
  (void)device;
  struct pyxis_gl_context *context = (struct pyxis_gl_context *)opaque;
  PYXIS_LockVideo();
  if (context->owner && context->owner != SDL_GetCurrentThreadID()) {
    PYXIS_UnlockVideo();
    return SDL_SetError("Detach the OpenGL context before deleting it on another thread");
  }
  if (context == current_context) {
    glFinish();
    OSMesaMakeCurrent(NULL, NULL, GL_UNSIGNED_BYTE, 0, 0);
    current_context = NULL;
  }
  struct pyxis_gl_context **link = &contexts;
  while (*link && *link != context) {
    link = &(*link)->next;
  }
  if (*link) {
    *link = context->next;
  }
  OSMesaDestroyContext(context->mesa);
  SDL_free(context->pixels);
  SDL_free(context);
  PYXIS_UnlockVideo();
  return true;
}

static bool PYXIS_GL_SwapWindow(SDL_VideoDevice *device, SDL_Window *window)
{
  (void)device;
  PYXIS_LockVideo();
  struct pyxis_gl_context *context = current_context;
  bool result;
  if (!context || context->window != window || window != pyxis_video.window) {
    result = SDL_SetError("Pyxis OpenGL window is not current on this thread");
  } else if (context->double_buffer && !OSMesaSwapBuffers(context->mesa)) {
    result = SDL_SetError("OSMesa could not swap the current OpenGL context");
  } else {
    if (!context->double_buffer) {
      glFinish();
    }
    result = PYXIS_PresentPixels(context->pixels, context->width, context->height);
  }
  PYXIS_UnlockVideo();
  return result;
}

void PYXIS_GL_DestroyWindow(SDL_Window *window)
{
  for (struct pyxis_gl_context *context = contexts; context; context = context->next) {
    if (context->window == window) {
      context->window = NULL;
    }
  }
}

/* The requested size becomes the logical canvas. A context current on another
 * thread must be detached first; a refusal keeps the old canvas. */
void PYXIS_GL_ResizeWindow(SDL_VideoDevice *device, SDL_Window *window)
{
  (void)device;
  PYXIS_LockVideo();
  int width = window->pending.w, height = window->pending.h;
  bool refused = false;
  for (struct pyxis_gl_context *context = contexts; context; context = context->next) {
    if (context->window == window && context->owner &&
        context->owner != SDL_GetCurrentThreadID()) {
      refused = true;
    }
  }
  if (refused) {
    SDL_SetError("Detach the rendering thread's OpenGL context before resizing the canvas");
  } else if (current_context && current_context->window == window &&
      !bind_context(current_context, width, height)) {
    refused = true;
  }
  if (!refused) {
    pyxis_video.canvas_width = width;
    pyxis_video.canvas_height = height;
    PYXIS_UpdateViewport();
  }
  width = pyxis_video.canvas_width;
  height = pyxis_video.canvas_height;
  PYXIS_UnlockVideo();
  SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_RESIZED, width, height);
}

static bool PYXIS_GL_SetSwapInterval(SDL_VideoDevice *device, int interval)
{
  (void)device;
  if (interval != 0) {
    return SDL_SetError("Pyxis OpenGL supports only swap interval zero");
  }
  return true;
}

static bool PYXIS_GL_GetSwapInterval(SDL_VideoDevice *device, int *interval)
{
  (void)device;
  *interval = 0;
  return true;
}

void PYXIS_GL_InitDevice(SDL_VideoDevice *device)
{
  device->GL_LoadLibrary = PYXIS_GL_LoadLibrary;
  device->GL_UnloadLibrary = PYXIS_GL_UnloadLibrary;
  device->GL_GetProcAddress = PYXIS_GL_GetProcAddress;
  device->GL_CreateContext = PYXIS_GL_CreateContext;
  device->GL_MakeCurrent = PYXIS_GL_MakeCurrent;
  device->GL_DestroyContext = PYXIS_GL_DestroyContext;
  device->GL_SwapWindow = PYXIS_GL_SwapWindow;
  device->GL_SetSwapInterval = PYXIS_GL_SetSwapInterval;
  device->GL_GetSwapInterval = PYXIS_GL_GetSwapInterval;
}

#endif /* SDL_VIDEO_DRIVER_PYXIS && PYXIS_USE_OSMESA */
