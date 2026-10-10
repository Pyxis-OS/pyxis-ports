#ifndef UV_PYXIS_INTERNAL_H
#define UV_PYXIS_INTERNAL_H

#include "uv.h"
#include "uv/pyxis-native.h"
#include "uv-common.h"
#include <abi/syscall.h>
#include <abi/wait.h>
#include <pyxis/descriptor.h>

int uv__pyxis_status(enum call_status status);
/* Copies TEXT for libuv's buffer/size convention: UV_ENOBUFS with the needed
 * size, otherwise the length without the NUL. */
int uv__pyxis_copy_string(const char *text, char *buffer, size_t *size);
int uv__pyxis_reserve(uv_loop_t *loop, unsigned count);
void uv__pyxis_release(uv_loop_t *loop, unsigned count);
void uv__pyxis_stream_init(uv_loop_t *loop, uv_stream_t *stream, uv_handle_type type);
void uv__pyxis_stream_close(uv_stream_t *stream);
void uv__pyxis_stream_dispatch(uv_stream_t *stream, uint64_t events);
int uv__pyxis_stream_events(uv_stream_t *stream, struct wait_interest *interest);
void uv__pyxis_process_close(uv_process_t *process);
void uv__pyxis_process_dispatch(uv_process_t *process);
void uv__run_prepare(uv_loop_t *loop);
void uv__run_check(uv_loop_t *loop);
void uv__run_idle(uv_loop_t *loop);

#endif
