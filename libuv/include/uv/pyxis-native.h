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
 * A record replaces an inherited capability of the same name. Names must be
 * unique; memory, clock, launcher and script are reserved. Arrays, names and
 * source handles are borrowed until return; source grants survive all outcomes.
 * Native launch capture/startup budgets apply. Close process after failure too. */
UV_EXTERN int uv_pyxis_spawn(uv_loop_t *loop, uv_process_t *process,
    const uv_process_options_t *options, const uv_pyxis_resource_t *resources,
    size_t resource_count);

struct command_selection;

/* The exact successful name/path selection, borrowed until process close.
 * Includes full bundle policy and fixed package provenance; NULL for native
 * image-backed jobs and unsuccessful spawns. Copy resources to retain views. */
UV_EXTERN const struct command_selection *uv_pyxis_process_selection(
    const uv_process_t *process);

struct pyxis_job_options;

/* Exact native job inventory, with libc FILE/descriptor preparation and audit.
 * Inputs survive all outcomes. Caller prepares any PIPE endpoints separately.
 * Close process after failure too; close only detaches the owned observer.
 * Native uncertain admission maps to UV_EIO; never automatically replay. */
UV_EXTERN int uv_pyxis_spawn_job(uv_loop_t *loop, uv_process_t *process,
    handle_t launcher, const struct pyxis_job_options *command,
    uv_exit_cb exit_cb);

/* Requests termination through this handle's observer, without awaiting cleanup.
 * Completion remains the exit callback and native process->exit_reason. */
UV_EXTERN int uv_pyxis_process_terminate(uv_process_t *process);

#ifdef __cplusplus
}
#endif

#endif
