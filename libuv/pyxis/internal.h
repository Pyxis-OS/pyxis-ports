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
#define UV__PYXIS_SIGNAL_TAG(index) (UINT64_MAX - (index))

struct uv__pyxis_watch {
  uv_loop_t *loop;
  uv_handle_t *owner; /* NULL only for the wake pipe. */
  handle_t target;
  uint64_t tag;
  uint64_t allowed;
  uint64_t active;
  struct readiness_set_registration registration;
  int error;
  /* Closing or restarting unlinks queued work before callback-visible release. */
  struct uv__queue ready_queue;
  unsigned queued;
};

struct uv__pyxis_backend {
  handle_t set;
  uint64_t epoch;
  unsigned rebuild;
  struct uv__pyxis_watch **watches;
  size_t capacity;
  struct uv__queue ready_queue;
  struct uv__queue dispatch_queue;
  struct uv__pyxis_watch *wake;
};

int uv__pyxis_backend_init(uv_loop_t *loop);
void uv__pyxis_backend_close(uv_loop_t *loop);
int uv__pyxis_backend_ensure(uv_loop_t *loop, struct call_limit *limit);
void uv__pyxis_backend_reset(uv_loop_t *loop);
int uv__pyxis_watch_new(uv_loop_t *loop, uv_handle_t *owner, handle_t target,
    uint64_t allowed, uint64_t active, struct call_limit *limit,
    struct uv__pyxis_watch **out);
void uv__pyxis_watch_bind(struct uv__pyxis_watch *watch, handle_t target,
    struct readiness_set_registration registration);
int uv__pyxis_watch_update(struct uv__pyxis_watch *watch, uint64_t active);
void uv__pyxis_watch_close(struct uv__pyxis_watch **watch);
void uv__pyxis_watch_ready(struct uv__pyxis_watch *watch);
void uv__pyxis_watch_unready(struct uv__pyxis_watch *watch);
struct uv__pyxis_watch *uv__pyxis_watch_find(uv_loop_t *loop,
    const struct readiness_set_event *event, uint64_t epoch);
int uv__pyxis_set_register(uv_loop_t *loop, const struct wait_interest *interest,
    uint64_t allowed, uint64_t tag, struct readiness_set_registration *registration,
    struct call_limit *limit);
void uv__pyxis_set_remove(uv_loop_t *loop,
    struct readiness_set_registration registration);
int uv__pyxis_set_wait(uv_loop_t *loop, uint64_t deadline,
    struct readiness_set_event *events, size_t *count);
void uv__pyxis_stream_init(uv_loop_t *loop, uv_stream_t *stream, uv_handle_type type);
void uv__pyxis_stream_close(uv_stream_t *stream);
int uv__pyxis_stream_dispatch(uv_stream_t *stream, uint64_t events);
int uv__pyxis_stream_refresh(uv_stream_t *stream);
void uv__pyxis_process_close(uv_process_t *process);
int uv__pyxis_process_dispatch(uv_process_t *process);
int uv__pyxis_signal_pending(const uv_loop_t *loop);
void uv__pyxis_signal_dispatch(uv_loop_t *loop);
int uv__pyxis_signal_wait(uv_loop_t *loop, uint64_t deadline,
    struct readiness_set_event *events, size_t *count, uint64_t *epoch);
void uv__run_prepare(uv_loop_t *loop);
void uv__run_check(uv_loop_t *loop);
void uv__run_idle(uv_loop_t *loop);

#endif
