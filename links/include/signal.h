/* Port-local: Pyxis has no signals. Links' Pyxis block defines
 * NO_SIGNAL_HANDLERS, so these numbers only index Links' own handler table.
 * This directory precedes the SDK on the include path; libc's signal.h
 * supplies the ISO C part and this header adds what Links expects of POSIX. */
#ifndef LINKS_PYXIS_SIGNAL_H
#define LINKS_PYXIS_SIGNAL_H

#include_next <signal.h>
#include <sys/types.h>

typedef int pid_t;

#define SIGHUP 1
#define SIGPIPE 13

#endif
