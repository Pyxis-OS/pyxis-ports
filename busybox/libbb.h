/* Pyxis replacement for BusyBox's libbb.h, limited to what editors/vi.c uses.
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Names and contracts follow BusyBox libbb; busybox_pyxis.c implements them
 * over Pyxis libc and libterm. The pyxis_vi_* functions are called by this
 * recipe's vi.c patch in place of termios, poll and stat. */
#ifndef PYXIS_BUSYBOX_LIBBB_H
#define PYXIS_BUSYBOX_LIBBB_H

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "vi_config.h"

#define FAST_FUNC
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define NOINLINE __attribute__((noinline))
#define NORETURN __attribute__((noreturn))
#define UNUSED_PARAM __attribute__((unused))
#define ALIGN1 __attribute__((aligned(1)))
#define MAIN_EXTERNALLY_VISIBLE
#define ARRAY_SIZE(x) ((unsigned)(sizeof(x) / sizeof((x)[0])))
#define BB_VER "1.39.0.git"
#define TRUE 1
#define FALSE 0
#define ESC "\033"
#define STRERROR_FMT "%s"
#define STRERROR_ERRNO , strerror(errno)

typedef signed char smallint;
typedef unsigned char smalluint;

/* vi keeps its state in one allocation reached through this pointer. */
extern struct globals *ptr_to_globals;
#define barrier() __asm__ __volatile__("" ::: "memory")
#define SET_PTR_TO_GLOBALS(x) do { ptr_to_globals = (void *)(x); barrier(); } while (0)

/* read_key results: bytes are 0..255; special keys are negative. */
enum {
  KEYCODE_UP = -2,
  KEYCODE_DOWN = -3,
  KEYCODE_RIGHT = -4,
  KEYCODE_LEFT = -5,
  KEYCODE_HOME = -6,
  KEYCODE_END = -7,
  KEYCODE_INSERT = -8,
  KEYCODE_DELETE = -9,
  KEYCODE_PAGEUP = -10,
  KEYCODE_PAGEDOWN = -11,
  KEYCODE_FUN = 0x80000000,
  KEYCODE_CURSOR_POS = -0x100,
  KEYCODE_BUFFER_SIZE = 16,
};

typedef struct llist_t {
  struct llist_t *link;
  char *data;
} llist_t;

extern const char *applet_name;
extern int optind;

/* Allocation failure prints a diagnostic and exits; unsaved edits are lost. */
void *xmalloc(size_t size);
void *xzalloc(size_t size);
void *xrealloc(void *pointer, size_t size);
char *xstrdup(const char *text);
char *xstrndup(const char *text, int limit);
char *xasprintf(const char *format, ...) __attribute__((format(printf, 1, 2)));
/* Keep the old text alive while formatting a replacement from it. */
#define xasprintf_inplace(text, ...) do { \
  char *bb_previous_text = (text); \
  (text) = xasprintf(__VA_ARGS__); \
  free(bb_previous_text); \
} while (0)
/* NULL on success; otherwise an allocated regerror message for the caller. */
char *regcomp_or_errmsg(regex_t *pattern, const char *text, int flags);
/* Read to EOF or *size bytes; *size returns the count. NULL with errno on a
 * read error. The buffer has one spare NUL byte after the data. */
void *xmalloc_read(int descriptor, size_t *size);

void xfunc_die(void) NORETURN;
void bb_simple_error_msg_and_die(const char *message) NORETURN;
void bb_show_usage(void) NORETURN;

/* Retry short transfers until complete, EOF or an error. */
ssize_t full_read(int descriptor, void *buffer, size_t size);
ssize_t full_write(int descriptor, const void *buffer, size_t size);

/* Options: "x" flag, "x:" argument, "x:*" argument appended to an llist_t.
 * Returns option bits in string order; stops at "--" or the first operand. */
uint32_t getopt32(char **argv, const char *options, ...);

int index_in_strings(const char *strings, const char *key);
char *last_char_is(const char *text, int character);
void *llist_pop(llist_t **list);
char *safe_strncpy(char *dest, const char *src, size_t size);
char *skip_whitespace(const char *text);
char *skip_non_whitespace(const char *text);
unsigned bb_strtou(const char *text, char **end, int base);

/* Terminal output is buffered and written to the named output console. */
void bb_putchar(int character);
void fputs_stdout(const char *text);
int fflush_all(void);
/* Dimensions of the named output console; the descriptor is not used. */
int get_terminal_width_height(int descriptor, unsigned *width, unsigned *height);
/* Reads the named input console. Only STDIN_FILENO and timeout -1 (block) or
 * a millisecond bound are supported. -1 with errno EAGAIN on timeout; -1 with
 * errno 0 on input EOF. Unsupported escape sequences are skipped. */
int64_t safe_read_key(int descriptor, char *buffer, int timeout);

void pyxis_vi_write(const char *bytes, size_t size);
void pyxis_vi_terminal_raw(void);
void pyxis_vi_terminal_cooked(void);
/* Wait up to milliseconds for a key, keeping it for safe_read_key. */
int pyxis_vi_input_pending(int milliseconds);
/* Whether the path opens with WRITE authority now, without changing it. */
bool pyxis_vi_writable(const char *path);
/* True only when opening reports ENOENT; any other result counts as present. */
bool pyxis_vi_absent(const char *path);

#endif
