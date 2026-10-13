/* Selected libbb interfaces for the ordinary BusyBox printf applet.
 * SPDX-License-Identifier: GPL-2.0-only */
#ifndef PYXIS_BUSYBOX_PRINTF_SUPPORT_H
#define PYXIS_BUSYBOX_PRINTF_SUPPORT_H

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAST_FUNC
#define UNUSED_PARAM __attribute__((unused))
#define ALIGN1 __attribute__((aligned(1)))
#define ENABLE_ASH_PRINTF 0
#define ENABLE_HUSH_PRINTF 0
#define bb_putchar putchar

extern const char *applet_name;

void bb_error_msg(const char *format, ...) __attribute__((format(printf, 1, 2)));
void bb_simple_error_msg(const char *message);
void bb_show_usage(void) __attribute__((noreturn));
void *xmalloc(size_t size);
char *utoa(unsigned value);
char **skip_dash_dash(char **argv);
char *skip_whitespace(const char *text);
void overlapping_strcpy(char *dest, const char *source);
char bb_process_escape_sequence(const char **text);
unsigned long long bb_strtoull(const char *text, char **end, int base);
long long bb_strtoll(const char *text, char **end, int base);
int bb_strtoi(const char *text, char **end, int base);

#endif
