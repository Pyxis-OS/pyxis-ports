/* SDL audio playback through the space's native PCM session. The one default
 * playback device holds the session from physical open to close; SDL's device
 * thread waits for queue space and writes one 1,024-frame buffer at a time,
 * keeping the native queue near full. Underruns become native silence, counted
 * by the session's starvation counter; engine failure disconnects the device. */

#include "SDL_internal.h"

#ifdef SDL_AUDIO_DRIVER_PYXIS

#include <audio.h>
#include <clock.h>
#include <environment.h>
#include <handle.h>
#include <startup.h>
#include <wait.h>

#include "audio/SDL_sysaudio.h"

/* One maximum native WRITE; also the frame count that makes the session
 * writable, so each wakeup has room for exactly one buffer. */
#define PYXIS_AUDIO_FRAMES (AUDIO_WRITE_MAX / AUDIO_FRAME_BYTES)
/* Bounds one native wait so a closing device rechecks SDL's shutdown flag. */
#define PYXIS_AUDIO_WAIT_NS UINT64_C(100000000)

struct SDL_PrivateAudioData {
  Uint8 *buffer;
  bool acquired;
};

static handle_t audio_grant = HANDLE_INVALID;
static handle_t clock_grant = HANDLE_INVALID;

/* Returning false disconnects the device. During shutdown SDL's thread loop
 * stops at its next iteration. */
static bool PYXIS_WaitDevice(SDL_AudioDevice *device)
{
  while (!SDL_GetAtomicInt(&device->shutdown)) {
    uint64_t now;
    if (clock_now(clock_grant, &now) != CALL_OK) {
      return false;
    }
    struct wait_interest interest = {.handle = audio_grant, .events = WAIT_WRITABLE};
    uint64_t events = 0;
    enum call_status status = wait_many(&interest, 1, now + PYXIS_AUDIO_WAIT_NS, &events);
    if (status == CALL_TIMED_OUT) {
      continue;
    }
    if (status != CALL_OK || (events & WAIT_ERROR)) {
      return false;
    }
    if (events & WAIT_WRITABLE) {
      return true;
    }
  }
  return true;
}

/* Only this device writes to the session, so space observed by the wait is
 * still there. A refused write during shutdown drops the final buffer. */
static bool PYXIS_PlayDevice(SDL_AudioDevice *device, const Uint8 *buffer, int size)
{
  enum call_status status = audio_write(audio_grant, buffer, (size_t)size);
  return status == CALL_OK ||
      (status == CALL_WOULD_BLOCK && SDL_GetAtomicInt(&device->shutdown));
}

static Uint8 *PYXIS_GetDeviceBuf(SDL_AudioDevice *device, int *size)
{
  (void)size;
  return device->hidden->buffer;
}

/* SDL joins the device thread before calling this, also after a failed open.
 * Release discards PCM still queued; already mixed hardware frames can still
 * play. */
static void PYXIS_CloseDevice(SDL_AudioDevice *device)
{
  if (!device->hidden) {
    return;
  }
  if (device->hidden->acquired) {
    audio_release(audio_grant);
  }
  SDL_free(device->hidden->buffer);
  SDL_free(device->hidden);
  device->hidden = NULL;
}

/* The session's format replaces whatever SDL requested; SDL converts. */
static bool PYXIS_OpenDevice(SDL_AudioDevice *device)
{
  device->hidden = SDL_calloc(1, sizeof(*device->hidden));
  if (!device->hidden) {
    return false;
  }

  struct audio_acquire_reply session;
  enum call_status status = audio_acquire(audio_grant, &session);
  if (status == CALL_BUSY) {
    return SDL_SetError("Pyxis audio session is already owned in this space");
  }
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis audio acquisition failed (status %u)", (unsigned)status);
  }
  device->hidden->acquired = true;
  if (session.rate != AUDIO_RATE || session.channels != AUDIO_CHANNELS ||
      session.format != AUDIO_FORMAT_S16LE) {
    return SDL_SetError("Pyxis audio session has an unsupported format");
  }

  device->spec.freq = AUDIO_RATE;
  device->spec.format = SDL_AUDIO_S16LE;
  device->spec.channels = AUDIO_CHANNELS;
  device->sample_frames = PYXIS_AUDIO_FRAMES;
  SDL_UpdatedAudioDeviceFormat(device);
  device->hidden->buffer = SDL_malloc(device->buffer_size);
  if (!device->hidden->buffer) {
    return false;
  }
  SDL_memset(device->hidden->buffer, device->silence_value, device->buffer_size);
  return true;
}

static void PYXIS_Deinitialize(void)
{
  if (audio_grant != HANDLE_INVALID) {
    handle_close(audio_grant);
    audio_grant = HANDLE_INVALID;
  }
  if (clock_grant != HANDLE_INVALID) {
    handle_close(clock_grant);
    clock_grant = HANDLE_INVALID;
  }
}

static bool PYXIS_Init(SDL_AudioDriverImpl *impl)
{
  environment_capability(startup_environment_handle(), "audio", &audio_grant);
  environment_capability(startup_environment_handle(), "clock", &clock_grant);
  if (audio_grant == HANDLE_INVALID || clock_grant == HANDLE_INVALID) {
    PYXIS_Deinitialize();
    return SDL_SetError("Pyxis audio needs the audio and clock grants");
  }
  impl->Deinitialize = PYXIS_Deinitialize;
  impl->OpenDevice = PYXIS_OpenDevice;
  impl->WaitDevice = PYXIS_WaitDevice;
  impl->PlayDevice = PYXIS_PlayDevice;
  impl->GetDeviceBuf = PYXIS_GetDeviceBuf;
  impl->CloseDevice = PYXIS_CloseDevice;
  impl->OnlyHasDefaultPlaybackDevice = true;
  return true;
}

AudioBootStrap PYXISAUDIO_bootstrap = {
  "pyxis", "Pyxis PCM session", PYXIS_Init, false, false,
};

#endif /* SDL_AUDIO_DRIVER_PYXIS */
