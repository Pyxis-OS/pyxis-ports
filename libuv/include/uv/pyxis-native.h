#ifndef UV_PYXIS_NATIVE_H
#define UV_PYXIS_NATIVE_H

#include "uv.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *name;
  handle_t source;
  uint64_t rights;
  uint64_t transport;
} uv_pyxis_resource_t;

/* Additional child resources; memory/clock/launcher retain ordinary attenuation.
 * Names memory, clock, launcher and script are reserved. Arrays, names and
 * source handles are borrowed until return; source grants survive all outcomes.
 * Native launch capture/startup budgets apply. Close process after failure too. */
UV_EXTERN int uv_pyxis_spawn(uv_loop_t *loop, uv_process_t *process,
    const uv_process_options_t *options, const uv_pyxis_resource_t *resources,
    size_t resource_count);

/* Requests termination through this handle's observer, without awaiting cleanup.
 * Completion remains the exit callback and native process->exit_reason. */
UV_EXTERN int uv_pyxis_process_terminate(uv_process_t *process);

typedef void (*uv_pyxis_tty_resize_cb)(uv_tty_t *handle, int status,
    int width, int height);

/* Start snapshots current geometry; callbacks report later generation changes.
 * Errors stop watching and supply zero dimensions. The existing TTY wait slot
 * observes resize alongside stream I/O; close withdraws the callback. */
UV_EXTERN int uv_pyxis_tty_resize_start(uv_tty_t *handle,
    uv_pyxis_tty_resize_cb callback);
UV_EXTERN int uv_pyxis_tty_resize_stop(uv_tty_t *handle);

#ifdef __cplusplus
}
#endif

#endif
