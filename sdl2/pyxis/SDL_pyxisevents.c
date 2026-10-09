/* Pyxis keyboard and pointer input for SDL. Focus changes and input resets
 * release every held key and button: a key held across them must be pressed
 * again, as the Pyxis input sessions require. */

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <clock.h>
#include <clipboard.h>
#include <keyboard.h>
#include <pointer.h>
#include <pxe/key_layout.h>
#include <wait.h>

#include "SDL_pyxisvideo.h"
#include "events/SDL_keyboard_c.h"
#include "events/SDL_mouse_c.h"

#define ASCII_FIRST_PRINTABLE ' '
#define ASCII_LAST_PRINTABLE '~'
#define NANOSECONDS_PER_MILLISECOND UINT64_C(1000000)
#define INPUT_WAIT_CAPACITY 3

static bool keyboard_focused;
static bool shared_c_held;
static bool shared_v_held;
static bool pointer_focused;
static bool pointer_inside;
static uint32_t held_buttons; /* Pyxis POINTER_BUTTON_* bits SDL has pressed. */
/* The threadless SDL loop pumps immediately after the wait hook. Consume its
 * observation once, avoiding a second BSP handoff before draining input. */
static bool waited;
static uint64_t waited_events[INPUT_WAIT_CAPACITY];

static const SDL_Scancode scancodes[KEY_COUNT] = {
  [KEY_ESCAPE] = SDL_SCANCODE_ESCAPE,
  [KEY_F1] = SDL_SCANCODE_F1, [KEY_F2] = SDL_SCANCODE_F2, [KEY_F3] = SDL_SCANCODE_F3,
  [KEY_F4] = SDL_SCANCODE_F4, [KEY_F5] = SDL_SCANCODE_F5, [KEY_F6] = SDL_SCANCODE_F6,
  [KEY_F7] = SDL_SCANCODE_F7, [KEY_F8] = SDL_SCANCODE_F8, [KEY_F9] = SDL_SCANCODE_F9,
  [KEY_F10] = SDL_SCANCODE_F10, [KEY_F11] = SDL_SCANCODE_F11, [KEY_F12] = SDL_SCANCODE_F12,
  [KEY_GRAVE] = SDL_SCANCODE_GRAVE,
  [KEY_1] = SDL_SCANCODE_1, [KEY_2] = SDL_SCANCODE_2, [KEY_3] = SDL_SCANCODE_3,
  [KEY_4] = SDL_SCANCODE_4, [KEY_5] = SDL_SCANCODE_5, [KEY_6] = SDL_SCANCODE_6,
  [KEY_7] = SDL_SCANCODE_7, [KEY_8] = SDL_SCANCODE_8, [KEY_9] = SDL_SCANCODE_9,
  [KEY_0] = SDL_SCANCODE_0, [KEY_MINUS] = SDL_SCANCODE_MINUS, [KEY_EQUAL] = SDL_SCANCODE_EQUALS,
  [KEY_BACKSPACE] = SDL_SCANCODE_BACKSPACE, [KEY_TAB] = SDL_SCANCODE_TAB,
  [KEY_Q] = SDL_SCANCODE_Q, [KEY_W] = SDL_SCANCODE_W, [KEY_E] = SDL_SCANCODE_E,
  [KEY_R] = SDL_SCANCODE_R, [KEY_T] = SDL_SCANCODE_T, [KEY_Y] = SDL_SCANCODE_Y,
  [KEY_U] = SDL_SCANCODE_U, [KEY_I] = SDL_SCANCODE_I, [KEY_O] = SDL_SCANCODE_O,
  [KEY_P] = SDL_SCANCODE_P, [KEY_LEFT_BRACKET] = SDL_SCANCODE_LEFTBRACKET,
  [KEY_RIGHT_BRACKET] = SDL_SCANCODE_RIGHTBRACKET, [KEY_BACKSLASH] = SDL_SCANCODE_BACKSLASH,
  [KEY_CAPS_LOCK] = SDL_SCANCODE_CAPSLOCK,
  [KEY_A] = SDL_SCANCODE_A, [KEY_S] = SDL_SCANCODE_S, [KEY_D] = SDL_SCANCODE_D,
  [KEY_F] = SDL_SCANCODE_F, [KEY_G] = SDL_SCANCODE_G, [KEY_H] = SDL_SCANCODE_H,
  [KEY_J] = SDL_SCANCODE_J, [KEY_K] = SDL_SCANCODE_K, [KEY_L] = SDL_SCANCODE_L,
  [KEY_SEMICOLON] = SDL_SCANCODE_SEMICOLON, [KEY_APOSTROPHE] = SDL_SCANCODE_APOSTROPHE,
  [KEY_ENTER] = SDL_SCANCODE_RETURN, [KEY_LEFT_SHIFT] = SDL_SCANCODE_LSHIFT,
  [KEY_NON_US_BACKSLASH] = SDL_SCANCODE_NONUSBACKSLASH,
  [KEY_Z] = SDL_SCANCODE_Z, [KEY_X] = SDL_SCANCODE_X, [KEY_C] = SDL_SCANCODE_C,
  [KEY_V] = SDL_SCANCODE_V, [KEY_B] = SDL_SCANCODE_B, [KEY_N] = SDL_SCANCODE_N,
  [KEY_M] = SDL_SCANCODE_M, [KEY_COMMA] = SDL_SCANCODE_COMMA, [KEY_PERIOD] = SDL_SCANCODE_PERIOD,
  [KEY_SLASH] = SDL_SCANCODE_SLASH, [KEY_RIGHT_SHIFT] = SDL_SCANCODE_RSHIFT,
  [KEY_LEFT_CONTROL] = SDL_SCANCODE_LCTRL, [KEY_LEFT_SUPER] = SDL_SCANCODE_LGUI,
  [KEY_LEFT_ALT] = SDL_SCANCODE_LALT, [KEY_SPACE] = SDL_SCANCODE_SPACE,
  [KEY_RIGHT_ALT] = SDL_SCANCODE_RALT, [KEY_RIGHT_SUPER] = SDL_SCANCODE_RGUI,
  [KEY_MENU] = SDL_SCANCODE_APPLICATION, [KEY_RIGHT_CONTROL] = SDL_SCANCODE_RCTRL,
  [KEY_PRINT_SCREEN] = SDL_SCANCODE_PRINTSCREEN, [KEY_SCROLL_LOCK] = SDL_SCANCODE_SCROLLLOCK,
  [KEY_PAUSE] = SDL_SCANCODE_PAUSE, [KEY_INSERT] = SDL_SCANCODE_INSERT,
  [KEY_HOME] = SDL_SCANCODE_HOME, [KEY_PAGE_UP] = SDL_SCANCODE_PAGEUP,
  [KEY_DELETE] = SDL_SCANCODE_DELETE, [KEY_END] = SDL_SCANCODE_END,
  [KEY_PAGE_DOWN] = SDL_SCANCODE_PAGEDOWN, [KEY_UP] = SDL_SCANCODE_UP,
  [KEY_LEFT] = SDL_SCANCODE_LEFT, [KEY_DOWN] = SDL_SCANCODE_DOWN, [KEY_RIGHT] = SDL_SCANCODE_RIGHT,
  [KEY_NUM_LOCK] = SDL_SCANCODE_NUMLOCKCLEAR, [KEY_KP_DIVIDE] = SDL_SCANCODE_KP_DIVIDE,
  [KEY_KP_MULTIPLY] = SDL_SCANCODE_KP_MULTIPLY, [KEY_KP_MINUS] = SDL_SCANCODE_KP_MINUS,
  [KEY_KP_PLUS] = SDL_SCANCODE_KP_PLUS, [KEY_KP_0] = SDL_SCANCODE_KP_0,
  [KEY_KP_1] = SDL_SCANCODE_KP_1, [KEY_KP_2] = SDL_SCANCODE_KP_2, [KEY_KP_3] = SDL_SCANCODE_KP_3,
  [KEY_KP_4] = SDL_SCANCODE_KP_4, [KEY_KP_5] = SDL_SCANCODE_KP_5, [KEY_KP_6] = SDL_SCANCODE_KP_6,
  [KEY_KP_7] = SDL_SCANCODE_KP_7, [KEY_KP_8] = SDL_SCANCODE_KP_8, [KEY_KP_9] = SDL_SCANCODE_KP_9,
  [KEY_KP_PERIOD] = SDL_SCANCODE_KP_PERIOD, [KEY_KP_ENTER] = SDL_SCANCODE_KP_ENTER,
};

static const struct {
  uint32_t pyxis;
  Uint8 sdl;
} buttons[] = {
  {POINTER_BUTTON_LEFT, SDL_BUTTON_LEFT},
  {POINTER_BUTTON_RIGHT, SDL_BUTTON_RIGHT},
  {POINTER_BUTTON_MIDDLE, SDL_BUTTON_MIDDLE},
};

static void release_buttons(void)
{
  for (size_t i = 0; i < SDL_arraysize(buttons); ++i) {
    if (held_buttons & buttons[i].pyxis) {
      SDL_SendMouseButton(pyxis_video.window, 0, SDL_RELEASED, buttons[i].sdl);
    }
  }
  held_buttons = 0;
}

void PYXIS_ResetInput(void)
{
  waited = false;
  PYXIS_CancelClipboard();
  shared_c_held = shared_v_held = false;
  SDL_ResetKeyboard();
  release_buttons();
  keyboard_focused = false;
  pointer_focused = false;
  pointer_inside = false;
  SDL_Mouse *mouse = SDL_GetMouse();
  mouse->relative_mode = SDL_FALSE;
  mouse->relative_mode_warp = SDL_FALSE;
  mouse->xdelta = mouse->ydelta = 0;
  mouse->scale_accum_x = mouse->scale_accum_y = 0.0f;
  mouse->has_position = SDL_FALSE;
  SDL_FlushEvent(SDL_MOUSEMOTION);
}

void PYXIS_InitInput(void)
{
  PYXIS_InitMouse();
}

static void send_text(const struct keyboard_event *event)
{
  if (!SDL_IsTextInputActive() ||
      (event->modifiers & (KEY_MOD_CONTROL | KEY_MOD_ALT | KEY_MOD_SUPER))) {
    return;
  }
  char character = key_layout_character(event->key, event->modifiers);
  if (character >= ASCII_FIRST_PRINTABLE && character <= ASCII_LAST_PRINTABLE) {
    char text[2] = {character, '\0'};
    SDL_SendKeyboardText(text);
  }
}

static void handle_key(const struct keyboard_event *event)
{
  if (event->action == KEY_FOCUS_GAINED || event->action == KEY_FOCUS_LOST ||
      event->action == KEY_STATE_RESET) {
    PYXIS_CancelClipboard();
    shared_c_held = shared_v_held = false;
    keyboard_focused = (event->flags & KEYBOARD_EVENT_FOCUSED) != 0;
    SDL_ResetKeyboard();
    SDL_SetKeyboardFocus(keyboard_focused ? pyxis_video.window : NULL);
    return;
  }
  if (!keyboard_focused || event->key >= KEY_COUNT || !scancodes[event->key]) {
    return;
  }
  bool *shared_held = event->key == KEY_C ? &shared_c_held :
      event->key == KEY_V ? &shared_v_held : NULL;
  if (shared_held && *shared_held) {
    if (event->action == KEY_RELEASE) {
      *shared_held = false;
    }
    return;
  }
  bool shared_command = (event->key == KEY_C || event->key == KEY_V) &&
      (event->modifiers & (KEY_MOD_CONTROL | KEY_MOD_ALT | KEY_MOD_SUPER | KEY_MOD_SHIFT)) ==
      (KEY_MOD_SUPER | KEY_MOD_SHIFT);
  if (shared_command) {
    if (event->action != KEY_PRESS) {
      return;
    }
    *shared_held = true;
    if (!event->clipboard_action_id) {
      return;
    }
  }
  bool pressed = event->action != KEY_RELEASE;
  if (event->clipboard_action_id && event->action == KEY_PRESS) {
    bool armed = PYXIS_QueueClipboard(event->clipboard_action_id,
        event->clipboard_operation, event->clipboard_layer);
    if (!armed && event->clipboard_layer == CLIPBOARD_LAYER_SHARED) {
      return;
    }
    int posted = SDL_SendKeyboardKeyWithClipboard(SDL_PRESSED, scancodes[event->key],
        armed ? event->clipboard_action_id : 0,
        event->clipboard_layer == CLIPBOARD_LAYER_SHARED);
    if (!posted) {
      PYXIS_CancelClipboard();
    }
  } else {
    SDL_SendKeyboardKey(pressed ? SDL_PRESSED : SDL_RELEASED, scancodes[event->key]);
  }
  if (pressed) {
    send_text(event);
  }
}

static void update_pointer_state(uint64_t flags, bool reset)
{
  SDL_Mouse *mouse = SDL_GetMouse();
  bool locked = (flags & POINTER_EVENT_LOCKED) != 0;
  if (reset || (!locked && mouse->relative_mode)) {
    release_buttons();
    mouse->xdelta = mouse->ydelta = 0;
    mouse->scale_accum_x = mouse->scale_accum_y = 0.0f;
    mouse->has_position = SDL_FALSE;
    SDL_FlushEvent(SDL_MOUSEMOTION);
    SDL_FlushEvent(SDL_MOUSEWHEEL);
  }
  if (!locked && mouse->relative_mode) {
    /* The kernel already revoked it. SDL's setter would warp on exit. */
    mouse->relative_mode = SDL_FALSE;
    mouse->relative_mode_warp = SDL_FALSE;
    SDL_UpdateWindowGrab(pyxis_video.window);
    SDL_SetCursor(NULL);
  }
  pointer_focused = (flags & POINTER_EVENT_FOCUSED) != 0;
  SDL_SetMouseFocus(pointer_focused && (pointer_inside || locked) ? pyxis_video.window : NULL);
}

static bool ordinary_geometry(const struct pointer_event *event,
    struct pointer_geometry *geometry)
{
  return pointer_geometry(pyxis_video.pointer, geometry) == CALL_OK &&
      event->generation == geometry->generation &&
      event->mapping_identity == geometry->mapping_identity &&
      geometry->generation == pyxis_video.generation &&
      geometry->mapping_width == pyxis_video.buffer.width &&
      geometry->mapping_height == pyxis_video.buffer.height &&
      event->x >= SDL_MIN_SINT32 && event->x <= SDL_MAX_SINT32 &&
      event->y >= SDL_MIN_SINT32 && event->y <= SDL_MAX_SINT32;
}

static void send_ordinary_position(const struct pointer_event *event)
{
  SDL_SendMouseMotion(pointer_inside ? pyxis_video.window : NULL, 0, SDL_FALSE,
      (int)event->x, (int)event->y);
}

static void restore_ordinary_position(const struct pointer_event *event)
{
  struct pointer_geometry geometry;
  if (!pointer_focused || !ordinary_geometry(event, &geometry) ||
      event->x < 0 || event->y < 0 ||
      (uint64_t)event->x >= SDL_min(geometry.width, geometry.mapping_width) ||
      (uint64_t)event->y >= SDL_min(geometry.height, geometry.mapping_height)) {
    return;
  }
  /* State events carry the native parked position. Restore it even when no
   * device movement follows; ENTER separately establishes ordinary hover. */
  send_ordinary_position(event);
}

static void handle_pointer(const struct pointer_event *event)
{
  bool locked = (event->flags & POINTER_EVENT_LOCKED) != 0;
  if (!locked && (event->type == POINTER_ENTER || event->type == POINTER_ACTIVATED)) {
    pointer_inside = true;
  } else if (event->type == POINTER_LEAVE || event->type == POINTER_FOCUS_LOST ||
      (!locked && event->type == POINTER_LOCK_CHANGED)) {
    pointer_inside = false;
  }
  if (event->type != POINTER_INPUT) {
    bool keep_buttons = locked && event->type == POINTER_GEOMETRY_CHANGED;
    update_pointer_state(event->flags, !keep_buttons);
    if (!locked && (event->type == POINTER_ENTER ||
        event->type == POINTER_GEOMETRY_CHANGED || event->type == POINTER_LOCK_CHANGED ||
        event->type == POINTER_STATE_RESET || event->type == POINTER_ACTIVATED)) {
      restore_ordinary_position(event);
    }
    return;
  }
  update_pointer_state(event->flags, false);
  if (!pointer_focused) {
    return;
  }
  if (locked) {
    if (!SDL_GetRelativeMouseMode()) {
      return;
    }
    if (event->dx || event->dy) {
      SDL_SendMouseMotion(pyxis_video.window, 0, SDL_TRUE, event->dx, event->dy);
    }
  } else {
    struct pointer_geometry geometry;
    if (!ordinary_geometry(event, &geometry)) {
      return;
    }
    pointer_inside = true;
    SDL_SetMouseFocus(pyxis_video.window);
    send_ordinary_position(event);
  }
  for (size_t i = 0; i < SDL_arraysize(buttons); ++i) {
    uint32_t button = buttons[i].pyxis;
    if ((event->buttons ^ held_buttons) & button) {
      bool pressed = (event->buttons & button) != 0;
      SDL_SendMouseButton(pyxis_video.window, 0, pressed ? SDL_PRESSED : SDL_RELEASED,
          buttons[i].sdl);
      held_buttons ^= button;
    }
  }
  /* Pyxis counts toward the user as positive; SDL counts away from the user. */
  if (event->wheel) {
    SDL_SendMouseWheel(pyxis_video.window, 0, 0.0f, -(float)event->wheel,
        SDL_MOUSEWHEEL_NORMAL);
  }
}

static void drain_keyboard(void)
{
  struct keyboard_event event;
  while (keyboard_read(pyxis_video.keyboard, KEYBOARD_READ_POLL, &event) == CALL_OK) {
    handle_key(&event);
  }
}

static size_t input_interests(struct wait_interest interests[INPUT_WAIT_CAPACITY])
{
  size_t count = 0;
  interests[count++] = (struct wait_interest){
    .handle = pyxis_video.display, .events = WAIT_RESIZED,
    .observed_generation = pyxis_video.generation,
  };
  if (pyxis_video.keyboard_owned) {
    interests[count++] = (struct wait_interest){
      .handle = pyxis_video.keyboard, .events = WAIT_READABLE,
    };
  }
  if (pyxis_video.pointer_owned) {
    interests[count++] = (struct wait_interest){
      .handle = pyxis_video.pointer, .events = WAIT_READABLE,
    };
  }
  return count;
}

int PYXIS_WaitEventTimeout(SDL_VideoDevice *device, int timeout)
{
  (void)device;
  if (!pyxis_video.window || !pyxis_video.keyboard_owned) {
    return -1;
  }

  struct wait_interest interests[INPUT_WAIT_CAPACITY];
  size_t count = input_interests(interests);
  uint64_t end = 0;
  if (timeout > 0) {
    uint64_t now;
    uint64_t duration = (uint64_t)timeout * NANOSECONDS_PER_MILLISECOND;
    if (clock_now(pyxis_video.clock, &now) != CALL_OK || now > UINT64_MAX - duration) {
      return -1;
    }
    end = now + duration;
  }

  for (;;) {
    uint64_t deadline = 0;
    if (timeout != 0) {
      uint64_t now;
      if (clock_now(pyxis_video.clock, &now) != CALL_OK ||
          now > UINT64_MAX - WAIT_MAX_WAIT_NS) {
        return -1;
      }
      deadline = now + WAIT_MAX_WAIT_NS;
      if (timeout > 0 && end < deadline) {
        deadline = end;
      }
    }

    uint64_t events[INPUT_WAIT_CAPACITY] = {0};
    enum call_status status = wait_many(interests, count, deadline, events);
    if (status == CALL_OK) {
      bool ready = false;
      for (size_t i = 0; i < count; ++i) {
        if (events[i] & WAIT_ERROR) {
          return -1; /* Ownership/backend loss needs the polling fallback. */
        }
        ready |= (events[i] & interests[i].events) != 0;
      }
      if (ready) {
        SDL_memcpy(waited_events, events, sizeof(waited_events));
        waited = true;
      }
      /* SDL pumps again before taking an event from its queue. */
      return ready ? 1 : (timeout == 0 ? 0 : -1);
    }
    if (status != CALL_TIMED_OUT) {
      return -1;
    }
    if (timeout > 0 && deadline == end) {
      return 0;
    }
    /* Native waits are bounded to 30 seconds. Re-arm long/infinite waits;
     * each call checks readiness before interpreting an expired deadline. */
  }
}

void PYXIS_PumpEvents(SDL_VideoDevice *device)
{
  if (!pyxis_video.window) {
    return;
  }

  struct wait_interest interests[INPUT_WAIT_CAPACITY];
  size_t count = input_interests(interests);
  uint64_t events[INPUT_WAIT_CAPACITY] = {0};
  bool pointer_ready = true;
  enum call_status status;
  if (waited) {
    SDL_memcpy(events, waited_events, sizeof(events));
    waited = false;
    status = CALL_OK;
  } else {
    status = wait_many(interests, count, 0, events);
  }
  if (status == CALL_OK) {
    if (events[0] & WAIT_RESIZED) {
      PYXIS_FollowDisplaySize(device);
    }
    if (pyxis_video.keyboard_owned && (events[1] & (WAIT_READABLE | WAIT_ERROR))) {
      drain_keyboard();
    }
    pointer_ready = pyxis_video.pointer_owned &&
        (events[count - 1] & (WAIT_READABLE | WAIT_ERROR));
  } else {
    if (pyxis_video.keyboard_owned) {
      drain_keyboard();
    }
  }

  struct pointer_event motion;
  while (pointer_ready && pyxis_video.pointer_owned &&
         pointer_read(pyxis_video.pointer, POINTER_READ_POLL, &motion) == CALL_OK) {
    handle_pointer(&motion);
  }
  uint64_t flags;
  if (pyxis_video.pointer_owned && pointer_state(pyxis_video.pointer, &flags) == CALL_OK) {
    update_pointer_state(flags, false);
  }
}

#endif /* SDL_VIDEO_DRIVER_PYXIS */
