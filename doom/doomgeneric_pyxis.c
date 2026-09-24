#include "doomkeys.h"

/* Both interfaces use KEY_* names; keep Doom values distinct at this boundary. */
enum doom_key {
  DOOM_RIGHTARROW = KEY_RIGHTARROW,
  DOOM_LEFTARROW = KEY_LEFTARROW,
  DOOM_UPARROW = KEY_UPARROW,
  DOOM_DOWNARROW = KEY_DOWNARROW,
  DOOM_ESCAPE = KEY_ESCAPE,
  DOOM_ENTER = KEY_ENTER,
  DOOM_TAB = KEY_TAB,
  DOOM_BACKSPACE = KEY_BACKSPACE,
  DOOM_PAUSE = KEY_PAUSE,
  DOOM_RSHIFT = KEY_RSHIFT,
  DOOM_FIRE = KEY_FIRE,
  DOOM_USE = KEY_USE,
  DOOM_STRAFE_L = KEY_STRAFE_L,
  DOOM_STRAFE_R = KEY_STRAFE_R,
  DOOM_RALT = KEY_RALT,
  DOOM_F1 = KEY_F1,
  DOOM_F2 = KEY_F2,
  DOOM_F3 = KEY_F3,
  DOOM_F4 = KEY_F4,
  DOOM_F5 = KEY_F5,
  DOOM_F6 = KEY_F6,
  DOOM_F7 = KEY_F7,
  DOOM_F8 = KEY_F8,
  DOOM_F9 = KEY_F9,
  DOOM_F10 = KEY_F10,
  DOOM_F11 = KEY_F11,
  DOOM_F12 = KEY_F12,
};
#undef KEY_RIGHTARROW
#undef KEY_LEFTARROW
#undef KEY_UPARROW
#undef KEY_DOWNARROW
#undef KEY_STRAFE_L
#undef KEY_STRAFE_R
#undef KEY_USE
#undef KEY_FIRE
#undef KEY_ESCAPE
#undef KEY_ENTER
#undef KEY_TAB
#undef KEY_F1
#undef KEY_F2
#undef KEY_F3
#undef KEY_F4
#undef KEY_F5
#undef KEY_F6
#undef KEY_F7
#undef KEY_F8
#undef KEY_F9
#undef KEY_F10
#undef KEY_F11
#undef KEY_F12
#undef KEY_BACKSPACE
#undef KEY_PAUSE
#undef KEY_EQUALS
#undef KEY_MINUS
#undef KEY_RSHIFT
#undef KEY_RCTRL
#undef KEY_RALT
#undef KEY_LALT
#undef KEY_CAPSLOCK
#undef KEY_NUMLOCK
#undef KEY_SCRLCK
#undef KEY_PRTSCR
#undef KEY_HOME
#undef KEY_END
#undef KEY_PGUP
#undef KEY_PGDN
#undef KEY_INS
#undef KEY_DEL

#include <clock.h>
#include <display.h>
#include <keyboard.h>
#include <startup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomgeneric.h"
#include "i_system.h"

static handle_t display, keyboard, clock;
static struct display_buffer buffer;
static bool display_owned, keyboard_owned, focused;
static bool held[KEY_COUNT];
static bool game_down[256];
static bool releasing_keys;
static uint64_t started_at, paused_at, paused_ns;
static unsigned scale, left, top;

static const unsigned char keymap[KEY_COUNT] = {
  [KEY_ESCAPE] = DOOM_ESCAPE, [KEY_ENTER] = DOOM_ENTER,
  [KEY_TAB] = DOOM_TAB, [KEY_BACKSPACE] = DOOM_BACKSPACE,
  [KEY_UP] = DOOM_UPARROW, [KEY_DOWN] = DOOM_DOWNARROW,
  [KEY_LEFT] = DOOM_LEFTARROW, [KEY_RIGHT] = DOOM_RIGHTARROW,
  [KEY_LEFT_CONTROL] = DOOM_FIRE, [KEY_RIGHT_CONTROL] = DOOM_FIRE,
  [KEY_LEFT_SHIFT] = DOOM_RSHIFT, [KEY_RIGHT_SHIFT] = DOOM_RSHIFT,
  [KEY_LEFT_ALT] = DOOM_RALT, [KEY_RIGHT_ALT] = DOOM_RALT,
  [KEY_SPACE] = DOOM_USE, [KEY_PAUSE] = DOOM_PAUSE,
  [KEY_F1] = DOOM_F1, [KEY_F2] = DOOM_F2, [KEY_F3] = DOOM_F3,
  [KEY_F4] = DOOM_F4, [KEY_F5] = DOOM_F5, [KEY_F6] = DOOM_F6,
  [KEY_F7] = DOOM_F7, [KEY_F8] = DOOM_F8, [KEY_F9] = DOOM_F9,
  [KEY_F10] = DOOM_F10, [KEY_F11] = DOOM_F11, [KEY_F12] = DOOM_F12,
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
  [KEY_COMMA] = DOOM_STRAFE_L, [KEY_PERIOD] = DOOM_STRAFE_R, [KEY_SLASH] = '/',
  [KEY_SEMICOLON] = ';', [KEY_APOSTROPHE] = '\'', [KEY_GRAVE] = '`',
  [KEY_LEFT_BRACKET] = '[', [KEY_RIGHT_BRACKET] = ']', [KEY_BACKSLASH] = '\\',
};

static void release_resources(void)
{
  if (keyboard_owned) {
    enum call_status status = keyboard_release(keyboard);
    keyboard_owned = false;
    if (status != CALL_OK) {
      fprintf(stderr, "doom: keyboard release failed (status %u)\n", (unsigned)status);
    }
  }
  if (display_owned) {
    enum call_status status = display_release(display);
    display_owned = false;
    if (status != CALL_OK) {
      fprintf(stderr, "doom: display release failed (status %u)\n", (unsigned)status);
    }
  }
}

static void require_ok(enum call_status status, const char *operation)
{
  if (status != CALL_OK) {
    release_resources();
    fprintf(stderr, "doom: %s failed (status %u)\n", operation, (unsigned)status);
    exit(EXIT_FAILURE);
  }
}

static uint64_t now_ns(void)
{
  uint64_t now;
  require_ok(clock_now(clock, &now), "clock read");
  return now;
}

void DG_Init(void)
{
  display = startup_resource("display");
  keyboard = startup_resource("keyboard");
  clock = startup_resource("clock");
  if (display == HANDLE_INVALID || keyboard == HANDLE_INVALID || clock == HANDLE_INVALID) {
    fputs("doom: display, keyboard and clock resources are required\n", stderr);
    exit(EXIT_FAILURE);
  }
  if (!DG_ScreenBuffer) {
    fputs("doom: cannot allocate render buffer\n", stderr);
    exit(EXIT_FAILURE);
  }

  require_ok(display_acquire(display, &buffer), "display acquisition");
  display_owned = true;
  require_ok(keyboard_acquire(keyboard), "keyboard acquisition");
  keyboard_owned = true;
  I_AtExit(release_resources, true);

  scale = buffer.width / DOOMGENERIC_RESX;
  if (scale > buffer.height / DOOMGENERIC_RESY) {
    scale = buffer.height / DOOMGENERIC_RESY;
  }
  if (scale == 0) {
    release_resources();
    fputs("doom: display is smaller than the game frame\n", stderr);
    exit(EXIT_FAILURE);
  }
  left = (buffer.width - DOOMGENERIC_RESX * scale) / 2;
  top = (buffer.height - DOOMGENERIC_RESY * scale) / 2;
  /* Doom asks for time before its first input poll. Consume initial focus
   * here so an inactive start cannot leave its tic loop waiting on time zero. */
  while (!focused) {
    struct keyboard_event event;
    require_ok(keyboard_read(keyboard, 0, &event), "initial keyboard focus");
    focused = (event.flags & KEYBOARD_EVENT_FOCUSED) != 0;
  }
  started_at = paused_at = now_ns();
  require_ok(display_present(display), "display presentation");
}

void DG_DrawFrame(void)
{
  for (unsigned y = 0; y < DOOMGENERIC_RESY; ++y) {
    for (unsigned repeat = 0; repeat < scale; ++repeat) {
      volatile uint32_t *out = (volatile uint32_t *)(uintptr_t)
          (buffer.address + (top + y * scale + repeat) * buffer.pitch);
      out += left;
      for (unsigned x = 0; x < DOOMGENERIC_RESX; ++x) {
        uint32_t rgb = DG_ScreenBuffer[y * DOOMGENERIC_RESX + x];
        uint32_t pixel = ((rgb >> 16) & 0xff) << buffer.red_shift |
            ((rgb >> 8) & 0xff) << buffer.green_shift |
            (rgb & 0xff) << buffer.blue_shift;
        for (unsigned column = 0; column < scale; ++column) {
          *out++ = pixel;
        }
      }
    }
  }
}

void DG_SleepMs(uint32_t milliseconds)
{
  require_ok(clock_sleep_for(clock, (uint64_t)milliseconds * 1000000), "sleep");
}

uint32_t DG_GetTicksMs(void)
{
  uint64_t now = focused ? now_ns() : paused_at;
  return (uint32_t)((now - started_at - paused_ns) / 1000000);
}

int DG_GetKey(int *pressed, unsigned char *key)
{
  for (;;) {
    /* Reset the game's state as well as ours; dropping only local state leaves
     * Doom moving/firing after a focus change or a lost-event notification. */
    if (releasing_keys) {
      for (unsigned i = 0; i < sizeof(game_down) / sizeof(game_down[0]); ++i) {
        if (game_down[i]) {
          game_down[i] = false;
          *pressed = 0;
          *key = i;
          return 1;
        }
      }
      releasing_keys = false;
    }

    struct keyboard_event event;
    enum call_status status = keyboard_read(keyboard,
        focused ? KEYBOARD_READ_POLL : 0, &event);
    if (status == CALL_TIMED_OUT) {
      return 0;
    }
    require_ok(status, "keyboard read");

    if (event.action == KEY_FOCUS_GAINED || event.action == KEY_FOCUS_LOST ||
        event.action == KEY_STATE_RESET) {
      uint64_t now = now_ns();
      bool next_focus = (event.flags & KEYBOARD_EVENT_FOCUSED) != 0;
      if (focused && !next_focus) {
        paused_at = now;
      } else if (!focused && next_focus) {
        paused_ns += now - paused_at;
      }
      focused = next_focus;
      memset(held, 0, sizeof(held));
      releasing_keys = true;
      continue;
    }
    if (event.key >= KEY_COUNT || !keymap[event.key] ||
        (event.action != KEY_PRESS && event.action != KEY_RELEASE)) {
      continue;
    }
    held[event.key] = event.action == KEY_PRESS;
    unsigned char translated = keymap[event.key];
    bool down = false;
    /* Left/right modifiers share a Doom key; releasing one must not release
     * the other while it is still held. */
    for (unsigned i = 0; i < KEY_COUNT; ++i) {
      down |= held[i] && keymap[i] == translated;
    }
    if (game_down[translated] != down) {
      game_down[translated] = down;
      *pressed = down;
      *key = translated;
      return 1;
    }
  }
}

void DG_SetWindowTitle(const char *title)
{
  (void)title; /* The space owns its tab title. */
}

int main(int argc, char **argv)
{
  bool has_iwad = false;
  for (int i = 1; i < argc; ++i) {
    has_iwad |= strcmp(argv[i], "-iwad") == 0;
    if (strcmp(argv[i], "-gfxmode") == 0 || strcmp(argv[i], "-scaling") == 0 ||
        strcmp(argv[i], "-loadgame") == 0 || strcmp(argv[i], "-record") == 0 ||
        strcmp(argv[i], "-timedemo") == 0) {
      fprintf(stderr, "doom: %s is not supported by this port\n", argv[i]);
      return EXIT_FAILURE;
    }
  }

  char **arguments = calloc((size_t)argc + 3, sizeof(*arguments));
  if (!arguments) {
    return EXIT_FAILURE;
  }
  memcpy(arguments, argv, (size_t)argc * sizeof(*arguments));
  if (!has_iwad) {
    arguments[argc++] = "-iwad";
    arguments[argc++] = "app://share/doom/DOOM.WAD";
  }
  doomgeneric_Create(argc, arguments);
  for (;;) {
    doomgeneric_Tick();
  }
}
