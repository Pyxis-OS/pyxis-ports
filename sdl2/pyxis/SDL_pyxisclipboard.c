/* Clipboard authority follows the original native command through SDL's
 * private queue identity. No SDL event field or cached text grants access. */
#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_PYXIS

#include <clipboard.h>

#include "SDL_pyxisvideo.h"

static uint64_t action_id;
static uint64_t operation;
static handle_t clipboard = HANDLE_INVALID;
static bool delivered;
static unsigned int clipboard_callbacks;

static void forget_action(void)
{
  action_id = 0;
  operation = 0;
  clipboard = HANDLE_INVALID;
  delivered = false;
}

void PYXIS_CancelClipboard(void)
{
  if (action_id) {
    clipboard_graphics_refuse(clipboard, action_id, operation);
  }
  forget_action();
}

static handle_t layer_grant(uint64_t layer)
{
  switch (layer) {
  case CLIPBOARD_LAYER_LOCAL: return pyxis_video.clipboard_local;
  case CLIPBOARD_LAYER_SHARED: return pyxis_video.clipboard_shared;
  default: return HANDLE_INVALID;
  }
}

bool PYXIS_QueueClipboard(uint64_t id, uint64_t op, uint64_t layer)
{
  if (action_id) {
    PYXIS_CancelClipboard();
    clipboard_graphics_refuse(layer_grant(layer), id, op);
    return false;
  }
  clipboard = layer_grant(layer);
  action_id = id;
  operation = op;
  delivered = false;
  return true;
}

void PYXIS_DeliverClipboard(uint64_t id, bool batch)
{
  if (batch || (id && id != action_id) || (!id && delivered)) {
    PYXIS_CancelClipboard();
  } else if (id && id == action_id) {
    delivered = true;
  }
}

void PYXIS_DiscardClipboard(uint64_t id)
{
  if (id && id == action_id) {
    PYXIS_CancelClipboard();
  }
}

void PYXIS_BeginClipboardCallback(void)
{
  ++clipboard_callbacks;
}

void PYXIS_EndClipboardCallback(void)
{
  --clipboard_callbacks;
}

static bool ready(uint64_t expected, bool consume_refusal)
{
  if (clipboard_callbacks || !action_id || !delivered || operation != expected ||
      clipboard == HANDLE_INVALID) {
    if (consume_refusal) {
      PYXIS_CancelClipboard();
    }
    SDL_SetError("Pyxis clipboard needs a delivered matching physical command");
    return false;
  }
  return true;
}

/* Validate Unicode scalar UTF-8 without reading beyond the bounded C string. */
static bool text_length(const char *text, size_t *length)
{
  size_t i = 0;
  while (i <= CLIPBOARD_TEXT_MAX && text[i]) {
    unsigned char first = (unsigned char)text[i++];
    if (first < 0x80) {
      continue;
    }
    size_t continuation;
    uint32_t value;
    uint32_t minimum;
    if (first >= 0xc2 && first <= 0xdf) {
      continuation = 1;
      value = first & 0x1f;
      minimum = 0x80;
    } else if (first >= 0xe0 && first <= 0xef) {
      continuation = 2;
      value = first & 0x0f;
      minimum = 0x800;
    } else if (first >= 0xf0 && first <= 0xf4) {
      continuation = 3;
      value = first & 0x07;
      minimum = 0x10000;
    } else {
      return false;
    }
    if (i + continuation > CLIPBOARD_TEXT_MAX) {
      return false;
    }
    for (size_t j = 0; j < continuation; ++j) {
      unsigned char next = (unsigned char)text[i++];
      if ((next & 0xc0) != 0x80) {
        return false;
      }
      value = (value << 6) | (next & 0x3f);
    }
    if (value < minimum || value > 0x10ffff ||
        (value >= 0xd800 && value <= 0xdfff)) {
      return false;
    }
  }
  if (i > CLIPBOARD_TEXT_MAX) {
    return false;
  }
  *length = i;
  return true;
}

int PYXIS_SetClipboardText(SDL_VideoDevice *device, const char *text)
{
  (void)device;
  if (!ready(CLIPBOARD_PUBLISH, true)) {
    return -1;
  }
  size_t length;
  if (!text_length(text, &length)) {
    PYXIS_CancelClipboard();
    return SDL_SetError("Pyxis clipboard text is invalid UTF-8 or exceeds 64 KiB");
  }
  enum call_status status = clipboard_graphics_publish(clipboard, action_id, text, length);
  forget_action();
  if (status != CALL_OK) {
    return SDL_SetError("Pyxis clipboard publication refused (status %u)", (unsigned)status);
  }
  return 0;
}

char *PYXIS_GetClipboardText(SDL_VideoDevice *device)
{
  (void)device;
  if (!ready(CLIPBOARD_PASTE, true)) {
    char *empty = SDL_strdup("");
    if (!empty) {
      SDL_OutOfMemory();
    }
    return empty;
  }
  char *text = SDL_malloc(CLIPBOARD_TEXT_MAX + 1);
  if (!text) {
    PYXIS_CancelClipboard();
    SDL_OutOfMemory();
    return NULL;
  }
  size_t length;
  enum call_status status = clipboard_graphics_read(clipboard, action_id, text,
      CLIPBOARD_TEXT_MAX, &length);
  forget_action();
  if (status != CALL_OK) {
    text[0] = '\0';
    SDL_SetError("Pyxis clipboard read refused (status %u)", (unsigned)status);
  } else {
    text[length] = '\0';
  }
  return text;
}

SDL_bool PYXIS_HasClipboardText(SDL_VideoDevice *device)
{
  (void)device;
  if (!ready(CLIPBOARD_PASTE, false)) {
    return SDL_FALSE;
  }
  bool has_text;
  enum call_status status = clipboard_graphics_has(clipboard, action_id, &has_text);
  if (status != CALL_OK) {
    SDL_SetError("Pyxis clipboard probe refused (status %u)", (unsigned)status);
    return SDL_FALSE;
  }
  return has_text ? SDL_TRUE : SDL_FALSE;
}

#endif /* SDL_VIDEO_DRIVER_PYXIS */
