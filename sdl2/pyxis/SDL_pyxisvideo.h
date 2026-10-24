#ifndef SDL_pyxisvideo_h_
#define SDL_pyxisvideo_h_

/* Pyxis video driver state shared by the video and event halves. A process
 * has one display grant, so there is one window and one set of sessions. */

#include <display.h>
#include <stdbool.h>

#include "SDL_internal.h"
#include "SDL_events.h"
#include "SDL_mutex.h"
#include "video/SDL_sysvideo.h"

struct pyxis_video {
  handle_t display;
  handle_t keyboard;
  handle_t pointer; /* HANDLE_INVALID without a pointer grant. */
  handle_t clock;
  handle_t clipboard_local;
  handle_t clipboard_shared;
  SDL_Window *window; /* The only window, or NULL. */
  /* Input sessions belong to the window. Keyboard starts at window creation;
   * pointer starts after graphics acquisition. Destruction releases both. */
  bool keyboard_owned;
  bool pointer_owned;
  /* Starts with the first surface framebuffer, or an OpenGL window. */
  bool display_owned;
  bool presented; /* A frame has been submitted since acquisition. */
  uint64_t slot; /* Held display slot the next frame is copied into. */
  struct display_buffer buffer; /* Valid while display_owned. */
  uint64_t generation; /* Display geometry the window currently follows. */
#ifdef PYXIS_USE_OSMESA
  SDL_mutex *mutex; /* Serializes native mappings and GL context attachments. */
  int canvas_width, canvas_height;
  SDL_Rect viewport;
#endif
};

extern struct pyxis_video pyxis_video;

void PYXIS_LockVideo(void);
void PYXIS_UnlockVideo(void);
int PYXIS_AcquireDisplay(void); /* Caller holds the video lock. */
/* Coordinates use one locked snapshot; callbacks run after unlocking. */
void PYXIS_MapPointer(int *x, int *y, bool to_native);
bool PYXIS_PointerGeometry(uint64_t generation, uint64_t width, uint64_t height);
bool PYXIS_HasPresented(void);

#ifdef PYXIS_USE_OSMESA
int PYXIS_PresentPixels(const void *pixels, int width, int height);
void PYXIS_UpdateViewport(void);
void PYXIS_GL_InitDevice(SDL_VideoDevice *device);
void PYXIS_GL_DestroyWindow(SDL_Window *window);
void PYXIS_GL_ResizeWindow(SDL_VideoDevice *device, SDL_Window *window);
#endif

/* Replace the mapping and update the display mode. Surface windows follow
 * output size; OpenGL windows preserve their logical canvas. A failed
 * replacement keeps the old mapping and waits for the next generation. */
void PYXIS_FollowDisplaySize(SDL_VideoDevice *device);

void PYXIS_InitInput(void);
void PYXIS_InitMouse(void);
int PYXIS_AcquirePointer(void);
void PYXIS_PumpEvents(SDL_VideoDevice *device);
int PYXIS_WaitEventTimeout(SDL_VideoDevice *device, int timeout);
/* Create the cross-thread wakeup when the program holds threads authority. */
void PYXIS_InitWakeup(void);
void PYXIS_SendWakeupEvent(SDL_VideoDevice *device, SDL_Window *window);
/* Release held keys and buttons in SDL; the sessions are released separately. */
void PYXIS_ResetInput(void);

/* The event-core patch keeps the native identity outside public SDL events. */
int SDL_SendKeyboardKeyWithClipboard(Uint8 state, SDL_Scancode scancode,
    Uint64 id, bool shared);
int PYXIS_PushClipboardEvent(SDL_Event *event, Uint64 id);
bool PYXIS_QueueClipboard(uint64_t id, uint64_t operation, uint64_t layer);
void PYXIS_DeliverClipboard(uint64_t id, bool batch);
void PYXIS_DiscardClipboard(uint64_t id);
void PYXIS_CancelClipboard(void);
/* Event filters/watchers run before application command handling. */
void PYXIS_BeginClipboardCallback(void);
void PYXIS_EndClipboardCallback(void);
int PYXIS_SetClipboardText(SDL_VideoDevice *device, const char *text);
char *PYXIS_GetClipboardText(SDL_VideoDevice *device);
SDL_bool PYXIS_HasClipboardText(SDL_VideoDevice *device);

#endif /* SDL_pyxisvideo_h_ */
