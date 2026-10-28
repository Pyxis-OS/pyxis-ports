/* Pyxis platform adapter for BusyBox vi/less: entry points and libbb helpers.
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * vi draws on the named output console and reads the named input console
 * through libc descriptors. Those grants are separate from stdio, so
 * stdout/stderr only carry usage text and fatal diagnostics. */
#include <handle.h>
#include <stdarg.h>
#include <startup.h>
#include <environment.h>
#include <term.h>
#include <pyxis/descriptor.h>
#include <pyxis/terminal.h>
#include "libbb.h"

int vi_main(int argc, char **argv);

struct globals *ptr_to_globals;
#ifdef PYXIS_APPLET_LESS
int less_main(int argc, char **argv);
const char *applet_name = "less";
char bb_common_bufsiz1[4096];
#define ALLOCATION_ERROR "out of memory"
#define TERMINAL_WRITE_ERROR "cannot write terminal output"
#define PASSTHROUGH_ERROR "cannot keep Ctrl+C as pager input"
#else
const char *applet_name = "vi";
#define ALLOCATION_ERROR "out of memory; unsaved edits are lost"
#define TERMINAL_WRITE_ERROR "cannot write terminal output; unsaved edits are lost"
#define PASSTHROUGH_ERROR "cannot keep Ctrl+C as editor input"
#endif

static struct terminal terminal;
static int input_fd = -1;
static struct pyxis_terminal_state *raw_state;

/* Screen updates are collected and written when vi waits for input. */
static char output[4096];
static size_t output_length;

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

void bb_error_msg_and_die(const char *format, ...)
{
  va_list arguments;
  write_output();
  fprintf(stderr, "%s: ", applet_name);
  va_start(arguments, format);
  vfprintf(stderr, format, arguments);
  va_end(arguments);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

void xfunc_die(void)
{
  write_output();
  exit(EXIT_FAILURE);
}

void bb_show_usage(void)
{
#ifdef PYXIS_APPLET_LESS
  fputs("Usage: less [-EMmN~IF] [FILE]...\n", stderr);
#else
  fputs("Usage: vi [-c CMD] [-R] [-H] [FILE]...\n", stderr);
#endif
  exit(EXIT_FAILURE);
}

/* BusyBox appletlib.c helper; applet dispatch itself is not built. */
unsigned FAST_FUNC string_array_len(char **argv)
{
	char **start = argv;

	while (*argv)
		argv++;

	return argv - start;
}

int main(int argc, char **argv)
{
  handle_t input = HANDLE_INVALID;
  environment_capability(startup_environment_handle(), "input", &input);
  if (input != HANDLE_INVALID) {
    input_fd = pyxis_descriptor_adopt(&input, PYXIS_DESCRIPTOR_READ);
    if (input_fd < 0) {
      handle_close(input);
      fail("cannot adopt input console");
    }
  }
  environment_capability(startup_environment_handle(), "output", &terminal.output);
#ifndef PYXIS_APPLET_LESS
  if (input_fd < 0 || terminal.output == HANDLE_INVALID) {
    fail("input and output console capabilities are required");
  }
#endif

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

char *regcomp_or_errmsg(regex_t *pattern, const char *text, int flags)
{
  int error = regcomp(pattern, text, flags);
  if (!error) {
    return NULL;
  }
  size_t size = regerror(error, pattern, NULL, 0);
  char *message = xmalloc(size);
  regerror(error, pattern, message, size);
  return message;
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

int pyxis_vi_input_fd(void)
{
  return input_fd;
}

void pyxis_vi_terminal_raw(void)
{
  if (raw_state) {
    return;
  }
  if (pyxis_terminal_make_raw(input_fd, &raw_state) < 0) {
    bb_simple_error_msg_and_die(PASSTHROUGH_ERROR);
  }
}

void pyxis_vi_terminal_cooked(void)
{
  if (!raw_state) {
    return;
  }
  /* A failed withdrawal leaves nothing to retry; process exit withdraws it. */
  pyxis_terminal_restore(input_fd, raw_state);
  pyxis_terminal_state_free(raw_state);
  raw_state = NULL;
}

/* Selected libbb/xfuncs.c helpers, unchanged from the pinned upstream. */
int FAST_FUNC ndelay_on(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	if (flags & O_NONBLOCK)
		return flags;
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	return flags;
}

int FAST_FUNC ndelay_off(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	if (!(flags & O_NONBLOCK))
		return flags;
	fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
	return flags;
}

#ifdef PYXIS_APPLET_LESS
int bb_cat(char **paths)
{
  int result = EXIT_SUCCESS;
  do {
    int fd = *paths && strcmp(*paths, "-") ? open(*paths, O_RDONLY) : STDIN_FILENO;
    if (fd < 0) {
      result = EXIT_FAILURE;
    } else {
      char bytes[4096];
      ssize_t count;
      while ((count = read(fd, bytes, sizeof(bytes))) > 0) {
        if (full_write(STDOUT_FILENO, bytes, (size_t)count) != count) {
          result = EXIT_FAILURE;
          break;
        }
      }
      if (count < 0)
        result = EXIT_FAILURE;
      if (fd != STDIN_FILENO)
        close(fd);
    }
    if (*paths)
      paths++;
  } while (*paths);
  return result;
}

FILE *fopen_for_write(const char *path)
{
  return fopen(path, "w");
}
#endif

#ifdef PYXIS_APPLET_LESS
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
