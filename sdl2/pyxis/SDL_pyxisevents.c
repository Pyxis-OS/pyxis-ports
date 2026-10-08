/* Pyxis keyboard and pointer input for SDL. Focus changes and input resets
 * release every held key and button: a key held across them must be pressed
 * again, as the Pyxis input sessions require. */

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <keyboard.h>
#include <pointer.h>
#include <pxe/key_layout.h>
#include <wait.h>

#include "SDL_pyxisvideo.h"
#include "events/SDL_keyboard_c.h"
#include "events/SDL_mouse_c.h"

#define ASCII_FIRST_PRINTABLE ' '
#define ASCII_LAST_PRINTABLE '~'

static bool keyboard_focused;
static bool pointer_focused;
static uint32_t held_buttons; /* Pyxis POINTER_BUTTON_* bits SDL has pressed. */

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

/* The only place pointer movement becomes an SDL position. Pyxis reports
 * relative counts, so SDL keeps the position, clamped to the window. When the
 * system pointer reports positions, this becomes an absolute update. */
static void send_pointer_motion(int32_t dx, int32_t dy)
{
  if (dx || dy) {
    SDL_SendMouseMotion(pyxis_video.window, 0, SDL_TRUE, dx, dy);
  }
}

/* SDL owns the position, so warping only moves SDL's copy of it. */
static void PYXIS_WarpMouse(SDL_Window *window, int x, int y)
{
  SDL_SendMouseMotion(window, 0, SDL_FALSE, x, y);
}

/* Every pointer event is already relative; SDL derives both modes from it. */
static int PYXIS_SetRelativeMouseMode(SDL_bool enabled)
{
  (void)enabled;
  return 0;
}

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
  SDL_ResetKeyboard();
  release_buttons();
  keyboard_focused = false;
  pointer_focused = false;
}

void PYXIS_InitInput(void)
{
  SDL_Mouse *mouse = SDL_GetMouse();
  mouse->WarpMouse = PYXIS_WarpMouse;
  mouse->SetRelativeMouseMode = PYXIS_SetRelativeMouseMode;
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
    keyboard_focused = (event->flags & KEYBOARD_EVENT_FOCUSED) != 0;
    SDL_ResetKeyboard();
    SDL_SetKeyboardFocus(keyboard_focused ? pyxis_video.window : NULL);
    return;
  }
  if (!keyboard_focused || event->key >= KEY_COUNT || !scancodes[event->key]) {
    return;
  }
  bool pressed = event->action != KEY_RELEASE;
  SDL_SendKeyboardKey(pressed ? SDL_PRESSED : SDL_RELEASED, scancodes[event->key]);
  if (pressed) {
    send_text(event);
  }
}

static void handle_pointer(const struct pointer_event *event)
{
  if (event->type != POINTER_INPUT) {
    release_buttons();
    pointer_focused = (event->flags & POINTER_EVENT_FOCUSED) != 0;
    SDL_SetMouseFocus(pointer_focused ? pyxis_video.window : NULL);
    return;
  }
  if (!pointer_focused) {
    return;
  }
  send_pointer_motion(event->dx, event->dy);
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
    SDL_SendMouseWheel(pyxis_video.window, 0, 0.0f, (float)-event->wheel,
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

void PYXIS_PumpEvents(SDL_VideoDevice *device)
{
  if (!pyxis_video.window) {
    return;
  }

  /* One poll covers display geometry and keyboard readiness. The pointer
   * cannot be waited on, so it is always polled. */
  struct wait_interest interests[2] = {
    {.handle = pyxis_video.display, .events = WAIT_RESIZED,
     .observed_generation = pyxis_video.generation},
    {.handle = pyxis_video.keyboard, .events = WAIT_READABLE},
  };
  uint64_t events[2] = {0};
  if (wait_many(interests, SDL_arraysize(interests), 0, events) == CALL_OK) {
    if (events[0] & WAIT_RESIZED) {
      PYXIS_FollowDisplaySize(device);
    }
    if (events[1] & (WAIT_READABLE | WAIT_ERROR)) {
      drain_keyboard();
    }
  } else {
    drain_keyboard();
  }

  struct pointer_event motion;
  while (pyxis_video.pointer_owned &&
         pointer_read(pyxis_video.pointer, POINTER_READ_POLL, &motion) == CALL_OK) {
    handle_pointer(&motion);
  }
}

#endif /* SDL_VIDEO_DRIVER_PYXIS */
