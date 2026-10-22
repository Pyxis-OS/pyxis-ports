#ifndef RAYLIB_PYXIS_MINIAUDIO_H
#define RAYLIB_PYXIS_MINIAUDIO_H

#include <SDL2/SDL.h>

#define MA_PYXIS_RATE 48000
#define MA_PYXIS_CHANNELS 2
#define MA_PYXIS_FRAMES 1024

struct ma_pyxis_context {
  SDL_AudioDeviceID output;
  ma_device *device;
  SDL_atomic_t callback_failed;
};

static struct ma_pyxis_context *ma_pyxis_context_state(ma_context *context)
{
  return context->pUserData;
}

static void SDLCALL ma_pyxis_audio_callback(void *argument, Uint8 *buffer, int length)
{
  ma_device *device = argument;
  struct ma_pyxis_context *state = ma_pyxis_context_state(device->pContext);
  const int frame_bytes = MA_PYXIS_CHANNELS * (int)sizeof(float);
  if (length <= 0) {
    return;
  }
  if (length % frame_bytes != 0 || SDL_AtomicGet(&state->callback_failed)) {
    SDL_memset(buffer, 0, (size_t)length);
    SDL_AtomicSet(&state->callback_failed, 1);
    return;
  }
  ma_result result = ma_device_handle_backend_data_callback(device, buffer, NULL,
      (ma_uint32)(length / frame_bytes));
  if (result != MA_SUCCESS) {
    SDL_memset(buffer, 0, (size_t)length);
    SDL_AtomicSet(&state->callback_failed, 1);
  }
}

static ma_bool32 ma_pyxis_device_ready(ma_device *device)
{
  if (device == NULL || device->pContext == NULL) {
    return MA_FALSE;
  }
  struct ma_pyxis_context *state = ma_pyxis_context_state(device->pContext);
  return state != NULL && state->device == device && state->output != 0 &&
      !SDL_AtomicGet(&state->callback_failed) &&
      SDL_GetAudioDeviceStatus(state->output) != SDL_AUDIO_STOPPED;
}

static ma_result ma_pyxis_device_init(ma_device *device,
    const ma_device_config *config, ma_device_descriptor *playback,
    ma_device_descriptor *capture)
{
  (void)capture;
  if (config->deviceType != ma_device_type_playback) {
    return MA_DEVICE_TYPE_NOT_SUPPORTED;
  }
  if (config->playback.pDeviceID != NULL) {
    return MA_NO_DEVICE;
  }
  if (config->playback.shareMode != ma_share_mode_shared) {
    return MA_SHARE_MODE_NOT_SUPPORTED;
  }
  struct ma_pyxis_context *state = ma_pyxis_context_state(device->pContext);
  if (state->output != 0) {
    return MA_BUSY;
  }

  SDL_AudioSpec desired = {0};
  SDL_AudioSpec obtained = {0};
  desired.freq = MA_PYXIS_RATE;
  desired.format = AUDIO_F32SYS;
  desired.channels = MA_PYXIS_CHANNELS;
  desired.samples = MA_PYXIS_FRAMES;
  desired.callback = ma_pyxis_audio_callback;
  desired.userdata = device;
  SDL_AudioDeviceID output = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
  if (output == 0) {
    return MA_FAILED_TO_OPEN_BACKEND_DEVICE;
  }
  if (obtained.freq != desired.freq || obtained.format != desired.format ||
      obtained.channels != desired.channels || obtained.samples != desired.samples) {
    SDL_CloseAudioDevice(output);
    return MA_FORMAT_NOT_SUPPORTED;
  }

  /* SDL opens paused, so the converter and raylib mixer are ready before the
   * first callback. SDL converts these floats to the native S16LE session. */
  state->output = output;
  state->device = device;
  SDL_AtomicSet(&state->callback_failed, 0);
  playback->format = ma_format_f32;
  playback->channels = MA_PYXIS_CHANNELS;
  playback->sampleRate = MA_PYXIS_RATE;
  ma_channel_map_init_standard(ma_standard_channel_map_default,
      playback->channelMap, ma_countof(playback->channelMap), MA_PYXIS_CHANNELS);
  playback->periodSizeInFrames = obtained.samples;
  playback->periodSizeInMilliseconds = 0;
  playback->periodCount = 1;
  return MA_SUCCESS;
}

static ma_result ma_pyxis_device_uninit(ma_device *device)
{
  struct ma_pyxis_context *state = ma_pyxis_context_state(device->pContext);
  if (state->device == device && state->output != 0) {
    /* Close joins SDL's callback thread before its driver releases the native
     * session, and before miniaudio destroys the converter or raylib mutex. */
    SDL_CloseAudioDevice(state->output);
    state->output = 0;
    state->device = NULL;
  }
  return MA_SUCCESS;
}

static ma_result ma_pyxis_device_start(ma_device *device)
{
  if (!ma_pyxis_device_ready(device)) {
    return MA_FAILED_TO_START_BACKEND_DEVICE;
  }
  struct ma_pyxis_context *state = ma_pyxis_context_state(device->pContext);
  SDL_PauseAudioDevice(state->output, 0);
  return ma_pyxis_device_ready(device) ? MA_SUCCESS : MA_FAILED_TO_START_BACKEND_DEVICE;
}

static ma_result ma_pyxis_device_stop(ma_device *device)
{
  struct ma_pyxis_context *state = ma_pyxis_context_state(device->pContext);
  if (state->device != device || state->output == 0) {
    return MA_DEVICE_NOT_INITIALIZED;
  }
  /* Pause takes SDL's callback lock, so no callback can still use the mixer
   * when it returns. Like SDL pause, it holds the session and supplies silence. */
  SDL_PauseAudioDevice(state->output, 1);
  return ma_pyxis_device_ready(device) ? MA_SUCCESS : MA_FAILED_TO_STOP_BACKEND_DEVICE;
}

static ma_result ma_pyxis_enumerate(ma_context *context,
    ma_enum_devices_callback_proc callback, void *user_data)
{
  (void)context;
  (void)callback;
  (void)user_data;
  return MA_NOT_IMPLEMENTED;
}

static ma_result ma_pyxis_device_info(ma_context *context, ma_device_type type,
    const ma_device_id *id, ma_device_info *info)
{
  (void)context;
  (void)type;
  (void)id;
  (void)info;
  return MA_NOT_IMPLEMENTED;
}

static ma_result ma_pyxis_context_uninit(ma_context *context)
{
  struct ma_pyxis_context *state = ma_pyxis_context_state(context);
  if (state != NULL) {
    if (state->output != 0) {
      SDL_CloseAudioDevice(state->output);
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_free(state);
    context->pUserData = NULL;
  }
  return MA_SUCCESS;
}

static ma_result ma_pyxis_context_init(ma_context *context,
    const ma_context_config *config, ma_backend_callbacks *callbacks)
{
  (void)config;
  struct ma_pyxis_context *state = SDL_calloc(1, sizeof(*state));
  if (state == NULL) {
    return MA_OUT_OF_MEMORY;
  }
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    SDL_free(state);
    return MA_FAILED_TO_INIT_BACKEND;
  }
  const char *driver = SDL_GetCurrentAudioDriver();
  if (driver == NULL || SDL_strcmp(driver, "pyxis") != 0) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_free(state);
    return MA_NO_BACKEND;
  }
  context->pUserData = state;
  *callbacks = (ma_backend_callbacks){
    .onContextInit = ma_pyxis_context_init,
    .onContextUninit = ma_pyxis_context_uninit,
    .onContextEnumerateDevices = ma_pyxis_enumerate,
    .onContextGetDeviceInfo = ma_pyxis_device_info,
    .onDeviceInit = ma_pyxis_device_init,
    .onDeviceUninit = ma_pyxis_device_uninit,
    .onDeviceStart = ma_pyxis_device_start,
    .onDeviceStop = ma_pyxis_device_stop,
  };
  return MA_SUCCESS;
}

#endif
