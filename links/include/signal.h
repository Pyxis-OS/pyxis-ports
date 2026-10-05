/* Port-local: Pyxis has no signals. Links' Pyxis block defines
 * NO_SIGNAL_HANDLERS, so these numbers only index Links' own handler table. */
#ifndef LINKS_PYXIS_SIGNAL_H
#define LINKS_PYXIS_SIGNAL_H

#include <sys/types.h>

typedef int pid_t;

#define SIGHUP 1
#define SIGINT 2
#define SIGPIPE 13
#define SIGTERM 15

#endif
