/* Selected BusyBox libbb declarations for standalone Pyxis applets.
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Upstream sources implement option, list, numeric and string helpers.
 * busybox_pyxis.c supplies the native console backend. The pyxis_vi_* hooks
 * preserve named-console ownership in the selected editor/pager sources. */
#ifndef PYXIS_BUSYBOX_LIBBB_H
#define PYXIS_BUSYBOX_LIBBB_H

#ifdef PYXIS_APPLET_TAR
#include "tar_support.h"
#else
#include <alloca.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <regex.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vi_config.h"

#define FAST_FUNC
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define NOINLINE __attribute__((noinline))
#define NORETURN __attribute__((noreturn))
#define UNUSED_PARAM __attribute__((unused))
/* Pinned platform.h selects this form for the SDK's Clang GNU compatibility. */
#define UNINITIALIZED_VAR(var,zero) var = var
#define ALIGN_PTR __attribute__((aligned(sizeof(void *))))
#define ALIGN_SUFFIX __attribute__((aligned(4)))
#define PUSH_AND_SET_FUNCTION_VISIBILITY_TO_HIDDEN
#define POP_SAVED_FUNCTION_VISIBILITY
#define GETOPT_RESET() (optind = 0)
#define ENABLE_LONG_OPTS 0
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
  KEYCODE_BACKSPACE = -12,
  KEYCODE_D = -13,
  KEYCODE_CTRL_RIGHT = KEYCODE_RIGHT & ~0x40,
  KEYCODE_CTRL_LEFT = KEYCODE_LEFT & ~0x40,
  KEYCODE_ALT_RIGHT = KEYCODE_RIGHT & ~0x20,
  KEYCODE_ALT_LEFT = KEYCODE_LEFT & ~0x20,
  KEYCODE_ALT_BACKSPACE = KEYCODE_BACKSPACE & ~0x20,
  KEYCODE_ALT_D = KEYCODE_D & ~0x20,
  KEYCODE_CURSOR_POS = -0x100,
  KEYCODE_BUFFER_SIZE = 16,
};

typedef struct llist_t {
  struct llist_t *link;
  char *data;
} llist_t;

extern const char *applet_name;
extern uint32_t option_mask32;
void bb_error_msg_and_die(const char *format, ...) NORETURN;
void llist_add_to(llist_t **list, void *data);
void llist_add_to_end(llist_t **list, void *data);
void llist_unlink(llist_t **head, llist_t *elm);
void llist_free(llist_t *elm, void (*freeit)(void *));
llist_t *llist_rev(llist_t *list);
llist_t *llist_find_str(llist_t *list, const char *str);
unsigned string_array_len(char **argv);
void overlapping_strcpy(char *dest, const char *source);
struct suffix_mult {
  char suffix[4];
  uint32_t mult;
};
#include "xatonum.h"
int xatoi_positive(const char *text);

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

void xfunc_die(void) NORETURN;
void bb_simple_error_msg_and_die(const char *message) NORETURN;
void bb_show_usage(void) NORETURN;

/* Retry short transfers until complete, EOF or an error. */
ssize_t full_read(int descriptor, void *buffer, size_t size);
ssize_t safe_read(int descriptor, void *buffer, size_t size);
int safe_poll(struct pollfd *descriptors, nfds_t count, int timeout);
int check_got_signal_and_poll(struct pollfd descriptors[1], int timeout);
void bb_simple_perror_msg(const char *message);
int ndelay_on(int descriptor);
int ndelay_off(int descriptor);
ssize_t full_write(int descriptor, const void *buffer, size_t size);

/* Upstream getopt32, using libc getopt and its process-wide parser state. */
uint32_t getopt32(char **argv, const char *options, ...);

char *is_prefixed_with(const char *text, const char *prefix);
int index_in_strings(const char *strings, const char *key);
char *last_char_is(const char *text, int character);
void *llist_pop(llist_t **list);
char *safe_strncpy(char *dest, const char *src, size_t size);
char *skip_whitespace(const char *text);
char *skip_non_whitespace(const char *text);

/* Terminal output is buffered and written to the named output console. */
void bb_putchar(int character);
void fputs_stdout(const char *text);
int fflush_all(void);
/* Dimensions of the named output console; the descriptor is not used. */
int get_terminal_width_height(int descriptor, unsigned *width, unsigned *height);
/* Upstream decoder over the supplied keyboard descriptor. */
int64_t safe_read_key(int descriptor, char *buffer, int timeout);

void pyxis_vi_write(const char *bytes, size_t size);
void pyxis_vi_terminal_raw(void);
void pyxis_vi_terminal_cooked(void);
/* Owned named input, distinct from content stdin. */
int pyxis_vi_input_fd(void);
int bb_cat(char **paths);
FILE *fopen_for_write(const char *path);

#endif /* PYXIS_APPLET_TAR */
#endif
