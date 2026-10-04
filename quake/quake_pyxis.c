#include <clock.h>
#include <display.h>
#include <keyboard.h>
#include <pointer.h>
#include <startup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quake_pyxis.h"
#include "quakegeneric.h"

#define NANOSECONDS_PER_SECOND 1000000000.0
#define QUAKE_KEY_COUNT 256
#define PENDING_CAPACITY 32
/* One pointer event yields at most three button changes and this many wheel
 * press/release pairs, so a translated event always fits the pending queue. */
#define WHEEL_STEPS_PER_EVENT 8

struct pending_key {
  int key;
  bool down;
};

static handle_t display, keyboard, pointer, clock;
static struct display_buffer buffer;
static bool display_owned, keyboard_owned, pointer_owned;
static unsigned scale, left, top;
static uint32_t palette_pixels[256];

static bool focused;
static uint64_t started_at, paused_at, paused_ns;

static bool held[KEY_COUNT];
static bool quake_down[QUAKE_KEY_COUNT];
static bool releasing_keys;
static struct pending_key pending[PENDING_CAPACITY];
static size_t pending_head, pending_count;
static uint32_t pointer_buttons;
static int64_t motion_x, motion_y;

static const unsigned char keymap[KEY_COUNT] = {
  [KEY_ESCAPE] = K_ESCAPE, [KEY_ENTER] = K_ENTER, [KEY_KP_ENTER] = K_ENTER,
  [KEY_TAB] = K_TAB, [KEY_BACKSPACE] = K_BACKSPACE, [KEY_SPACE] = K_SPACE,
  [KEY_UP] = K_UPARROW, [KEY_DOWN] = K_DOWNARROW,
  [KEY_LEFT] = K_LEFTARROW, [KEY_RIGHT] = K_RIGHTARROW,
  [KEY_LEFT_ALT] = K_ALT, [KEY_RIGHT_ALT] = K_ALT,
  [KEY_LEFT_CONTROL] = K_CTRL, [KEY_RIGHT_CONTROL] = K_CTRL,
  [KEY_LEFT_SHIFT] = K_SHIFT, [KEY_RIGHT_SHIFT] = K_SHIFT,
  [KEY_F1] = K_F1, [KEY_F2] = K_F2, [KEY_F3] = K_F3, [KEY_F4] = K_F4,
  [KEY_F5] = K_F5, [KEY_F6] = K_F6, [KEY_F7] = K_F7, [KEY_F8] = K_F8,
  [KEY_F9] = K_F9, [KEY_F10] = K_F10, [KEY_F11] = K_F11, [KEY_F12] = K_F12,
  [KEY_INSERT] = K_INS, [KEY_DELETE] = K_DEL, [KEY_HOME] = K_HOME, [KEY_END] = K_END,
  [KEY_PAGE_UP] = K_PGUP, [KEY_PAGE_DOWN] = K_PGDN, [KEY_PAUSE] = K_PAUSE,
  [KEY_A] = 'a', [KEY_B] = 'b', [KEY_C] = 'c', [KEY_D] = 'd',
  [KEY_E] = 'e', [KEY_F] = 'f', [KEY_G] = 'g', [KEY_H] = 'h',
  [KEY_I] = 'i', [KEY_J] = 'j', [KEY_K] = 'k', [KEY_L] = 'l',
  [KEY_M] = 'm', [KEY_N] = 'n', [KEY_O] = 'o', [KEY_P] = 'p',
  [KEY_Q] = 'q', [KEY_R] = 'r', [KEY_S] = 's', [KEY_T] = 't',
  [KEY_U] = 'u', [KEY_V] = 'v', [KEY_W] = 'w', [KEY_X] = 'x',
  [KEY_Y] = 'y', [KEY_Z] = 'z',
  [KEY_0] = '0', [KEY_1] = '1', [KEY_2] = '2', [KEY_3] = '3',
  [KEY_4] = '4', [KEY_5] = '5', [KEY_6] = '6', [KEY_7] = '7',
  [KEY_8] = '8', [KEY_9] = '9', [KEY_MINUS] = '-', [KEY_EQUAL] = '=',
  [KEY_COMMA] = ',', [KEY_PERIOD] = '.', [KEY_SLASH] = '/',
  [KEY_SEMICOLON] = ';', [KEY_APOSTROPHE] = '\'', [KEY_GRAVE] = '`',
  [KEY_LEFT_BRACKET] = '[', [KEY_RIGHT_BRACKET] = ']', [KEY_BACKSLASH] = '\\',
  [KEY_KP_PLUS] = '+', [KEY_KP_MINUS] = '-', [KEY_KP_MULTIPLY] = '*',
  [KEY_KP_DIVIDE] = '/',
};

static const struct {
  uint32_t button;
  int key;
} button_keys[] = {
  {POINTER_BUTTON_LEFT, K_MOUSE1},
  {POINTER_BUTTON_RIGHT, K_MOUSE2},
  {POINTER_BUTTON_MIDDLE, K_MOUSE3},
};

void pyxis_quake_stop(void)
{
  if (pointer_owned) {
    enum call_status status = pointer_release(pointer);
    pointer_owned = false;
    if (status != CALL_OK) {
      fprintf(stderr, "quake: pointer release failed (status %u)\n", (unsigned)status);
    }
  }
  if (keyboard_owned) {
    enum call_status status = keyboard_release(keyboard);
    keyboard_owned = false;
    if (status != CALL_OK) {
      fprintf(stderr, "quake: keyboard release failed (status %u)\n", (unsigned)status);
    }
  }
  if (display_owned) {
    enum call_status status = display_release(display);
    display_owned = false;
    if (status != CALL_OK) {
      fprintf(stderr, "quake: display release failed (status %u)\n", (unsigned)status);
    }
  }
}

static void require_ok(enum call_status status, const char *operation)
{
  if (status != CALL_OK) {
    pyxis_quake_stop();
    fprintf(stderr, "quake: %s failed (status %u)\n", operation, (unsigned)status);
    exit(EXIT_FAILURE);
  }
}

static uint64_t now_ns(void)
{
  uint64_t now;
  require_ok(clock_now(clock, &now), "clock read");
  return now;
}

static void set_focus(bool next)
{
  if (focused && !next) {
    paused_at = now_ns();
  } else if (!focused && next) {
    paused_ns += now_ns() - paused_at;
  }
  focused = next;
}

/* Focus changes and resets end every held input, in our state and Quake's. */
static void reset_input(bool next_focus)
{
  set_focus(next_focus);
  memset(held, 0, sizeof(held));
  pointer_buttons = 0;
  motion_x = motion_y = 0;
  pending_head = pending_count = 0;
  releasing_keys = true;
}

static void push_key(int key, bool down)
{
  if (pending_count == PENDING_CAPACITY) {
    return;
  }
  pending[(pending_head + pending_count) % PENDING_CAPACITY] =
      (struct pending_key){.key = key, .down = down};
  ++pending_count;
}

static void handle_key_event(const struct keyboard_event *event)
{
  if (event->action == KEY_FOCUS_GAINED || event->action == KEY_FOCUS_LOST ||
      event->action == KEY_STATE_RESET) {
    reset_input((event->flags & KEYBOARD_EVENT_FOCUSED) != 0);
    return;
  }
  if (event->key >= KEY_COUNT || !keymap[event->key]) {
    return;
  }
  unsigned char key = keymap[event->key];
  if (event->action == KEY_REPEAT) {
    /* Quake counts repeats itself and uses them only for text editing. */
    if (quake_down[key]) {
      push_key(key, true);
    }
    return;
  }
  held[event->key] = event->action == KEY_PRESS;
  /* Left/right modifiers share a Quake key; releasing one must not release
   * the other while it is still held. */
  bool down = false;
  for (unsigned i = 0; i < KEY_COUNT; ++i) {
    down |= held[i] && keymap[i] == key;
  }
  if (quake_down[key] != down) {
    push_key(key, down);
  }
}

static void handle_pointer_event(const struct pointer_event *event)
{
  if (event->type != POINTER_INPUT) {
    reset_input((event->flags & POINTER_EVENT_FOCUSED) != 0);
    return;
  }
  motion_x += event->dx;
  motion_y += event->dy;
  for (size_t i = 0; i < sizeof(button_keys) / sizeof(button_keys[0]); ++i) {
    uint32_t button = button_keys[i].button;
    if ((event->buttons & button) != (pointer_buttons & button)) {
      push_key(button_keys[i].key, (event->buttons & button) != 0);
    }
  }
  pointer_buttons = event->buttons;

  /* Positive wheel counts scroll toward the user, which Quake calls down. */
  int key = event->wheel > 0 ? K_MWHEELDOWN : K_MWHEELUP;
  int64_t steps = event->wheel > 0 ? event->wheel : -(int64_t)event->wheel;
  if (steps > WHEEL_STEPS_PER_EVENT) {
    steps = WHEEL_STEPS_PER_EVENT;
  }
  for (int64_t i = 0; i < steps; ++i) {
    push_key(key, true);
    push_key(key, false);
  }
}

static bool read_keyboard(void)
{
  struct keyboard_event event;
  enum call_status status = keyboard_read(keyboard, KEYBOARD_READ_POLL, &event);
  if (status == CALL_TIMED_OUT) {
    return false;
  }
  require_ok(status, "keyboard read");
  handle_key_event(&event);
  return true;
}

static bool read_pointer(void)
{
  if (!pointer_owned) {
    return false;
  }
  struct pointer_event event;
  enum call_status status = pointer_read(pointer, POINTER_READ_POLL, &event);
  if (status == CALL_TIMED_OUT) {
    return false;
  }
  require_ok(status, "pointer read");
  handle_pointer_event(&event);
  return true;
}

void pyxis_quake_start(void)
{
  display = startup_resource("display");
  keyboard = startup_resource("keyboard");
  pointer = startup_resource("pointer");
  clock = startup_resource("clock");
  if (display == HANDLE_INVALID || keyboard == HANDLE_INVALID || clock == HANDLE_INVALID) {
    fputs("quake: display, keyboard and clock resources are required\n", stderr);
    exit(EXIT_FAILURE);
  }

  require_ok(display_acquire(display, &buffer), "display acquisition");
  display_owned = true;
  require_ok(keyboard_acquire(keyboard), "keyboard acquisition");
  keyboard_owned = true;
  /* The game remains playable from the keyboard without a mouse. */
  if (pointer == HANDLE_INVALID) {
    fputs("quake: no pointer resource; mouse input disabled\n", stderr);
  } else {
    enum call_status status = pointer_acquire(pointer);
    if (status == CALL_UNAVAILABLE) {
      fputs("quake: no mouse is available; mouse input disabled\n", stderr);
    } else {
      require_ok(status, "pointer acquisition");
      pointer_owned = true;
    }
  }

  scale = buffer.width / QUAKEGENERIC_RES_X;
  if (scale > buffer.height / QUAKEGENERIC_RES_Y) {
    scale = buffer.height / QUAKEGENERIC_RES_Y;
  }
  if (scale == 0) {
    pyxis_quake_stop();
    fputs("quake: display is smaller than the game frame\n", stderr);
    exit(EXIT_FAILURE);
  }
  left = (buffer.width - QUAKEGENERIC_RES_X * scale) / 2;
  top = (buffer.height - QUAKEGENERIC_RES_Y * scale) / 2;

  /* Acquisition queues the initial focus state; an inactive start waits here
   * so game time begins when the space is first shown. */
  while (!focused) {
    struct keyboard_event event;
    require_ok(keyboard_read(keyboard, 0, &event), "initial keyboard focus");
    focused = (event.flags & KEYBOARD_EVENT_FOCUSED) != 0;
  }
  started_at = paused_at = now_ns();
  require_ok(display_present(display), "display presentation");
}

double pyxis_quake_time(void)
{
  uint64_t now = focused ? now_ns() : paused_at;
  return (double)(now - started_at - paused_ns) / NANOSECONDS_PER_SECOND;
}

void pyxis_quake_wait_focus(void)
{
  while (!focused) {
    struct keyboard_event event;
    require_ok(keyboard_read(keyboard, 0, &event), "keyboard read");
    handle_key_event(&event);
  }
}

void pyxis_quake_sleep_until(double seconds)
{
  if (seconds <= 0) {
    return;
  }
  uint64_t deadline = started_at + paused_ns + (uint64_t)(seconds * NANOSECONDS_PER_SECOND);
  require_ok(clock_sleep_until(clock, deadline), "sleep");
}

void QG_Init(void)
{
}

void QG_Quit(void)
{
}

void QG_SetPalette(unsigned char palette[768])
{
  for (unsigned i = 0; i < 256; ++i) {
    palette_pixels[i] = (uint32_t)palette[i * 3] << buffer.red_shift |
        (uint32_t)palette[i * 3 + 1] << buffer.green_shift |
        (uint32_t)palette[i * 3 + 2] << buffer.blue_shift;
  }
}

void QG_DrawFrame(void *pixels)
{
  const unsigned char *frame = pixels;
  for (unsigned y = 0; y < QUAKEGENERIC_RES_Y; ++y) {
    for (unsigned repeat = 0; repeat < scale; ++repeat) {
      volatile uint32_t *out = (volatile uint32_t *)(uintptr_t)
          (buffer.address + (top + y * scale + repeat) * buffer.pitch);
      out += left;
      for (unsigned x = 0; x < QUAKEGENERIC_RES_X; ++x) {
        uint32_t pixel = palette_pixels[frame[y * QUAKEGENERIC_RES_X + x]];
        for (unsigned column = 0; column < scale; ++column) {
          *out++ = pixel;
        }
      }
    }
  }
}

int QG_GetKey(int *down, int *key)
{
  for (;;) {
    if (releasing_keys) {
      for (unsigned i = 0; i < QUAKE_KEY_COUNT; ++i) {
        if (quake_down[i]) {
          quake_down[i] = false;
          *down = 0;
          *key = (int)i;
          return 1;
        }
      }
      releasing_keys = false;
    }
    if (pending_count) {
      struct pending_key next = pending[pending_head];
      pending_head = (pending_head + 1) % PENDING_CAPACITY;
      --pending_count;
      quake_down[next.key] = next.down;
      *down = next.down;
      *key = next.key;
      return 1;
    }
    if (!focused || (!read_keyboard() && !read_pointer())) {
      return 0;
    }
  }
}

void QG_GetMouseMove(int *x, int *y)
{
  *x = (int)(motion_x > INT32_MAX ? INT32_MAX : motion_x < INT32_MIN ? INT32_MIN : motion_x);
  *y = (int)(motion_y > INT32_MAX ? INT32_MAX : motion_y < INT32_MIN ? INT32_MIN : motion_y);
  motion_x = motion_y = 0;
}

void QG_GetJoyAxes(float *axes)
{
  for (unsigned i = 0; i < QUAKEGENERIC_JOY_MAX_AXES; ++i) {
    axes[i] = 0;
  }
}
