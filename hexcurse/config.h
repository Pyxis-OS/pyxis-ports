#ifndef PYXIS_HEXCURSE_CONFIG_H
#define PYXIS_HEXCURSE_CONFIG_H

/* The SDK supplies these headers and 64-bit file offsets. Terminal resize and
 * fault handling use the native curses/process boundary, not Unix signals. */
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_FSEEKO 1
#define STDC_HEADERS 1
#define VERSION "1.60.0"

#endif
