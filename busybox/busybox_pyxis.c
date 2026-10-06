/* Pyxis platform adapter for BusyBox vi/less: entry points and libbb helpers.
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * vi draws on the named output console and reads the named input console
 * through libterm, as Kilo does. Those grants are separate from stdio, so
 * stdout/stderr only carry usage text and fatal diagnostics. */
#include <handle.h>
#include <stdarg.h>
#include <startup.h>
#include <term.h>
#include "libbb.h"

int vi_main(int argc, char **argv);

struct globals *ptr_to_globals;
#ifdef PYXIS_APPLET_LESS
int less_main(int argc, char **argv);
const char *applet_name = "less";
char bb_common_bufsiz1[4096];
uint32_t option_mask32;
#define ALLOCATION_ERROR "out of memory"
#define TERMINAL_WRITE_ERROR "cannot write terminal output"
#define PASSTHROUGH_ERROR "cannot keep Ctrl+C as pager input"
#else
const char *applet_name = "vi";
#define ALLOCATION_ERROR "out of memory; unsaved edits are lost"
#define TERMINAL_WRITE_ERROR "cannot write terminal output; unsaved edits are lost"
#define PASSTHROUGH_ERROR "cannot keep Ctrl+C as editor input"
#endif
int optind = 1;

static struct terminal terminal;
static handle_t passthrough = HANDLE_INVALID;

/* Screen updates are collected and written when vi waits for input. */
static char output[4096];
static size_t output_length;

/* A key read while vi only asked whether input was waiting. */
static bool key_pending;
static int64_t pending_key;
static int pending_errno;

static NORETURN void fail(const char *message)
{
  fprintf(stderr, "%s: %s\n", applet_name, message);
  exit(EXIT_FAILURE);
}

static bool write_output(void)
{
  size_t length = output_length;
  output_length = 0;
  return !length || term_write_all(&terminal, output, length) == CALL_OK;
}

/* Fatal paths keep their own message even if the terminal is gone. */
void bb_simple_error_msg_and_die(const char *message)
{
  write_output();
  fail(message);
}

void xfunc_die(void)
{
  write_output();
  exit(EXIT_FAILURE);
}

void bb_show_usage(void)
{
#ifdef PYXIS_APPLET_LESS
  fputs("Usage: less [-EMmN~F] [FILE]...\n", stderr);
#else
  fputs("Usage: vi [-c CMD] [-R] [-H] [FILE]...\n", stderr);
#endif
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  terminal.input = startup_resource("input");
  terminal.output = startup_resource("output");
  if (terminal.input == HANDLE_INVALID || terminal.output == HANDLE_INVALID) {
    fail("input and output console capabilities are required");
  }

#ifdef PYXIS_APPLET_LESS
  int status = less_main(argc, argv);
#else
  int status = vi_main(argc, argv);
#endif
  fflush_all();
  return status;
}

void *xmalloc(size_t size)
{
  void *pointer = malloc(size ? size : 1);
  if (!pointer) {
    bb_simple_error_msg_and_die(ALLOCATION_ERROR);
  }
  return pointer;
}

void *xzalloc(size_t size)
{
  void *pointer = xmalloc(size);
  memset(pointer, 0, size);
  return pointer;
}

void *xrealloc(void *pointer, size_t size)
{
  pointer = realloc(pointer, size ? size : 1);
  if (!pointer) {
    bb_simple_error_msg_and_die(ALLOCATION_ERROR);
  }
  return pointer;
}

char *xstrdup(const char *text)
{
  return xstrndup(text, INT_MAX);
}

char *xstrndup(const char *text, int limit)
{
  size_t length = strnlen(text, limit < 0 ? 0 : (size_t)limit);
  char *copy = xmalloc(length + 1);
  memcpy(copy, text, length);
  copy[length] = '\0';
  return copy;
}

char *xasprintf(const char *format, ...)
{
  char *text;
  va_list args;
  va_start(args, format);
  int length = vasprintf(&text, format, args);
  va_end(args);
  if (length < 0) {
    bb_simple_error_msg_and_die(ALLOCATION_ERROR);
  }
  return text;
}

void *xmalloc_read(int descriptor, size_t *size)
{
  size_t limit = *size < SIZE_MAX ? *size : SIZE_MAX - 1;
  size_t length = 0;
  size_t capacity = 0;
  char *buffer = NULL;

  for (;;) {
    if (length == capacity) {
      if (capacity == limit) {
        break;
      }
      size_t next = capacity ? capacity : 4096;
      next = next > limit - capacity ? limit : capacity + next;
      buffer = xrealloc(buffer, next + 1);
      capacity = next;
    }
    ssize_t count = read(descriptor, buffer + length, capacity - length);
    if (count < 0) {
      int error = errno;
      free(buffer);
      errno = error;
      return NULL;
    }
    if (!count) {
      break;
    }
    length += (size_t)count;
  }

  if (!buffer) {
    buffer = xmalloc(1);
  }
  buffer[length] = '\0';
  *size = length;
  return buffer;
}

ssize_t full_read(int descriptor, void *buffer, size_t size)
{
  size_t total = 0;
  while (total < size) {
    ssize_t count = read(descriptor, (char *)buffer + total, size - total);
    if (count < 0) {
      return total ? (ssize_t)total : -1;
    }
    if (!count) {
      break;
    }
    total += (size_t)count;
  }
  return (ssize_t)total;
}

ssize_t full_write(int descriptor, const void *buffer, size_t size)
{
  size_t total = 0;
  while (total < size) {
    ssize_t count = write(descriptor, (const char *)buffer + total, size - total);
    if (count < 0) {
      return total ? (ssize_t)total : -1;
    }
    total += (size_t)count;
  }
  return (ssize_t)total;
}

static void llist_append(llist_t **list, char *data)
{
  while (*list) {
    list = &(*list)->link;
  }
  llist_t *node = xmalloc(sizeof(*node));
  node->link = NULL;
  node->data = data;
  *list = node;
}

void *llist_pop(llist_t **list)
{
  llist_t *node = *list;
  if (!node) {
    return NULL;
  }
  void *data = node->data;
  *list = node->link;
  free(node);
  return data;
}

uint32_t getopt32(char **argv, const char *options, ...)
{
  enum { OPTION_MAX = 32 };
  struct {
    char letter;
    bool argument, list;
    void *target;
  } specs[OPTION_MAX];
  size_t spec_count = 0;
  va_list args;

  va_start(args, options);
  for (const char *p = options; *p; ++p) {
    if (spec_count == OPTION_MAX) {
      bb_simple_error_msg_and_die("too many options");
    }
    specs[spec_count].letter = *p;
    specs[spec_count].argument = p[1] == ':';
    specs[spec_count].list = specs[spec_count].argument && p[2] == '*';
    specs[spec_count].target =
        specs[spec_count].argument ? va_arg(args, void *) : NULL;
    p += specs[spec_count].argument + specs[spec_count].list;
    ++spec_count;
  }
  va_end(args);

  uint32_t found = 0;
  for (optind = 1; argv[optind]; ++optind) {
    char *arg = argv[optind];
    if (!strcmp(arg, "--")) {
      ++optind;
      break;
    }
    if (arg[0] != '-' || !arg[1]) {
      break;
    }
    for (char *letter = arg + 1; *letter; ++letter) {
      size_t i = 0;
      while (i < spec_count && specs[i].letter != *letter) {
        ++i;
      }
      if (i == spec_count) {
        bb_show_usage();
      }
      found |= UINT32_C(1) << i;
      if (!specs[i].argument) {
        continue;
      }
      char *value = letter[1] ? letter + 1 : argv[++optind];
      if (!value) {
        bb_show_usage();
      }
      if (specs[i].list) {
        llist_append(specs[i].target, value);
      } else {
        *(char **)specs[i].target = value;
      }
      break;
    }
  }
#ifdef PYXIS_APPLET_LESS
  option_mask32 = found;
#endif
  return found;
}

int index_in_strings(const char *strings, const char *key)
{
  for (int index = 0; *strings; ++index) {
    if (!strcmp(strings, key)) {
      return index;
    }
    strings += strlen(strings) + 1;
  }
  return -1;
}

char *last_char_is(const char *text, int character)
{
  if (!text || !*text) {
    return NULL;
  }
  const char *last = text + strlen(text) - 1;
  return *last == (char)character ? (char *)last : NULL;
}

char *safe_strncpy(char *dest, const char *src, size_t size)
{
  if (!size) {
    return dest;
  }
  size_t length = strnlen(src, size - 1);
  memcpy(dest, src, length);
  dest[length] = '\0';
  return dest;
}

/* POSIX-locale whitespace, as in libbb: space and \t through \r. */
static bool is_libbb_space(char character)
{
  return character == ' ' || (unsigned char)(character - '\t') <= '\r' - '\t';
}

char *skip_whitespace(const char *text)
{
  while (is_libbb_space(*text)) {
    ++text;
  }
  return (char *)text;
}

char *skip_non_whitespace(const char *text)
{
  while (*text && !is_libbb_space(*text)) {
    ++text;
  }
  return (char *)text;
}

/* Like libbb: an empty, signed or trailing-garbage number is an error. */
unsigned bb_strtou(const char *text, char **end, int base)
{
  char *stop;
  if (!isalnum((unsigned char)text[0])) {
    errno = ERANGE;
    return UINT_MAX;
  }
  errno = 0;
  unsigned long value = strtoul(text, &stop, base);
  if (end) {
    *end = stop;
  }
  if (errno || value > UINT_MAX || (*stop && isalnum((unsigned char)*stop))) {
    errno = ERANGE;
    return UINT_MAX;
  }
  if (*stop) {
    errno = EINVAL;
  }
  return (unsigned)value;
}

void pyxis_vi_write(const char *bytes, size_t size)
{
  while (size) {
    if (output_length == sizeof(output)) {
      fflush_all();
    }
    size_t count = sizeof(output) - output_length;
    if (count > size) {
      count = size;
    }
    memcpy(output + output_length, bytes, count);
    output_length += count;
    bytes += count;
    size -= count;
  }
}

void bb_putchar(int character)
{
  char byte = (char)character;
  pyxis_vi_write(&byte, 1);
}

void fputs_stdout(const char *text)
{
  pyxis_vi_write(text, strlen(text));
}

int fflush_all(void)
{
  if (!write_output()) {
    fail(TERMINAL_WRITE_ERROR);
  }
  return 0;
}

int get_terminal_width_height(int descriptor, unsigned *width, unsigned *height)
{
  (void)descriptor;
  size_t columns, rows;
  if (term_size(&terminal, &columns, &rows) != CALL_OK ||
      !columns || !rows || columns > UINT_MAX || rows > UINT_MAX) {
    return -1;
  }
  *width = (unsigned)columns;
  *height = (unsigned)rows;
  return 0;
}

void pyxis_vi_terminal_raw(void)
{
  if (passthrough != HANDLE_INVALID) {
    return;
  }
  if (term_passthrough(&terminal, &passthrough) != CALL_OK) {
    passthrough = HANDLE_INVALID;
    bb_simple_error_msg_and_die(PASSTHROUGH_ERROR);
  }
}

void pyxis_vi_terminal_cooked(void)
{
  if (passthrough == HANDLE_INVALID) {
    return;
  }
  /* A failed withdrawal leaves nothing to retry; process exit withdraws it. */
  handle_close(passthrough);
  passthrough = HANDLE_INVALID;
}

static int64_t translate_key(unsigned key)
{
  switch (key) {
  case TERM_KEY_LEFT: return KEYCODE_LEFT;
  case TERM_KEY_RIGHT: return KEYCODE_RIGHT;
  case TERM_KEY_UP: return KEYCODE_UP;
  case TERM_KEY_DOWN: return KEYCODE_DOWN;
  case TERM_KEY_HOME: return KEYCODE_HOME;
  case TERM_KEY_END: return KEYCODE_END;
  case TERM_KEY_DELETE: return KEYCODE_DELETE;
  case TERM_KEY_PAGE_UP: return KEYCODE_PAGEUP;
  case TERM_KEY_PAGE_DOWN: return KEYCODE_PAGEDOWN;
  default: return key;
  }
}

/* One decoded key, an EOF/error (-1 with errno), or false for no key yet.
 * Negative timeout blocks. Unsupported escape sequences are dropped. */
static bool read_one_key(int timeout, int64_t *key, int *error)
{
  for (;;) {
    unsigned raw;
    enum call_status status = timeout < 0 ?
        term_read_key(&terminal, &raw) :
        term_read_key_timeout(&terminal, (uint32_t)timeout, &raw);
    if (status == CALL_TIMED_OUT) {
      return false;
    }
    if (status != CALL_OK) {
      *key = -1;
      *error = EIO;
      return true;
    }
    if (raw == TERM_KEY_EOF) {
      *key = -1;
      *error = 0;
      return true;
    }
    if (raw != TERM_KEY_UNKNOWN) {
      *key = translate_key(raw);
      *error = 0;
      return true;
    }
  }
}

int pyxis_vi_input_pending(int milliseconds)
{
  if (!key_pending) {
    key_pending = read_one_key(milliseconds, &pending_key, &pending_errno);
  }
  return key_pending;
}

int64_t safe_read_key(int descriptor, char *buffer, int timeout)
{
  (void)descriptor;
  (void)buffer;
  if (!key_pending && !read_one_key(timeout, &pending_key, &pending_errno)) {
    errno = EAGAIN;
    return -1;
  }
  key_pending = false;
  errno = pending_errno;
  return pending_key;
}

bool pyxis_vi_writable(const char *path)
{
  int descriptor = open(path, O_WRONLY);
  if (descriptor < 0) {
    return false;
  }
  close(descriptor);
  return true;
}

bool pyxis_vi_absent(const char *path)
{
  int descriptor = open(path, O_RDONLY);
  if (descriptor >= 0) {
    close(descriptor);
    return false;
  }
  return errno == ENOENT;
}

#ifdef PYXIS_APPLET_LESS
void *bb_realloc_vector(void *pointer, size_t element, unsigned shift, unsigned index)
{
  size_t block = (size_t)1 << shift;
  if (index % block == 0) {
    if ((size_t)index > SIZE_MAX - block || element > SIZE_MAX / (index + block)) {
      bb_simple_error_msg_and_die("pager line count is too large");
    }
    pointer = xrealloc(pointer, ((size_t)index + block) * element);
  }
  return pointer;
}

int bb_console_printf(const char *format, ...)
{
  char *text;
  va_list args;
  va_start(args, format);
  int length = vasprintf(&text, format, args);
  va_end(args);
  if (length < 0) {
    bb_simple_error_msg_and_die("cannot format terminal output");
  }
  pyxis_vi_write(text, (size_t)length);
  free(text);
  return length;
}

int bb_console_puts(const char *text)
{
  fputs_stdout(text);
  bb_putchar('\n');
  return 0;
}
#endif
