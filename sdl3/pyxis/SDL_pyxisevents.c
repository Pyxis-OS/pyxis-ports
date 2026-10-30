/* Pyxis keyboard and pointer input for SDL. Focus changes and input resets
 * release every held key and button: a key held across them must be pressed
 * again, as the Pyxis input sessions require. */

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <clock.h>
#include <environment.h>
#include <handle.h>
#include <keyboard.h>
#include <pipe.h>
#include <pointer.h>
#include <pxe/key_layout.h>
#include <startup.h>
#include <wait.h>

#include "SDL_pyxisvideo.h"
#include "events/SDL_events_c.h"
#include "events/SDL_keyboard_c.h"
#include "events/SDL_mouse_c.h"

#define ASCII_FIRST_PRINTABLE ' '
#define ASCII_LAST_PRINTABLE '~'
#define INPUT_WAIT_CAPACITY 4
#define INPUT_WAIT_ABSENT SIZE_MAX
#define WAKEUP_DRAIN_BYTES 64
#define POINTER_BUTTON_COUNT 32

static bool keyboard_focused;
static bool pointer_focused;
static bool pointer_inside;
static uint32_t held_buttons; /* Pyxis POINTER_BUTTON_* bits SDL has pressed. */
/* SDL's wait loop pumps immediately after the wait hook, on the same thread
 * and with the same sessions. Consume its observation once, avoiding a second
 * BSP handoff before draining input. */
static bool waited;
static uint64_t waited_events[INPUT_WAIT_CAPACITY];

/* Other threads post events through SDL's queue. With threads authority a
 * blocking wait also watches a private pipe that SendWakeupEvent writes to.
 * Without it no other thread can exist, so waits need no wakeup. */
static bool wakeup_needed;
static handle_t wakeup_reader = HANDLE_INVALID;
static handle_t wakeup_writer = HANDLE_INVALID;

struct input_wait {
  struct wait_interest interests[INPUT_WAIT_CAPACITY];
  size_t count;
  size_t keyboard; /* Interest indices, or INPUT_WAIT_ABSENT. */
  size_t pointer;
  size_t wakeup;
};

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

static Uint8 sdl_button(unsigned usage)
{
  switch (usage) {
    case 1: return SDL_BUTTON_LEFT;
    case 2: return SDL_BUTTON_RIGHT;
    case 3: return SDL_BUTTON_MIDDLE;
    default: return (Uint8)usage;
  }
}

static void release_buttons(void)
{
  for (unsigned usage = 1; usage <= POINTER_BUTTON_COUNT; ++usage) {
    if (held_buttons & (UINT32_C(1) << (usage - 1))) {
      SDL_SendMouseButton(0, pyxis_video.window, SDL_DEFAULT_MOUSE_ID, sdl_button(usage), false);
    }
  }
  held_buttons = 0;
}

static void reset_scroll(void)
{
  SDL_Mouse *mouse = SDL_GetMouse();
  mouse->residual_scroll_x = mouse->residual_scroll_y = 0.0f;
  SDL_FlushEvent(SDL_EVENT_MOUSE_WHEEL);
}

static void reset_motion(void)
{
  SDL_Mouse *mouse = SDL_GetMouse();
  mouse->x_accu = mouse->y_accu = 0.0f;
  mouse->has_position = false;
  SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
}

void PYXIS_ResetInput(void)
{
  waited = false;
  SDL_ResetKeyboard();
  release_buttons();
  reset_scroll();
  keyboard_focused = false;
  pointer_focused = false;
  pointer_inside = false;
  SDL_GetMouse()->relative_mode = false;
  reset_motion();
}

static void send_text(const struct keyboard_event *event)
{
  if (!SDL_TextInputActive(pyxis_video.window) ||
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
    keyboard_focused = (event->flags & KEYBOARD_EVENT_FOCUSED) != 0;
    SDL_ResetKeyboard();
    SDL_SetKeyboardFocus(keyboard_focused ? pyxis_video.window : NULL);
    return;
  }
  if (!keyboard_focused || event->key >= KEY_COUNT || !scancodes[event->key]) {
    return;
  }
  bool pressed = event->action != KEY_RELEASE;
  SDL_SendKeyboardKey(0, SDL_DEFAULT_KEYBOARD_ID, (int)event->key, scancodes[event->key], pressed);
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
    reset_motion();
    reset_scroll();
  }
  if (!locked && mouse->relative_mode) {
    /* The kernel already revoked it. SDL's setter would warp on exit. */
    mouse->relative_mode = false;
    SDL_UpdateWindowGrab(pyxis_video.window);
    SDL_SetCursor(NULL);
  }
  pointer_focused = (flags & POINTER_EVENT_FOCUSED) != 0;
  SDL_SetMouseFocus(pointer_focused && (pointer_inside || locked) ? pyxis_video.window : NULL);
}

struct pointer_geometry_snapshot {
  struct pointer_geometry value;
  uint64_t event_generation, event_mapping_identity;
  bool attempted, available;
};

static bool ordinary_geometry(const struct pointer_event *event,
    struct pointer_geometry_snapshot *snapshot)
{
  if (!snapshot->attempted || snapshot->event_generation != event->generation ||
      snapshot->event_mapping_identity != event->mapping_identity) {
    snapshot->attempted = true;
    snapshot->event_generation = event->generation;
    snapshot->event_mapping_identity = event->mapping_identity;
    /* GEOMETRY parks through the BSP. Reuse its snapshot within this pump/key,
     * including a refused query, but validate every event against it below. */
    snapshot->available = pointer_geometry(pyxis_video.pointer, &snapshot->value) == CALL_OK;
  }
  const struct pointer_geometry *geometry = &snapshot->value;
  return snapshot->available &&
      event->generation == geometry->generation &&
      event->mapping_identity == geometry->mapping_identity &&
      PYXIS_PointerGeometry(geometry->generation, geometry->mapping_width,
          geometry->mapping_height) &&
      event->x >= SDL_MIN_SINT32 && event->x <= SDL_MAX_SINT32 &&
      event->y >= SDL_MIN_SINT32 && event->y <= SDL_MAX_SINT32;
}

static void send_ordinary_position(const struct pointer_event *event)
{
  float x = (float)event->x, y = (float)event->y;
  PYXIS_MapPointer(&x, &y, false);
  SDL_SendMouseMotion(0, pointer_inside ? pyxis_video.window : NULL, SDL_DEFAULT_MOUSE_ID,
      false, x, y);
}

static void restore_ordinary_position(const struct pointer_event *event,
    struct pointer_geometry_snapshot *snapshot)
{
  if (!pointer_focused || !ordinary_geometry(event, snapshot) ||
      event->x < 0 || event->y < 0 ||
      (uint64_t)event->x >= SDL_min(snapshot->value.width, snapshot->value.mapping_width) ||
      (uint64_t)event->y >= SDL_min(snapshot->value.height, snapshot->value.mapping_height)) {
    return;
  }
  /* State events carry the native parked position. Restore it even when no
   * device movement follows; ENTER separately establishes ordinary hover. */
  send_ordinary_position(event);
}

static void handle_pointer(const struct pointer_event *event,
    struct pointer_geometry_snapshot *snapshot)
{
  bool locked = (event->flags & POINTER_EVENT_LOCKED) != 0;
  if (!locked && (event->type == POINTER_ENTER || event->type == POINTER_ACTIVATED)) {
    pointer_inside = true;
  } else if (event->type == POINTER_LEAVE || event->type == POINTER_FOCUS_LOST ||
      (!locked && event->type == POINTER_LOCK_CHANGED)) {
    pointer_inside = false;
  }
  if (event->type != POINTER_INPUT) {
    snapshot->attempted = false;
    reset_scroll();
    bool keep_buttons = locked && event->type == POINTER_GEOMETRY_CHANGED;
    update_pointer_state(event->flags, !keep_buttons);
    if (!locked && (event->type == POINTER_ENTER ||
        event->type == POINTER_GEOMETRY_CHANGED || event->type == POINTER_LOCK_CHANGED ||
        event->type == POINTER_STATE_RESET || event->type == POINTER_ACTIVATED)) {
      restore_ordinary_position(event, snapshot);
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
      SDL_SendMouseMotion(0, pyxis_video.window, SDL_DEFAULT_MOUSE_ID, true,
          (float)event->dx, (float)event->dy);
    }
  } else {
    if (!ordinary_geometry(event, snapshot)) {
      reset_scroll();
      return;
    }
    pointer_inside = true;
    SDL_SetMouseFocus(pyxis_video.window);
    send_ordinary_position(event);
  }
  for (unsigned usage = 1; usage <= POINTER_BUTTON_COUNT; ++usage) {
    uint32_t button = UINT32_C(1) << (usage - 1);
    if ((event->buttons ^ held_buttons) & button) {
      bool pressed = (event->buttons & button) != 0;
      SDL_SendMouseButton(0, pyxis_video.window, SDL_DEFAULT_MOUSE_ID, sdl_button(usage), pressed);
      held_buttons ^= button;
    }
  }
  /* Direction policy is already applied; SDL's vertical axis is positive up. */
  if (event->scroll_x || event->scroll_y) {
    SDL_SendMouseWheel(0, pyxis_video.window, SDL_DEFAULT_MOUSE_ID,
        (float)event->scroll_x / POINTER_SCROLL_UNITS_PER_DETENT,
        -(float)event->scroll_y / POINTER_SCROLL_UNITS_PER_DETENT,
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

static size_t add_interest(struct input_wait *wait, handle_t handle, uint64_t events)
{
  wait->interests[wait->count] = (struct wait_interest){.handle = handle, .events = events};
  return wait->count++;
}

/* The display is always interest 0. */
static void input_interests(struct input_wait *wait)
{
  wait->count = 0;
  wait->keyboard = wait->pointer = wait->wakeup = INPUT_WAIT_ABSENT;
  add_interest(wait, pyxis_video.display, WAIT_RESIZED);
  wait->interests[0].observed_generation = pyxis_video.generation;
  if (pyxis_video.keyboard_owned) {
    wait->keyboard = add_interest(wait, pyxis_video.keyboard, WAIT_READABLE);
  }
  if (pyxis_video.pointer_owned) {
    wait->pointer = add_interest(wait, pyxis_video.pointer, WAIT_READABLE);
  }
  if (wakeup_reader != HANDLE_INVALID) {
    wait->wakeup = add_interest(wait, wakeup_reader, WAIT_READABLE);
  }
}

static bool interest_ready(const uint64_t events[], size_t index)
{
  return index != INPUT_WAIT_ABSENT && (events[index] & (WAIT_READABLE | WAIT_ERROR));
}

static void drain_wakeup(void)
{
  uint8_t bytes[WAKEUP_DRAIN_BYTES];
  size_t read;
  while (pipe_try_read(wakeup_reader, bytes, sizeof(bytes), &read) == CALL_OK && read > 0) {
  }
}

/* The pipe is created once and kept until the process exits, even across
 * SDL_QuitSubSystem: a sender racing a close could otherwise write to a
 * reused handle number. Failure leaves waits on SDL's polling fallback. */
void PYXIS_InitWakeup(void)
{
  handle_t threads = HANDLE_INVALID;
  wakeup_needed = environment_capability(startup_environment_handle(), "threads", &threads) == CALL_OK;
  if (threads != HANDLE_INVALID) {
    handle_close(threads);
  }
  if (!wakeup_needed || wakeup_writer != HANDLE_INVALID) {
    return;
  }
  handle_t service = HANDLE_INVALID;
  if (environment_capability(startup_environment_handle(), "pipe", &service) != CALL_OK) {
    return;
  }
  struct pipe_create_reply pair;
  if (pipe_create(service, &pair) == CALL_OK) {
    wakeup_reader = pair.reader;
    wakeup_writer = pair.writer;
  }
  handle_close(service);
}

/* Called from any thread, at most once per wait, after SDL has queued an
 * event. A full pipe already holds a pending wakeup, so the result is unused. */
void PYXIS_SendWakeupEvent(SDL_VideoDevice *device, SDL_Window *window)
{
  (void)device;
  (void)window;
  if (wakeup_writer == HANDLE_INVALID) {
    return;
  }
  uint8_t byte = 0;
  size_t written;
  pipe_try_write(wakeup_writer, &byte, sizeof(byte), &written);
}

/* Negative timeouts wait indefinitely and zero polls. Returns 1 when input or
 * a wakeup is ready, 0 on timeout and -1 for SDL's polling fallback. */
int PYXIS_WaitEventTimeout(SDL_VideoDevice *device, Sint64 timeout)
{
  (void)device;
  if (!pyxis_video.window || !pyxis_video.keyboard_owned) {
    return -1;
  }
  if (wakeup_needed && wakeup_reader == HANDLE_INVALID) {
    return -1; /* Another thread's event could not end the wait. */
  }
  struct input_wait wait;
  input_interests(&wait);
  uint64_t end = 0;
  if (timeout > 0) {
    uint64_t now;
    if (clock_now(pyxis_video.clock, &now) != CALL_OK || now > UINT64_MAX - (uint64_t)timeout) {
      return -1;
    }
    end = now + (uint64_t)timeout;
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
    /* SDL publishes the wakeup window before calling this hook, and senders
     * queue before taking it. An event queued before publication is visible
     * here; a later one writes the pipe. */
    if (wakeup_reader != HANDLE_INVALID && SDL_HasEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST)) {
      return 1;
    }
    uint64_t events[INPUT_WAIT_CAPACITY] = {0};
    enum call_status status = wait_many(wait.interests, wait.count, deadline, events);
    if (status == CALL_OK) {
      bool ready = false;
      for (size_t i = 0; i < wait.count; ++i) {
        if (events[i] & WAIT_ERROR) {
          return -1; /* Ownership/backend loss needs the polling fallback. */
        }
        ready |= (events[i] & wait.interests[i].events) != 0;
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
  struct input_wait wait;
  input_interests(&wait);
  uint64_t events[INPUT_WAIT_CAPACITY] = {0};
  bool pointer_ready = true;
  enum call_status status;
  if (waited) {
    SDL_memcpy(events, waited_events, sizeof(events));
    waited = false;
    status = CALL_OK;
  } else {
    status = wait_many(wait.interests, wait.count, 0, events);
  }
  if (status == CALL_OK) {
    if (interest_ready(events, wait.wakeup)) {
      drain_wakeup();
    }
    if (events[0] & WAIT_RESIZED) {
      PYXIS_FollowDisplaySize(device);
    }
    if (interest_ready(events, wait.keyboard)) {
      drain_keyboard();
    }
    pointer_ready = interest_ready(events, wait.pointer);
  } else {
    if (wakeup_reader != HANDLE_INVALID) {
      drain_wakeup();
    }
    if (pyxis_video.keyboard_owned) {
      drain_keyboard();
    }
  }
  struct pointer_geometry_snapshot geometry = {0};
  struct pointer_event motion;
  while (pointer_ready && pyxis_video.pointer_owned &&
         pointer_read(pyxis_video.pointer, POINTER_READ_POLL, &motion) == CALL_OK) {
    handle_pointer(&motion, &geometry);
  }
  uint64_t flags;
  if (pyxis_video.pointer_owned && pointer_state(pyxis_video.pointer, &flags) == CALL_OK) {
    update_pointer_state(flags, false);
  }
}

#endif /* SDL_VIDEO_DRIVER_PYXIS */
