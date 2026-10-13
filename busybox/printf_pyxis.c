/* Standalone BusyBox printf entry point and selected libbb helpers over libc.
 * SPDX-License-Identifier: GPL-2.0-only */
#include <stdarg.h>
#include "printf_support.h"

const char *applet_name = "printf";

int printf_main(int argc, char **argv);

int main(int argc, char **argv)
{
  int status = printf_main(argc, argv);
  if (fflush(stdout) == EOF || ferror(stdout)) {
    bb_simple_error_msg("cannot write output");
    return EXIT_FAILURE;
  }
  return status;
}

void bb_error_msg(const char *format, ...)
{
  int error = errno;
  va_list args;
  va_start(args, format);
  fprintf(stderr, "%s: ", applet_name);
  vfprintf(stderr, format, args);
  fputc('\n', stderr);
  va_end(args);
  errno = error;
}

void bb_simple_error_msg(const char *message)
{
  bb_error_msg("%s", message);
}

void bb_show_usage(void)
{
  fputs("Usage: printf FORMAT [ARG]...\n", stderr);
  exit(EXIT_FAILURE);
}

void *xmalloc(size_t size)
{
  void *pointer = malloc(size ? size : 1);
  if (!pointer) {
    bb_simple_error_msg("out of memory");
    exit(EXIT_FAILURE);
  }
  return pointer;
}

char *utoa(unsigned value)
{
  static char buffer[sizeof(value) * 3 + 1];
  snprintf(buffer, sizeof(buffer), "%u", value);
  return buffer;
}

char **skip_dash_dash(char **argv)
{
  ++argv;
  if (*argv && !strcmp(*argv, "--")) {
    ++argv;
  }
  return argv;
}

char *skip_whitespace(const char *text)
{
  while (*text == ' ' || (unsigned char)(*text - '\t') <= '\r' - '\t') {
    ++text;
  }
  return (char *)text;
}

void overlapping_strcpy(char *dest, const char *source)
{
  memmove(dest, source, strlen(source) + 1);
}
