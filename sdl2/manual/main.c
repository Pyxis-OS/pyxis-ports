/* Opt-in manual exercise: every clipboard call follows an operator key. */
#include <SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define GLYPH_WIDTH 5
#define GLYPH_HEIGHT 7
#define TEXT_LIMIT 65536
#define LINE_CAPACITY 180
#define RESULT_LINES 7

static const uint8_t glyph_digits[10][GLYPH_HEIGHT] = {
  {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e},
  {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e},
  {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f},
  {0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e},
  {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02},
  {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e},
  {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e},
  {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
  {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e},
  {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c},
};
static const uint8_t glyph_l[GLYPH_HEIGHT] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f};
static const uint8_t glyph_m[GLYPH_HEIGHT] = {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11};
static const uint8_t glyph_r[GLYPH_HEIGHT] = {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11};
static const uint8_t glyph_x[GLYPH_HEIGHT] = {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11};
static const uint8_t glyph_y[GLYPH_HEIGHT] = {0x11, 0x11, 0x0a, 0x04, 0x04, 0x04, 0x04};
static const uint8_t glyph_minus[GLYPH_HEIGHT] = {0, 0, 0, 0x1f, 0, 0, 0};
static const uint8_t glyph_a[GLYPH_HEIGHT] = {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
static const uint8_t glyph_c[GLYPH_HEIGHT] = {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e};
static const uint8_t glyph_d[GLYPH_HEIGHT] = {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e};
static const uint8_t glyph_e[GLYPH_HEIGHT] = {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f};
static const uint8_t glyph_f[GLYPH_HEIGHT] = {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10};
static const uint8_t glyph_i[GLYPH_HEIGHT] = {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1f};
static const uint8_t glyph_k[GLYPH_HEIGHT] = {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
static const uint8_t glyph_o[GLYPH_HEIGHT] = {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e};
static const uint8_t glyph_t[GLYPH_HEIGHT] = {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
static const uint8_t glyph_w[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a};

static const uint8_t glyph_b[GLYPH_HEIGHT] = {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e};
static const uint8_t glyph_g[GLYPH_HEIGHT] = {0xe, 0x11, 0x10, 0x17, 0x11, 0x11, 0xf};
static const uint8_t glyph_h[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11};
static const uint8_t glyph_j[GLYPH_HEIGHT] = {0x7, 0x2, 0x2, 0x2, 0x2, 0x12, 0xc};
static const uint8_t glyph_n[GLYPH_HEIGHT] = {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11};
static const uint8_t glyph_p[GLYPH_HEIGHT] = {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10};
static const uint8_t glyph_q[GLYPH_HEIGHT] = {0xe, 0x11, 0x11, 0x11, 0x15, 0x12, 0xd};
static const uint8_t glyph_s[GLYPH_HEIGHT] = {0xf, 0x10, 0x10, 0xe, 0x1, 0x1, 0x1e};
static const uint8_t glyph_u[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0xe};
static const uint8_t glyph_v[GLYPH_HEIGHT] = {0x11, 0x11, 0x11, 0x11, 0x11, 0xa, 0x4};
static const uint8_t glyph_z[GLYPH_HEIGHT] = {0x1f, 0x1, 0x2, 0x4, 0x8, 0x10, 0x1f};

static const uint8_t *glyph(char c)
{
  if (c >= '0' && c <= '9') {
    return glyph_digits[c - '0'];
  }
  switch (c) {
  case 'A': return glyph_a;
  case 'B': return glyph_b;
  case 'C': return glyph_c;
  case 'D': return glyph_d;
  case 'E': return glyph_e;
  case 'F': return glyph_f;
  case 'G': return glyph_g;
  case 'H': return glyph_h;
  case 'I': return glyph_i;
  case 'J': return glyph_j;
  case 'K': return glyph_k;
  case 'L': return glyph_l;
  case 'M': return glyph_m;
  case 'N': return glyph_n;
  case 'O': return glyph_o;
  case 'P': return glyph_p;
  case 'Q': return glyph_q;
  case 'R': return glyph_r;
  case 'S': return glyph_s;
  case 'T': return glyph_t;
  case 'U': return glyph_u;
  case 'V': return glyph_v;
  case 'W': return glyph_w;
  case 'X': return glyph_x;
  case 'Y': return glyph_y;
  case 'Z': return glyph_z;
  case '-': return glyph_minus;
  default: return NULL;
  }
}

static char results[RESULT_LINES][LINE_CAPACITY];
static size_t result_count;
static char payload[TEXT_LIMIT + 2];
static int payload_mode = 1;
static int paste_mode = 1;
static bool repeat_call;
static bool batch;
static bool flush_command;
static int delay_seconds;
static const char *payload_names[] = {
  "", "ASCII", "MULTIBYTE", "EMPTY", "64 KIB", "INVALID UTF8", "OVER LIMIT",
};

static void record(const char *text)
{
  memmove(results[1], results[0], sizeof(results) - sizeof(results[0]));
  snprintf(results[0], LINE_CAPACITY, "%s", text);
  ++result_count;
  printf("%s\n", text);
}

static uint64_t hash_bytes(const char *bytes, size_t length)
{
  uint64_t hash = UINT64_C(14695981039346656037);
  for (size_t i = 0; i < length; ++i) {
    hash = (hash ^ (unsigned char)bytes[i]) * UINT64_C(1099511628211);
  }
  return hash;
}

static void set_text(void)
{
  char line[LINE_CAPACITY];
  SDL_ClearError();
  int status = SDL_SetClipboardText(payload);
  snprintf(line, sizeof(line), "SET %d BYTES %zu HASH %016llX ERROR %s", status,
      strlen(payload), (unsigned long long)hash_bytes(payload, strlen(payload)), SDL_GetError());
  record(line);
}

static void get_text(bool probe)
{
  char line[LINE_CAPACITY];
  SDL_ClearError();
  if (probe) {
    SDL_bool has = SDL_HasClipboardText();
    snprintf(line, sizeof(line), "HAS %u ERROR %s", (unsigned)has, SDL_GetError());
    record(line);
    SDL_ClearError();
  }
  char *text = SDL_GetClipboardText();
  size_t length = text ? strlen(text) : 0;
  snprintf(line, sizeof(line), "GET BYTES %zu HASH %016llX ERROR %s", length,
      (unsigned long long)hash_bytes(text, length), SDL_GetError());
  record(line);
  if (text) {
    char preview[65];
    size_t n = length < sizeof(preview) - 1 ? length : sizeof(preview) - 1;
    for (size_t i = 0; i < n; ++i) {
      preview[i] = text[i] >= ' ' && text[i] <= '~' ? text[i] : '.';
    }
    preview[n] = '\0';
    snprintf(line, sizeof(line), "PREVIEW %s", preview);
    record(line);
  }
  SDL_free(text);
}

static void choose_payload(int mode)
{
  payload_mode = mode;
  switch (mode) {
  case 1: strcpy(payload, "Pyxis clipboard ASCII\nline two\tend"); break;
  case 2: strcpy(payload, "UTF-8 caf\xc3\xa9 \xe6\x97\xa5\xe6\x9c\xac \xf0\x9f\x8c\x8d\r\n"); break;
  case 3: payload[0] = '\0'; break;
  case 4:
  case 6:
    memset(payload, 'A', TEXT_LIMIT + (mode == 6));
    payload[TEXT_LIMIT + (mode == 6)] = '\0';
    break;
  case 5: strcpy(payload, "invalid \xed\xa0\x80"); break;
  }
}

static void draw_text(SDL_Renderer *renderer, int y, const char *text)
{
  int x = 12;
  for (; *text; ++text, x += 6) {
    char c = *text;
    if (c >= 'a' && c <= 'z') {
      c -= 'a' - 'A';
    }
    const uint8_t *rows = glyph(c);
    if (!rows) {
      continue;
    }
    for (int r = 0; r < GLYPH_HEIGHT; ++r) {
      for (int col = 0; col < GLYPH_WIDTH; ++col) {
        if (rows[r] & (1 << (GLYPH_WIDTH - col - 1))) {
          SDL_Rect pixel = {x + col, y + r, 1, 1};
          SDL_RenderFillRect(renderer, &pixel);
        }
      }
    }
  }
}

static void draw(SDL_Renderer *renderer)
{
  SDL_SetRenderDrawColor(renderer, 20, 26, 34, 255);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 220, 230, 240, 255);
  draw_text(renderer, 12, "SDL CLIPBOARD MANUAL - ESC QUIT");
  draw_text(renderer, 36, "1 ASCII  2 MULTIBYTE  3 EMPTY  4 64 KIB  5 INVALID  6 OVER LIMIT");
  draw_text(renderer, 60, "CTRL C V LOCAL - SUPER SHIFT C V SHARED");
  draw_text(renderer, 84, "G HAS THEN GET - D DIRECT GET - R REPEAT CALL - U UNARMED - F REFUSE");
  draw_text(renderer, 108, "T DELAY 6 SECONDS - B BATCH QUEUE - L FLUSH BEFORE COMMAND CALL");
  draw_text(renderer, 132, "Q PAUSE 2 SECONDS FOR OVERLAP - N SYNTHETIC CTRL V");
  char line[LINE_CAPACITY];
  snprintf(line, sizeof(line), "PAYLOAD %s - PASTE %s - REPEAT %u - DELAY %d - BATCH %u - DISCARD %u",
      payload_names[payload_mode], paste_mode == 1 ? "HAS GET" : paste_mode == 2 ? "DIRECT" : "REFUSE",
      (unsigned)repeat_call, delay_seconds, (unsigned)batch, (unsigned)flush_command);
  draw_text(renderer, 168, line);
  snprintf(line, sizeof(line), "RESULT COUNT %zu", result_count);
  draw_text(renderer, 192, line);
  for (size_t i = 0; i < RESULT_LINES; ++i) {
    draw_text(renderer, 220 + (int)i * 24, results[i]);
  }
  SDL_RenderPresent(renderer);
}

static bool handle(const SDL_Event *event)
{
  if (event->type == SDL_QUIT) {
    return false;
  }
  if (event->type != SDL_KEYDOWN || event->key.repeat) {
    return true;
  }
  SDL_Keycode key = event->key.keysym.sym;
  if (key == SDLK_ESCAPE) {
    return false;
  }
  if ((event->key.keysym.mod & KMOD_CTRL) && (key == SDLK_c || key == SDLK_v)) {
    if (flush_command) {
      SDL_FlushEvent(SDL_KEYDOWN);
      flush_command = false;
      record("KEYDOWN QUEUE FLUSHED BEFORE CLIPBOARD CALL");
    }
    if (delay_seconds) {
      SDL_Delay((Uint32)delay_seconds * 1000);
    }
    if (paste_mode == 3) {
      if (key == SDLK_c) {
        get_text(false);
      } else {
        set_text();
      }
    } else if (key == SDLK_c) {
      set_text();
      if (repeat_call) {
        set_text();
      }
    } else {
      get_text(paste_mode == 1);
      if (repeat_call) {
        get_text(false);
      }
    }
  } else if (key >= SDLK_1 && key <= SDLK_6) {
    choose_payload((int)(key - SDLK_0));
  } else if (key == SDLK_g) {
    paste_mode = 1;
  } else if (key == SDLK_d) {
    paste_mode = 2;
  } else if (key == SDLK_r) {
    repeat_call = !repeat_call;
  } else if (key == SDLK_t) {
    delay_seconds = delay_seconds ? 0 : 6;
  } else if (key == SDLK_b) {
    batch = !batch;
  } else if (key == SDLK_l) {
    flush_command = !flush_command;
  } else if (key == SDLK_u) {
    set_text();
    get_text(true);
  } else if (key == SDLK_f) {
    /* A Set against Paste or Get against Copy consumes the matching action. */
    paste_mode = 3;
    record("REFUSAL MODE - NEXT CTRL C CALLS GET - NEXT CTRL V CALLS SET");
  } else if (key == SDLK_q) {
    SDL_Delay(2000);
  } else if (key == SDLK_n) {
    SDL_Event synthetic;
    SDL_zero(synthetic);
    synthetic.type = SDL_KEYDOWN;
    synthetic.key.keysym.sym = SDLK_v;
    synthetic.key.keysym.mod = KMOD_CTRL;
    SDL_PushEvent(&synthetic);
  }
  return true;
}

int main(void)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL init: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Window *window = SDL_CreateWindow("Pyxis SDL clipboard", 0, 0, 1024, 768, 0);
  SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE) : NULL;
  if (!renderer) {
    printf("SDL window/renderer: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  choose_payload(1);
  draw(renderer);
  bool running = true;
  while (running) {
    SDL_Event event;
    if (batch) {
      if (!SDL_WaitEvent(NULL)) {
        break;
      }
      SDL_PumpEvents();
    } else {
      if (!SDL_WaitEvent(&event)) {
        break;
      }
      running = handle(&event);
    }
    if (batch) {
      SDL_Event events[8];
      SDL_PumpEvents();
      int count = SDL_PeepEvents(events, 8, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
      for (int i = 0; i < count && running; ++i) {
        running = handle(&events[i]);
      }
    }
    draw(renderer);
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
