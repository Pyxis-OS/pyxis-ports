/* SDL audio output through the space's native PCM session. One SDL output
 * device holds the session from open to close; SDL's callback thread waits for
 * queue space and writes one 1,024-frame buffer at a time, keeping the native
 * queue near full. Underruns become native silence, counted by the session's
 * starvation counter; engine failure disconnects the SDL device. */

#include "SDL_internal.h"

#ifdef SDL_AUDIO_DRIVER_PYXIS

#include <audio.h>
#include <clock.h>
#include <startup.h>
#include <environment.h>
#include <handle.h>
#include <wait.h>

#include "SDL_audio.h"
#include "audio/SDL_audio_c.h"
#include "audio/SDL_sysaudio.h"

/* One maximum native WRITE; also the frame count that makes the session
 * writable, so each wakeup has room for exactly one buffer. */
#define PYXIS_AUDIO_FRAMES (AUDIO_WRITE_MAX / AUDIO_FRAME_BYTES)
/* Bounds one native wait so a closing device rechecks SDL's shutdown flag. */
#define PYXIS_AUDIO_WAIT_NS UINT64_C(100000000)

struct SDL_PrivateAudioData {
  Uint8 *buffer;
};

static handle_t audio_grant = HANDLE_INVALID;
static handle_t clock_grant = HANDLE_INVALID;

static void PYXIS_WaitDevice(SDL_AudioDevice *device)
{
  while (!SDL_AtomicGet(&device->shutdown)) {
    uint64_t now;
    if (clock_now(clock_grant, &now) != CALL_OK) {
      SDL_OpenedAudioDeviceDisconnected(device);
      return;
    }
    struct wait_interest interest = {.handle = audio_grant, .events = WAIT_WRITABLE};
    uint64_t events = 0;
    enum call_status status = wait_many(&interest, 1, now + PYXIS_AUDIO_WAIT_NS, &events);
    if (status == CALL_TIMED_OUT) {
      continue;
    }
    if (status != CALL_OK || (events & WAIT_ERROR)) {
      SDL_OpenedAudioDeviceDisconnected(device);
      return;
    }
    if (events & WAIT_WRITABLE) {
      return;
    }
  }
}

/* Only this device writes to the session, so space observed by the wait is
 * still there. A refused write during shutdown drops the final buffer. */
static void PYXIS_PlayDevice(SDL_AudioDevice *device)
{
  enum call_status status = audio_write(audio_grant, device->hidden->buffer, device->spec.size);
  if (status != CALL_OK && !(status == CALL_WOULD_BLOCK && SDL_AtomicGet(&device->shutdown))) {
    SDL_OpenedAudioDeviceDisconnected(device);
  }
}

static Uint8 *PYXIS_GetDeviceBuf(SDL_AudioDevice *device)
{
  return device->hidden->buffer;
}

/* SDL joins the callback thread before calling this. Release discards PCM
 * still queued; already mixed hardware frames can still play. */
static void PYXIS_CloseDevice(SDL_AudioDevice *device)
{
  audio_release(audio_grant);
  SDL_free(device->hidden->buffer);
  SDL_free(device->hidden);
  device->hidden = NULL;
}

static int PYXIS_OpenDevice(SDL_AudioDevice *device, const char *name)
{
  (void)name;
  device->hidden = SDL_calloc(1, sizeof(*device->hidden));
  if (!device->hidden) {
    return SDL_OutOfMemory();
  }

  struct audio_acquire_reply session;
  enum call_status status = audio_acquire(audio_grant, &session);
  if (status == CALL_BUSY) {
    SDL_free(device->hidden);
    device->hidden = NULL;
    return SDL_SetError("Pyxis audio session is already owned in this space");
  }
  if (status != CALL_OK) {
    SDL_free(device->hidden);
    device->hidden = NULL;
    return SDL_SetError("Pyxis audio acquisition failed (status %u)", (unsigned)status);
  }
  /* From here CloseDevice releases the session on any failure. */
  if (session.rate != AUDIO_RATE || session.channels != AUDIO_CHANNELS ||
      session.format != AUDIO_FORMAT_S16LE) {
    return SDL_SetError("Pyxis audio session has an unsupported format");
  }

  device->spec.freq = AUDIO_RATE;
  device->spec.format = AUDIO_S16LSB;
  device->spec.channels = AUDIO_CHANNELS;
  device->spec.samples = PYXIS_AUDIO_FRAMES;
  SDL_CalculateAudioSpec(&device->spec);
  device->hidden->buffer = SDL_malloc(device->spec.size);
  if (!device->hidden->buffer) {
    return SDL_OutOfMemory();
  }
  SDL_memset(device->hidden->buffer, device->spec.silence, device->spec.size);
  return 0;
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

static SDL_bool PYXIS_Init(SDL_AudioDriverImpl *impl)
{
  environment_capability(startup_environment_handle(), "audio", &audio_grant);
  environment_capability(startup_environment_handle(), "clock", &clock_grant);
  if (audio_grant == HANDLE_INVALID || clock_grant == HANDLE_INVALID) {
    PYXIS_Deinitialize();
    SDL_SetError("Pyxis audio needs the audio and clock grants");
    return SDL_FALSE;
  }
  impl->Deinitialize = PYXIS_Deinitialize;
  impl->OpenDevice = PYXIS_OpenDevice;
  impl->WaitDevice = PYXIS_WaitDevice;
  impl->PlayDevice = PYXIS_PlayDevice;
  impl->GetDeviceBuf = PYXIS_GetDeviceBuf;
  impl->CloseDevice = PYXIS_CloseDevice;
  impl->OnlyHasDefaultOutputDevice = SDL_TRUE;
  return SDL_TRUE;
}

AudioBootStrap PYXISAUDIO_bootstrap = {
  "pyxis", "Pyxis PCM session", PYXIS_Init, SDL_FALSE,
};

#endif /* SDL_AUDIO_DRIVER_PYXIS */
