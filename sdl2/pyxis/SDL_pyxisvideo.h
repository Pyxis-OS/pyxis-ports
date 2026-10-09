#ifndef SDL_pyxisvideo_h_
#define SDL_pyxisvideo_h_

/* Pyxis video driver state shared by the video and event halves. A process
 * has one display grant, so there is one window and one set of sessions. */

#include <display.h>

#include "SDL_internal.h"
#include "video/SDL_sysvideo.h"

struct pyxis_video {
  handle_t display;
  handle_t keyboard;
  handle_t pointer; /* HANDLE_INVALID without a pointer grant. */
  handle_t clock;
  SDL_Window *window; /* The only window, or NULL. */
  /* Input sessions belong to the window. Keyboard starts at window creation;
   * pointer starts after graphics acquisition. Destruction releases both. */
  bool keyboard_owned;
  bool pointer_owned;
  /* The display session starts with the first window framebuffer. */
  bool display_owned;
  bool presented;
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

#endif /* SDL_pyxisvideo_h_ */
