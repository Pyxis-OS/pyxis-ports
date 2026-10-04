/* Pyxis services for Links' platform layer (pyxis.c). Kept apart from
 * links.h, because Links and libterm both define struct terminal.
 *
 * Part of the Pyxis port of Links; distributed under the GPL like Links
 * (see COPYING in the Links source). */
#ifndef LINKS_PYXIS_CONSOLE_H
#define LINKS_PYXIS_CONSOLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Bind the named input and output console grants. False if either is absent. */
bool pyxis_console_open(void);
/* Wait up to timeout_ms for input (negative waits without a bound). Returns
 * the byte count, 0 at input EOF, -1 on timeout, or -2 with errno on error. */
long pyxis_console_read(void *bytes, size_t capacity, long timeout_ms);
/* Write every byte; false with errno on failure. */
bool pyxis_console_write(const void *bytes, size_t size);
/* Character cells of the output console; false if it cannot report them. */
bool pyxis_console_size(int *columns, int *rows);
/* While held, Ctrl+C reaches Links as input instead of ending it. */
void pyxis_console_hold_ctrl_c(bool hold);

/* Monotonic milliseconds from the named clock; false if it is unavailable. */
bool pyxis_clock_ms(uint64_t *milliseconds);
bool pyxis_sleep_ms(uint64_t milliseconds);

/* Whether scheme names one of this program's startup roots, as host does in
 * host://. Other schemes reach namespace providers. */
bool pyxis_scheme_is_root(const char *scheme);

#endif
