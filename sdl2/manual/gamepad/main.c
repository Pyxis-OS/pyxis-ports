/* SPDX-License-Identifier: MPL-2.0 */
#include <SDL.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

enum { max_devices = 16, canvas_width = 1160, canvas_height = 800 };

struct device {
  SDL_Joystick *joystick;
  SDL_GameController *controller;
  SDL_JoystickID instance;
};

struct counters {
  uint64_t events;
  uint64_t joy_axis;
  uint64_t joy_button;
  uint64_t joy_hat;
  uint64_t pad_axis;
  uint64_t pad_button;
  uint64_t attach;
  uint64_t remove;
  uint64_t focus;
};

static struct device devices[max_devices];
static int selected = -1;
static struct counters counts;

/* Original diagnostic lettering; no external font or asset dependency. */
static const Uint8 letters[][7] = {
  {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
  {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
  {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
  {14,17,16,23,17,17,14}, {17,17,17,31,17,17,17},
  {14,4,4,4,4,4,14}, {7,2,2,2,2,18,12},
  {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
  {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
  {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
  {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
  {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
  {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
  {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
  {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
};

static const Uint8 digits[][7] = {
  {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
  {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
  {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
  {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
  {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
};

static void rectangle(SDL_Surface *surface, int x, int y, int w, int h,
    Uint8 red, Uint8 green, Uint8 blue)
{
  SDL_Rect rect = {x, y, w, h};
  SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, red, green, blue));
}

static void lettering(SDL_Surface *surface, int x, int y, const char *message)
{
  for (; *message; ++message, x += 12) {
    unsigned char c = (unsigned char)*message;
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    for (int row = 0; row < 7; ++row) {
      unsigned bits = 0;
      if (c >= 'A' && c <= 'Z') bits = letters[c - 'A'][row];
      else if (c >= '0' && c <= '9') bits = digits[c - '0'][row];
      else if (c == '-' && row == 3) bits = 14;
      else if (c == ':' && (row == 2 || row == 5)) bits = 4;
      else if (c == '/' && row >= 1 && row <= 5) bits = 1u << (row - 1);
      else if (c == '.' && row == 6) bits = 4;
      for (int col = 0; col < 5; ++col) {
        if (bits & (16u >> col))
          rectangle(surface, x + col*2, y + row*2, 2, 2, 225, 233, 242);
      }
    }
  }
}

static int find_instance(SDL_JoystickID instance)
{
  for (int i = 0; i < max_devices; ++i) {
    if (devices[i].joystick && devices[i].instance == instance) return i;
  }
  return -1;
}

static void mapping_log(struct device *device)
{
  if (!device->controller) {
    printf("instance=%d has no automatic GameController mapping\n", device->instance);
    return;
  }
  char *mapping = SDL_GameControllerMapping(device->controller);
  printf("instance=%d mapping=%s\n", device->instance,
      mapping ? mapping : SDL_GetError());
  SDL_free(mapping);
}

static void open_device(int index)
{
  SDL_JoystickID instance = SDL_JoystickGetDeviceInstanceID(index);
  if (instance < 0) {
    printf("device index=%d identity failed: %s\n", index, SDL_GetError());
    return;
  }
  if (find_instance(instance) >= 0) return;
  int slot = -1;
  for (int i = 0; i < max_devices; ++i) {
    if (!devices[i].joystick) {
      slot = i;
      break;
    }
  }
  if (slot < 0) {
    printf("device index=%d instance=%d exceeds viewer limit of %d open devices\n",
        index, instance, max_devices);
    return;
  }

  struct device *device = &devices[slot];
  if (SDL_IsGameController(index)) {
    device->controller = SDL_GameControllerOpen(index);
    if (!device->controller) {
      printf("GameControllerOpen index=%d failed: %s\n", index, SDL_GetError());
      return;
    }
    device->joystick = SDL_GameControllerGetJoystick(device->controller);
  } else {
    device->joystick = SDL_JoystickOpen(index);
    if (!device->joystick) {
      printf("JoystickOpen index=%d failed: %s\n", index, SDL_GetError());
      return;
    }
  }
  device->instance = SDL_JoystickInstanceID(device->joystick);
  if (selected < 0) selected = slot;
  char guid[33];
  SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(device->joystick), guid, sizeof(guid));
  const char *name = SDL_JoystickName(device->joystick);
  printf("opened slot=%d instance=%d name=%s GUID=%s VID=%04x PID=%04x version=%04x "
      "axes=%d buttons=%d hats=%d mapped=%d\n", slot, device->instance,
      name ? name : "unknown", guid, SDL_JoystickGetVendor(device->joystick),
      SDL_JoystickGetProduct(device->joystick),
      SDL_JoystickGetProductVersion(device->joystick),
      SDL_JoystickNumAxes(device->joystick), SDL_JoystickNumButtons(device->joystick),
      SDL_JoystickNumHats(device->joystick), device->controller != NULL);
  mapping_log(device);
}

static void close_device(int slot)
{
  struct device *device = &devices[slot];
  printf("closed slot=%d instance=%d\n", slot, device->instance);
  /* GameController owns its joystick reference. */
  if (device->controller) SDL_GameControllerClose(device->controller);
  else SDL_JoystickClose(device->joystick);
  *device = (struct device){0};
  if (selected == slot) {
    selected = -1;
    for (int i = 0; i < max_devices; ++i) {
      if (devices[i].joystick) {
        selected = i;
        break;
      }
    }
  }
}

static void select_next(void)
{
  for (int step = 1; step <= max_devices; ++step) {
    int slot = (selected + step) % max_devices;
    if (devices[slot].joystick) {
      selected = slot;
      printf("selected slot=%d instance=%d\n", slot, devices[slot].instance);
      return;
    }
  }
}

static void operation_log(const char *name, int result)
{
  printf("%s result=%d%s%s\n", name, result, result < 0 ? " error=" : "",
      result < 0 ? SDL_GetError() : "");
}

static void output_operation(SDL_Keycode key)
{
  if (selected < 0) {
    printf("output request: no selected device\n");
    return;
  }
  struct device *device = &devices[selected];
  printf("output request instance=%d key=%c\n", device->instance, (char)key);
  if (key == SDLK_r) {
    operation_log("SDL_JoystickRumble", SDL_JoystickRumble(device->joystick,
        0x1000, 0x1000, 150));
    if (device->controller)
      operation_log("SDL_GameControllerRumble", SDL_GameControllerRumble(
          device->controller, 0x1000, 0x1000, 150));
  } else if (key == SDLK_t) {
    operation_log("SDL_JoystickRumbleTriggers", SDL_JoystickRumbleTriggers(
        device->joystick, 0x1000, 0x1000, 150));
    if (device->controller)
      operation_log("SDL_GameControllerRumbleTriggers", SDL_GameControllerRumbleTriggers(
          device->controller, 0x1000, 0x1000, 150));
  } else if (key == SDLK_l) {
    operation_log("SDL_JoystickSetLED", SDL_JoystickSetLED(device->joystick, 64, 0, 0));
    if (device->controller)
      operation_log("SDL_GameControllerSetLED", SDL_GameControllerSetLED(
          device->controller, 64, 0, 0));
  } else if (key == SDLK_e) {
    /* Empty opaque effect: exercises refusal without supplying a vendor report. */
    const Uint8 unused = 0;
    operation_log("SDL_JoystickSendEffect", SDL_JoystickSendEffect(
        device->joystick, &unused, 0));
    if (device->controller)
      operation_log("SDL_GameControllerSendEffect", SDL_GameControllerSendEffect(
          device->controller, &unused, 0));
  }
  fflush(stdout);
}

static void axis_bar(SDL_Surface *surface, int x, int y, const char *label,
    bool available, Sint16 value, bool trigger)
{
  char text[48];
  if (available) snprintf(text, sizeof(text), "%s %d", label, value);
  else snprintf(text, sizeof(text), "%s N/A", label);
  lettering(surface, x, y, text);
  rectangle(surface, x + 240, y, 280, 16, 38, 48, 64);
  if (!available) return;
  int position = ((int)value + 32768)*279/65535;
  int center = trigger ? 0 : 140;
  if (trigger) position = value > 0 ? (int)value*279/32767 : 0;
  int left = position < center ? position : center;
  int length = position < center ? center - position : position - center;
  rectangle(surface, x + 240 + left, y, length + 1, 16, 42, 167, 165);
  rectangle(surface, x + 240 + center, y, 1, 16, 225, 233, 242);
}

static void button(SDL_Surface *surface, int x, int y, const char *label,
    bool available, bool pressed)
{
  rectangle(surface, x, y, 172, 26, available ? (pressed ? 28 : 38) : 25,
      available ? (pressed ? 130 : 48) : 29, available ? (pressed ? 102 : 64) : 35);
  lettering(surface, x + 5, y + 6, label);
}

static int present(SDL_Window *window, SDL_Surface *canvas)
{
  SDL_Surface *surface = SDL_GetWindowSurface(window);
  if (!surface) return -1;
  if (SDL_BlitScaled(canvas, NULL, surface, NULL) < 0) return -1;
  return SDL_UpdateWindowSurface(window);
}

static int draw(SDL_Window *window, SDL_Surface *surface)
{
  static const char *axis_names[] = {"LX", "LY", "RX", "RY", "LT", "RT"};
  static const char *raw_buttons[] = {
    "0 SQUARE", "1 CROSS", "2 CIRCLE", "3 TRIANGLE", "4 L1", "5 R1",
    "6 L2 DIG", "7 R2 DIG", "8 CREATE", "9 OPTIONS", "10 L3", "11 R3",
    "12 PS", "13 TOUCH", "14 MIC", "15 FN L", "16 FN R", "17 PAD L", "18 PAD R",
  };
  static const char *mapped_buttons[SDL_CONTROLLER_BUTTON_MAX] = {
    "A", "B", "X", "Y", "BACK", "GUIDE", "START", "LEFT STICK", "RIGHT STICK",
    "L SHOULDER", "R SHOULDER", "DPAD UP", "DPAD DOWN", "DPAD LEFT", "DPAD RIGHT",
    "MISC1", "PADDLE1", "PADDLE2", "PADDLE3", "PADDLE4", "TOUCHPAD",
  };
  rectangle(surface, 0, 0, surface->w, surface->h, 16, 23, 35);
  char line[96];
  snprintf(line, sizeof(line), "SDL GAMEPAD - %s - FOCUS %s", SDL_VIEWER_PROFILE,
      SDL_GetKeyboardFocus() == window ? "YES" : "NO");
  lettering(surface, 20, 18, line);
  lettering(surface, 20, 46, "TAB DEVICE  ESC EXIT  R RUMBLE  T TRIGGERS  L LED  E EMPTY EFFECT");
  snprintf(line, sizeof(line), "EVENTS %" PRIu64 "  ATTACH %" PRIu64 " REMOVE %" PRIu64 " FOCUS %" PRIu64,
      counts.events, counts.attach, counts.remove, counts.focus);
  lettering(surface, 20, 76, line);
  snprintf(line, sizeof(line), "JOY AX %" PRIu64 " BTN %" PRIu64 " HAT %" PRIu64 "  PAD AX %" PRIu64 " BTN %" PRIu64,
      counts.joy_axis, counts.joy_button, counts.joy_hat, counts.pad_axis, counts.pad_button);
  lettering(surface, 20, 104, line);
  if (selected < 0) {
    lettering(surface, 20, 160, "NO OPEN DEVICES - WAITING FOR SDL HOTPLUG EVENT");
    return present(window, surface);
  }
  struct device *device = &devices[selected];
  SDL_Joystick *joystick = device->joystick;
  snprintf(line, sizeof(line), "SLOT %d INSTANCE %d  AXES %d BUTTONS %d HATS %d  MAPPED %s",
      selected, device->instance, SDL_JoystickNumAxes(joystick),
      SDL_JoystickNumButtons(joystick), SDL_JoystickNumHats(joystick),
      device->controller ? "YES" : "NO");
  lettering(surface, 20, 142, line);
  lettering(surface, 20, 180, "RAW JOYSTICK - NATIVE ORDER");
  lettering(surface, 600, 180, "AUTOMATIC GAMECONTROLLER");
  for (int i = 0; i < 6; ++i) {
    bool raw_available = i < SDL_JoystickNumAxes(joystick);
    axis_bar(surface, 20, 212 + i*28, axis_names[i], raw_available,
        raw_available ? SDL_JoystickGetAxis(joystick, i) : 0, false);
    bool mapped_available = device->controller && SDL_GameControllerHasAxis(
        device->controller, (SDL_GameControllerAxis)i);
    axis_bar(surface, 600, 212 + i*28, axis_names[i], mapped_available,
        mapped_available ? SDL_GameControllerGetAxis(device->controller,
            (SDL_GameControllerAxis)i) : 0, i >= 4);
  }
  for (int i = 0; i < 19; ++i) {
    bool available = i < SDL_JoystickNumButtons(joystick);
    button(surface, 20 + (i%3)*180, 404 + (i/3)*32, raw_buttons[i], available,
        available && SDL_JoystickGetButton(joystick, i));
  }
  for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
    bool available = device->controller && SDL_GameControllerHasButton(
        device->controller, (SDL_GameControllerButton)i);
    button(surface, 600 + (i%3)*180, 404 + (i/3)*32, mapped_buttons[i], available,
        available && SDL_GameControllerGetButton(device->controller,
            (SDL_GameControllerButton)i));
  }
  bool has_hat = SDL_JoystickNumHats(joystick) > 0;
  Uint8 hat = has_hat ? SDL_JoystickGetHat(joystick, 0) : 0;
  snprintf(line, sizeof(line), "HAT0 %u  %s", hat, has_hat ? "" : "N/A");
  lettering(surface, 20, 650, line);
  button(surface, 200, 642, "UP", has_hat, hat & SDL_HAT_UP);
  button(surface, 200, 706, "DOWN", has_hat, hat & SDL_HAT_DOWN);
  button(surface, 20, 674, "LEFT", has_hat, hat & SDL_HAT_LEFT);
  button(surface, 380, 674, "RIGHT", has_hat, hat & SDL_HAT_RIGHT);
  lettering(surface, 600, 650, "TEAL PRESSED - DARK UNAVAILABLE");
  lettering(surface, 600, 678, "RAW TRIGGERS -32768 TO 32767");
  lettering(surface, 600, 706, "MAPPED TRIGGERS 0 TO 32767");
  lettering(surface, 20, 766, "IDLE SDL WAITEVENT - NO REFRESH TIMER - IDENTITY AND API RESULTS ON STDOUT");
  return present(window, surface);
}

int main(void)
{
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "0");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
    fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_Window *window = SDL_CreateWindow("SDL joystick / GameController viewer",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, canvas_width, canvas_height, 0);
  if (!window) {
    fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  SDL_Surface *canvas = SDL_CreateRGBSurfaceWithFormat(0, canvas_width, canvas_height,
      32, SDL_PIXELFORMAT_ARGB8888);
  if (!canvas) {
    fprintf(stderr, "SDL_CreateRGBSurfaceWithFormat: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_version version;
  SDL_GetVersion(&version);
  printf("SDL %u.%u.%u archive=%s software window surface; background events disabled\n",
      version.major, version.minor, version.patch, SDL_VIEWER_PROFILE);
  int total = SDL_NumJoysticks();
  printf("SDL_NumJoysticks=%d\n", total);
  if (total < 0) printf("enumeration error: %s\n", SDL_GetError());
  for (int i = 0; i < total; ++i) open_device(i);
  fflush(stdout);
  int result = 0;
  bool running = true;
  while (running) {
    if (draw(window, canvas) < 0) {
      fprintf(stderr, "draw: %s\n", SDL_GetError());
      result = 1;
      break;
    }
    SDL_Event event;
    /* No timeout or periodic refresh: controller-only input must wake SDL. */
    if (!SDL_WaitEvent(&event)) {
      fprintf(stderr, "SDL_WaitEvent: %s\n", SDL_GetError());
      result = 1;
      break;
    }
    ++counts.events;
    switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;
      case SDL_KEYDOWN:
        if (event.key.repeat) break;
        if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
        else if (event.key.keysym.sym == SDLK_TAB) select_next();
        else if (event.key.keysym.sym == SDLK_r || event.key.keysym.sym == SDLK_t ||
            event.key.keysym.sym == SDLK_l || event.key.keysym.sym == SDLK_e)
          output_operation(event.key.keysym.sym);
        break;
      case SDL_JOYDEVICEADDED:
        ++counts.attach;
        open_device(event.jdevice.which);
        break;
      case SDL_CONTROLLERDEVICEADDED:
        open_device(event.cdevice.which);
        break;
      case SDL_JOYDEVICEREMOVED: {
        ++counts.remove;
        int slot = find_instance(event.jdevice.which);
        if (slot >= 0) close_device(slot);
        break;
      }
      case SDL_CONTROLLERDEVICEREMAPPED: {
        int slot = find_instance(event.cdevice.which);
        if (slot >= 0) mapping_log(&devices[slot]);
        break;
      }
      case SDL_JOYAXISMOTION: ++counts.joy_axis; break;
      case SDL_JOYBUTTONDOWN:
      case SDL_JOYBUTTONUP: ++counts.joy_button; break;
      case SDL_JOYHATMOTION: ++counts.joy_hat; break;
      case SDL_CONTROLLERAXISMOTION: ++counts.pad_axis; break;
      case SDL_CONTROLLERBUTTONDOWN:
      case SDL_CONTROLLERBUTTONUP: ++counts.pad_button; break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
            event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
          ++counts.focus;
          printf("focus %s\n", event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ?
              "gained" : "lost");
        }
        if (event.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
        break;
    }
    fflush(stdout);
  }
  for (int i = 0; i < max_devices; ++i) {
    if (devices[i].joystick) close_device(i);
  }
  printf("exit events=%" PRIu64 " joy-axis=%" PRIu64 " joy-button=%" PRIu64
      " joy-hat=%" PRIu64 " controller-axis=%" PRIu64 " controller-button=%" PRIu64 "\n",
      counts.events, counts.joy_axis, counts.joy_button, counts.joy_hat,
      counts.pad_axis, counts.pad_button);
  SDL_FreeSurface(canvas);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return result;
}
