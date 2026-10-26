/* Native gamepad INPUT, independent of video acquisition. SDL's joystick lock
 * serializes this catalog, its nonblocking reads and subsystem lifetime. */

#include "SDL_internal.h"

#ifdef SDL_JOYSTICK_PYXIS

#include <environment.h>
#include <gamepad.h>
#include <startup.h>

#include "SDL_events.h"
#include "joystick/SDL_sysjoystick.h"
#include "SDL_pyxisjoystick.h"

#define EDGE_BUTTON_COUNT 19
#define CATALOG_ATTEMPTS 3
#define EVENT_DRAIN_BUDGET 256
#define PYXIS_GUID_SIGNATURE 'p'

struct native_pad {
  struct gamepad_info info;
  struct gamepad_event accepted;
  SDL_JoystickID id;
  int player;
  bool live;
};

struct joystick_hwdata {
  uint64_t instance;
  bool ready;
};

static struct native_pad pads[GAMEPAD_DEVICE_CAPACITY];
static handle_t session = HANDLE_INVALID;
static bool acquired;
static bool catalog_pending;
static bool recovery_pending;
static bool dispatching;

static struct native_pad *find_pad(uint64_t instance)
{
  for (size_t i = 0; i < SDL_arraysize(pads); ++i) {
    if (pads[i].live && pads[i].info.instance == instance) {
      return &pads[i];
    }
  }
  return NULL;
}

static struct native_pad *indexed_pad(int index)
{
  for (size_t i = 0; i < SDL_arraysize(pads); ++i) {
    if (pads[i].live && index-- == 0) {
      return &pads[i];
    }
  }
  return NULL;
}

static Sint16 axis_value(Uint8 value)
{
  return (Sint16)((int)value * 257 - 32768);
}

static void publish_pad(struct native_pad *pad)
{
  SDL_Joystick *joystick = SDL_JoystickFromInstanceID(pad->id);
  if (!joystick || !joystick->hwdata->ready) {
    return;
  }
  if (!(pad->accepted.flags & GAMEPAD_EVENT_VALID) ||
      !(pad->accepted.flags & GAMEPAD_EVENT_FOCUSED)) {
    SDL_PrivateJoystickForceRecentering(joystick);
    return;
  }

  const struct gamepad_state state = pad->accepted.state;
  for (int i = 0; i < GAMEPAD_AXIS_COUNT; ++i) {
    SDL_PrivateJoystickAxis(joystick, i, axis_value(state.axes[i]));
  }
  for (int i = 0; i < EDGE_BUTTON_COUNT; ++i) {
    SDL_PrivateJoystickButton(joystick, i,
        state.buttons & (UINT32_C(1) << i) ? SDL_PRESSED : SDL_RELEASED);
  }
  static const Uint8 hats[] = {
    SDL_HAT_UP, SDL_HAT_RIGHTUP, SDL_HAT_RIGHT, SDL_HAT_RIGHTDOWN,
    SDL_HAT_DOWN, SDL_HAT_LEFTDOWN, SDL_HAT_LEFT, SDL_HAT_LEFTUP,
  };
  SDL_PrivateJoystickHat(joystick, 0,
      state.hat < SDL_arraysize(hats) ? hats[state.hat] : SDL_HAT_CENTERED);
}

static void accept_state(struct native_pad *pad, const struct gamepad_event *event)
{
  /* STATE can be newer than records still queued by READ. */
  if (event->sequence < pad->accepted.sequence) {
    return;
  }
  pad->accepted = *event;
  publish_pad(pad);
}

static void invalidate_pads(uint64_t sequence)
{
  for (size_t i = 0; i < SDL_arraysize(pads); ++i) {
    if (pads[i].live && sequence > pads[i].accepted.sequence) {
      struct gamepad_event invalid = {
        .instance = pads[i].info.instance, .sequence = sequence,
      };
      accept_state(&pads[i], &invalid);
    }
  }
}

static void remove_pad(struct native_pad *pad)
{
  SDL_JoystickID id = pad->id;
  SDL_zero(*pad);
  /* SDL keeps an open, detached joystick until Close. Its hwdata contains the
   * old native instance, never a pointer into a slot a replacement can reuse. */
  SDL_PrivateJoystickRemoved(id);
}

static enum call_status refresh_catalog(bool recover)
{
  for (int attempt = 0; attempt < CATALOG_ATTEMPTS; ++attempt) {
    struct gamepad_catalog catalog, confirmed;
    struct gamepad_info info[GAMEPAD_DEVICE_CAPACITY];
    struct gamepad_event states[GAMEPAD_DEVICE_CAPACITY];
    enum call_status status = gamepad_catalog(session, &catalog);
    if (status != CALL_OK) {
      return status;
    }
    if (catalog.count > SDL_arraysize(pads)) {
      return CALL_BAD_REQUEST;
    }
    for (uint32_t i = 0; i < catalog.count; ++i) {
      status = gamepad_info(session, catalog.instances[i], &info[i]);
      if (status != CALL_OK) {
        break;
      }
      if (info[i].profile != GAMEPAD_PROFILE_DUALSENSE_EDGE_USB) {
        return CALL_BAD_OPERATION;
      }
      if (recover || !find_pad(info[i].instance)) {
        status = gamepad_state(session, info[i].instance, &states[i]);
        if (status != CALL_OK) {
          break;
        }
      }
    }
    if (status == CALL_NOT_FOUND) {
      continue;
    }
    if (status != CALL_OK) {
      return status;
    }
    status = gamepad_catalog(session, &confirmed);
    if (status != CALL_OK) {
      return status;
    }
    if (catalog.generation != confirmed.generation) {
      continue;
    }

    for (size_t i = 0; i < SDL_arraysize(pads); ++i) {
      if (!recover || !pads[i].live) {
        continue;
      }
      bool present = false;
      for (uint32_t j = 0; j < catalog.count; ++j) {
        present |= pads[i].info.instance == catalog.instances[j];
      }
      if (!present) {
        remove_pad(&pads[i]);
      }
    }
    for (uint32_t i = 0; i < catalog.count; ++i) {
      struct native_pad *pad = find_pad(info[i].instance);
      if (pad) {
        /* Ordinary topology changes must not skip queued discrete edges on
         * existing devices. Only explicit RESET permits snapshot recovery. */
        if (recover) {
          accept_state(pad, &states[i]);
        }
        continue;
      }
      for (size_t j = 0; j < SDL_arraysize(pads); ++j) {
        if (!pads[j].live) {
          pad = &pads[j];
          break;
        }
      }
      if (!pad) {
        /* Current CATALOG can run ahead of queued REMOVED records. Preserve
         * those devices' discrete edges until removal is read in order. */
        return CALL_BUSY;
      }
      *pad = (struct native_pad){
        .info = info[i], .accepted = states[i],
        .id = SDL_GetNextJoystickInstanceID(), .player = -1, .live = true,
      };
      SDL_PrivateJoystickAdded(pad->id);
    }
    return CALL_OK;
  }
  return CALL_BUSY;
}

static void lose_session(enum call_status status)
{
  SDL_SetError("Pyxis gamepad session failed (status %u)", (unsigned)status);
  acquired = false;
  for (size_t i = 0; i < SDL_arraysize(pads); ++i) {
    if (pads[i].live) {
      remove_pad(&pads[i]);
    }
  }
  gamepad_release(session);
}

static void recover_catalog(void)
{
  enum call_status status = refresh_catalog(recovery_pending);
  if (status == CALL_OK) {
    catalog_pending = recovery_pending = false;
  } else if (status != CALL_BUSY && status != CALL_NOT_FOUND) {
    lose_session(status);
  }
}

static void pump_gamepads(void)
{
  if (!acquired) {
    return;
  }
  if (catalog_pending) {
    recover_catalog();
  }
  for (int i = 0; acquired && i < EVENT_DRAIN_BUDGET; ++i) {
    struct gamepad_event event;
    enum call_status status = gamepad_read(session, GAMEPAD_READ_POLL, &event);
    if (status == CALL_TIMED_OUT) {
      break;
    }
    if (status != CALL_OK) {
      lose_session(status);
      break;
    }
    switch (event.type) {
    case GAMEPAD_INPUT: {
      struct native_pad *pad = find_pad(event.instance);
      if (pad) {
        accept_state(pad, &event);
      }
      break;
    }
    case GAMEPAD_RESET:
      invalidate_pads(event.sequence);
      recovery_pending = catalog_pending = true;
      recover_catalog();
      break;
    case GAMEPAD_REMOVED: {
      struct native_pad *pad = find_pad(event.instance);
      if (pad) {
        remove_pad(pad);
      }
      catalog_pending = true;
      recover_catalog();
      break;
    }
    case GAMEPAD_ATTACHED:
      catalog_pending = true;
      recover_catalog();
      break;
    case GAMEPAD_FOCUS_GAINED:
    case GAMEPAD_FOCUS_LOST:
      invalidate_pads(event.sequence);
      break;
    default:
      lose_session(CALL_BAD_OPERATION);
      break;
    }
  }
}

static void PYXIS_JoystickDetect(void)
{
  /* SDL event watchers run synchronously under the recursive joystick lock.
   * A watcher pumping again must not interleave two native reports' edges. */
  if (dispatching) {
    return;
  }
  dispatching = true;
  pump_gamepads();
  dispatching = false;
}

handle_t PYXIS_GamepadWaitHandle(void)
{
  SDL_LockJoysticks();
  handle_t handle = acquired ? session : HANDLE_INVALID;
  SDL_UnlockJoysticks();
  return handle;
}

static int PYXIS_JoystickInit(void)
{
  if (session == HANDLE_INVALID) {
    enum call_status status = environment_capability(startup_environment_handle(),
        "gamepad", &session);
    if (status != CALL_OK) {
      return SDL_SetError("Pyxis gamepad INPUT grant required (status %u)", (unsigned)status);
    }
  }
  enum call_status status = gamepad_acquire(session);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis gamepad acquisition failed (status %u)", (unsigned)status);
  }
  acquired = catalog_pending = recovery_pending = true;
  dispatching = true;
  recover_catalog();
  dispatching = false;
  return acquired ? 0 : -1;
}

static int PYXIS_JoystickGetCount(void)
{
  int count = 0;
  for (size_t i = 0; i < SDL_arraysize(pads); ++i) {
    count += pads[i].live;
  }
  return count;
}

static const char *PYXIS_JoystickGetDeviceName(int index)
{
  return indexed_pad(index)->info.name;
}

static const char *PYXIS_JoystickGetDevicePath(int index)
{
  (void)index;
  return NULL; /* No filesystem or serial identity is exposed by INPUT. */
}

static int PYXIS_JoystickGetDeviceSteamVirtualGamepadSlot(int index)
{
  (void)index;
  return -1;
}

static int PYXIS_JoystickGetDevicePlayerIndex(int index)
{
  return indexed_pad(index)->player;
}

static void PYXIS_JoystickSetDevicePlayerIndex(int index, int player)
{
  indexed_pad(index)->player = player;
}

static SDL_JoystickGUID PYXIS_JoystickGetDeviceGUID(int index)
{
  const struct gamepad_info *info = &indexed_pad(index)->info;
  return SDL_CreateJoystickGUID(SDL_HARDWARE_BUS_USB, info->vendor, info->product,
      info->version, NULL, info->name, PYXIS_GUID_SIGNATURE, info->profile);
}

static SDL_JoystickID PYXIS_JoystickGetDeviceInstanceID(int index)
{
  return indexed_pad(index)->id;
}

static int PYXIS_JoystickOpen(SDL_Joystick *joystick, int index)
{
  struct native_pad *pad = indexed_pad(index);
  struct gamepad_event state;
  enum call_status status = gamepad_state(session, pad->info.instance, &state);
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis gamepad snapshot failed (status %u)", (unsigned)status);
  }
  struct joystick_hwdata *data = SDL_calloc(1, sizeof(*data));
  if (!data) {
    return SDL_OutOfMemory();
  }
  data->instance = pad->info.instance;
  joystick->hwdata = data;
  joystick->naxes = GAMEPAD_AXIS_COUNT;
  joystick->nbuttons = EDGE_BUTTON_COUNT;
  joystick->nhats = 1;
  pad->accepted = state;
  return 0;
}

static Uint32 PYXIS_JoystickGetCapabilities(SDL_Joystick *joystick)
{
  (void)joystick;
  return 0;
}

static int PYXIS_JoystickRumble(SDL_Joystick *joystick, Uint16 low, Uint16 high)
{
  (void)joystick; (void)low; (void)high;
  return SDL_Unsupported();
}

static int PYXIS_JoystickRumbleTriggers(SDL_Joystick *joystick, Uint16 left, Uint16 right)
{
  (void)joystick; (void)left; (void)right;
  return SDL_Unsupported();
}

static int PYXIS_JoystickSetLED(SDL_Joystick *joystick, Uint8 red, Uint8 green, Uint8 blue)
{
  (void)joystick; (void)red; (void)green; (void)blue;
  return SDL_Unsupported();
}

static int PYXIS_JoystickSendEffect(SDL_Joystick *joystick, const void *data, int size)
{
  (void)joystick; (void)data; (void)size;
  return SDL_Unsupported();
}

static int PYXIS_JoystickSetSensorsEnabled(SDL_Joystick *joystick, SDL_bool enabled)
{
  (void)joystick; (void)enabled;
  return SDL_Unsupported();
}

static void PYXIS_JoystickUpdate(SDL_Joystick *joystick)
{
  if (dispatching) {
    return;
  }
  dispatching = true;
  if (!joystick->hwdata->ready) {
    /* Core allocates axes after Open. Known neutral points must not be learned
     * from a deflected opening sample or a trigger's first extreme value. */
    for (int i = 0; i < GAMEPAD_AXIS_COUNT; ++i) {
      Sint16 neutral = i < GAMEPAD_AXIS_LEFT_TRIGGER ? 0 : SDL_MIN_SINT16;
      joystick->axes[i] = (SDL_JoystickAxisInfo){
        .initial_value = neutral, .value = neutral, .zero = neutral,
        .has_initial_value = SDL_TRUE, .has_second_value = SDL_TRUE,
        .sent_initial_value = SDL_TRUE,
      };
    }
    joystick->hwdata->ready = true;
  }
  pump_gamepads();
  struct native_pad *pad = find_pad(joystick->hwdata->instance);
  if (pad) {
    publish_pad(pad);
  }
  dispatching = false;
}

static void PYXIS_JoystickClose(SDL_Joystick *joystick)
{
  SDL_free(joystick->hwdata);
}

static void PYXIS_JoystickQuit(void)
{
  if (acquired) {
    enum call_status status = gamepad_release(session);
    if (status != CALL_OK) {
      SDL_SetError("Pyxis gamepad release failed (status %u)", (unsigned)status);
    }
  }
  acquired = catalog_pending = recovery_pending = false;
  SDL_zero(pads);
}

static SDL_bool PYXIS_JoystickGetGamepadMapping(int index, SDL_GamepadMapping *out)
{
  if (indexed_pad(index)->info.profile != GAMEPAD_PROFILE_DUALSENSE_EDGE_USB) {
    return SDL_FALSE;
  }
  *out = (SDL_GamepadMapping){
    .a = {.kind = EMappingKind_Button, .target = 1}, .b = {.kind = EMappingKind_Button, .target = 2},
    .x = {.kind = EMappingKind_Button, .target = 0}, .y = {.kind = EMappingKind_Button, .target = 3},
    .back = {.kind = EMappingKind_Button, .target = 8}, .guide = {.kind = EMappingKind_Button, .target = 12},
    .start = {.kind = EMappingKind_Button, .target = 9},
    .leftstick = {.kind = EMappingKind_Button, .target = 10}, .rightstick = {.kind = EMappingKind_Button, .target = 11},
    .leftshoulder = {.kind = EMappingKind_Button, .target = 4}, .rightshoulder = {.kind = EMappingKind_Button, .target = 5},
    .dpup = {.kind = EMappingKind_Hat, .target = SDL_HAT_UP},
    .dpdown = {.kind = EMappingKind_Hat, .target = SDL_HAT_DOWN},
    .dpleft = {.kind = EMappingKind_Hat, .target = SDL_HAT_LEFT},
    .dpright = {.kind = EMappingKind_Hat, .target = SDL_HAT_RIGHT},
    .misc1 = {.kind = EMappingKind_Button, .target = 14}, .touchpad = {.kind = EMappingKind_Button, .target = 13},
    .paddle1 = {.kind = EMappingKind_Button, .target = 18}, .paddle2 = {.kind = EMappingKind_Button, .target = 17},
    .paddle3 = {.kind = EMappingKind_Button, .target = 16}, .paddle4 = {.kind = EMappingKind_Button, .target = 15},
    .leftx = {.kind = EMappingKind_Axis, .target = GAMEPAD_AXIS_LEFT_X},
    .lefty = {.kind = EMappingKind_Axis, .target = GAMEPAD_AXIS_LEFT_Y},
    .rightx = {.kind = EMappingKind_Axis, .target = GAMEPAD_AXIS_RIGHT_X},
    .righty = {.kind = EMappingKind_Axis, .target = GAMEPAD_AXIS_RIGHT_Y},
    .lefttrigger = {.kind = EMappingKind_Axis, .target = GAMEPAD_AXIS_LEFT_TRIGGER},
    .righttrigger = {.kind = EMappingKind_Axis, .target = GAMEPAD_AXIS_RIGHT_TRIGGER},
  };
  return SDL_TRUE;
}

SDL_JoystickDriver SDL_PYXIS_JoystickDriver = {
  .Init = PYXIS_JoystickInit,
  .GetCount = PYXIS_JoystickGetCount,
  .Detect = PYXIS_JoystickDetect,
  .GetDeviceName = PYXIS_JoystickGetDeviceName,
  .GetDevicePath = PYXIS_JoystickGetDevicePath,
  .GetDeviceSteamVirtualGamepadSlot = PYXIS_JoystickGetDeviceSteamVirtualGamepadSlot,
  .GetDevicePlayerIndex = PYXIS_JoystickGetDevicePlayerIndex,
  .SetDevicePlayerIndex = PYXIS_JoystickSetDevicePlayerIndex,
  .GetDeviceGUID = PYXIS_JoystickGetDeviceGUID,
  .GetDeviceInstanceID = PYXIS_JoystickGetDeviceInstanceID,
  .Open = PYXIS_JoystickOpen,
  .Rumble = PYXIS_JoystickRumble,
  .RumbleTriggers = PYXIS_JoystickRumbleTriggers,
  .GetCapabilities = PYXIS_JoystickGetCapabilities,
  .SetLED = PYXIS_JoystickSetLED,
  .SendEffect = PYXIS_JoystickSendEffect,
  .SetSensorsEnabled = PYXIS_JoystickSetSensorsEnabled,
  .Update = PYXIS_JoystickUpdate,
  .Close = PYXIS_JoystickClose,
  .Quit = PYXIS_JoystickQuit,
  .GetGamepadMapping = PYXIS_JoystickGetGamepadMapping,
};

#endif /* SDL_JOYSTICK_PYXIS */
