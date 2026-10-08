#ifndef QUAKE_PYXIS_H
#define QUAKE_PYXIS_H

/* Boundary between the engine-side system layer (gnu99, Quake headers) and the
 * Pyxis session side (C23, Pyxis headers). Plain C types only. */

/* Acquire display, keyboard and optional pointer sessions and present.
 * Exits with a message on failure. */
void pyxis_quake_start(void);
/* Release every acquired session. Safe to repeat; process exit also releases. */
void pyxis_quake_stop(void);
/* Elapsed monotonic seconds since start, independent of input focus. */
double pyxis_quake_time(void);
/* Sleep until the given elapsed time, measured like pyxis_quake_time(). */
void pyxis_quake_sleep_until(double seconds);

#endif
