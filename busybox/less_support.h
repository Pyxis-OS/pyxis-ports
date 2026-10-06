/* Helpers used by the selected BusyBox less source.
 * SPDX-License-Identifier: GPL-2.0-only */
#ifndef PYXIS_BUSYBOX_LESS_SUPPORT_H
#define PYXIS_BUSYBOX_LESS_SUPPORT_H

#include <sys/stat.h>
#include "libbb.h"
#include "less_config.h"

#define COMMON_BUFSIZE 4096
extern char bb_common_bufsiz1[COMMON_BUFSIZE];
extern uint32_t option_mask32;
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define xrealloc_vector(pointer, shift, index) \
  bb_realloc_vector((pointer), sizeof(*(pointer)), (shift), (index))
void *bb_realloc_vector(void *pointer, size_t element, unsigned shift, unsigned index);
int bb_console_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
int bb_console_puts(const char *text);

/* All pager drawing goes to its named output console, separately from stdout. */
#define printf bb_console_printf
#define puts bb_console_puts
#define bb_msg_standard_input "standard input"
#define bb_msg_read_error "read error"

#endif
