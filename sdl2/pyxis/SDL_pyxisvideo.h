#ifndef SDL_pyxisvideo_h_
#define SDL_pyxisvideo_h_

/* Pyxis video driver state shared by the video and event halves. A process
 * has one display grant, so there is one window and one set of sessions. */

#include <display.h>
#include <stdbool.h>

#include "SDL_internal.h"
#include "SDL_events.h"
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
  /* The display session starts with the first window framebuffer. */
  bool display_owned;
  bool presented; /* A frame has been submitted since acquisition. */
  uint64_t slot; /* Held display slot the next frame is copied into. */
  struct display_buffer buffer; /* Valid while display_owned. */
  uint64_t generation; /* Display geometry the window currently follows. */
};

extern struct pyxis_video pyxis_video;

/* Follow a display geometry change: replace the mapping, update the display
 * mode and resize the window. A failed replacement keeps the old mapping and
 * waits for the next generation. */
void PYXIS_FollowDisplaySize(SDL_VideoDevice *device);

void PYXIS_InitInput(void);
void PYXIS_InitMouse(void);
int PYXIS_AcquirePointer(void);
void PYXIS_PumpEvents(SDL_VideoDevice *device);
int PYXIS_WaitEventTimeout(SDL_VideoDevice *device, int timeout);
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
int PYXIS_SetClipboardText(SDL_VideoDevice *device, const char *text);
char *PYXIS_GetClipboardText(SDL_VideoDevice *device);
SDL_bool PYXIS_HasClipboardText(SDL_VideoDevice *device);

#endif /* SDL_pyxisvideo_h_ */
