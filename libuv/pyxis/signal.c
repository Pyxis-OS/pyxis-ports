#include "internal.h"
#include <errno.h>
#include <handle.h>
#include <pyxis/signal.h>
#include <signal.h>
#include <stdlib.h>
#include <syscall.h>
#include <wait.h>

static uv_loop_t *signal_owner;

static int native_signal(int signum)
{
  return signum == SIGWINCH || signum == SIGINT;
}

static int block_signals(sigset_t *previous)
{
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGWINCH);
  sigaddset(&mask, SIGINT);
  return sigprocmask(SIG_BLOCK, &mask, previous) ? uv_translate_sys_error(errno) : 0;
}

static void restore_mask(const sigset_t *previous)
{
  if (sigprocmask(SIG_SETMASK, previous, NULL)) {
    abort();
  }
}

static void signal_received(int signum)
{
  if (!signal_owner) {
    return;
  }
  struct uv__queue *entry;
  uv__queue_foreach(entry, &signal_owner->signal_handles) {
    uv_signal_t *handle = uv__queue_data(entry, uv_signal_t, queue);
    if (handle->signum == signum) {
      handle->pending = 1;
    }
  }
}

static int other_watcher(uv_loop_t *loop, const uv_signal_t *excluded, int signum)
{
  struct uv__queue *entry;
  uv__queue_foreach(entry, &loop->signal_handles) {
    uv_signal_t *handle = uv__queue_data(entry, uv_signal_t, queue);
    if (handle != excluded && handle->signum == signum) {
      return 1;
    }
  }
  return 0;
}

static void remove_sources(uv_loop_t *loop,
    struct readiness_set_registration registrations[2], size_t count,
    uint64_t epoch)
{
  for (size_t index = 0; index < count && epoch == loop->backend->epoch; ++index) {
    uv__pyxis_set_remove(loop, registrations[index]);
  }
}

static int prepare_sources(uv_loop_t *loop, struct pyxis_signal_wait *wait,
    struct readiness_set_registration registrations[2], size_t *count)
{
  *count = 0;
  if (pyxis_signal_wait_prepare(wait)) {
    return uv_translate_sys_error(errno);
  }
  if (wait->count > 2) {
    return UV_EIO;
  }
  uint64_t epoch = loop->backend->epoch;
  for (size_t index = 0; index < wait->count; ++index) {
    int error = uv__pyxis_set_register(loop, &wait->interests[index],
        wait->interests[index].events, UV__PYXIS_SIGNAL_TAG(index),
        &registrations[index], NULL);
    if (error) {
      remove_sources(loop, registrations, *count, epoch);
      *count = 0;
      return error;
    }
    ++*count;
  }
  return 0;
}

static int stop_locked(uv_signal_t *handle)
{
  if (!handle->signum) {
    return 0;
  }
  if (native_signal(handle->signum) &&
      !other_watcher(handle->loop, handle, handle->signum)) {
    struct sigaction action = {.sa_handler = SIG_DFL};
    if (sigaction(handle->signum, &action, NULL)) {
      return uv_translate_sys_error(errno);
    }
  }
  uv__queue_remove(&handle->queue);
  uv__queue_init(&handle->queue);
  handle->signum = 0;
  handle->pending = 0;
  handle->dispatch_pending = 0;
  uv__handle_stop(handle);
  if (uv__queue_empty(&handle->loop->signal_handles)) {
    signal_owner = NULL;
  }
  return 0;
}

int uv_signal_init(uv_loop_t *loop, uv_signal_t *handle)
{
  struct pyxis_signal_wait wait;
  if (pyxis_signal_wait_prepare(&wait)) {
    return uv_translate_sys_error(errno);
  }
  uv__handle_init(loop, (uv_handle_t *)handle, UV_SIGNAL);
  uv__queue_init(&handle->queue);
  handle->signal_cb = NULL;
  handle->signum = 0;
  handle->pending = 0;
  handle->dispatch_pending = 0;
  return 0;
}

int uv_signal_start(uv_signal_t *handle, uv_signal_cb callback, int signum)
{
  sigset_t previous;
  int error = block_signals(&previous);
  if (error) {
    return error;
  }
  if (!callback || uv__is_closing(handle) || signum <= 0 || signum >= NSIG ||
      signum == SIGKILL || signum == SIGSTOP) {
    error = UV_EINVAL;
    goto done;
  }
  if (signal_owner && signal_owner != handle->loop) {
    error = UV_EBUSY;
    goto done;
  }
  if (handle->signum == signum) {
    handle->signal_cb = callback;
    goto done;
  }
  if (native_signal(signum)) {
    error = uv__pyxis_backend_ensure(handle->loop, NULL);
    if (error) {
      goto done;
    }
  }
  error = stop_locked(handle);
  if (error) {
    goto done;
  }
  if (native_signal(signum)) {
    int installed = !other_watcher(handle->loop, NULL, signum);
    if (installed) {
      struct sigaction action = {.sa_handler = signal_received, .sa_flags = SA_RESTART};
      if (sigaction(signum, &action, NULL)) {
        error = uv_translate_sys_error(errno);
        goto done;
      }
    }
    /* Admission includes the native registrations, before publishing a watcher. */
    struct pyxis_signal_wait wait;
    struct readiness_set_registration registrations[2];
    size_t count;
    uint64_t epoch = handle->loop->backend->epoch;
    error = prepare_sources(handle->loop, &wait, registrations, &count);
    if (!error) {
      remove_sources(handle->loop, registrations, count, epoch);
    }
    if (error) {
      if (installed) {
        struct sigaction action = {.sa_handler = SIG_DFL};
        if (sigaction(signum, &action, NULL)) {
          abort();
        }
      }
      goto done;
    }
  }
  handle->signal_cb = callback;
  handle->signum = signum;
  handle->pending = 0;
  handle->dispatch_pending = 0;
  uv__queue_insert_tail(&handle->loop->signal_handles, &handle->queue);
  signal_owner = handle->loop;
  uv__handle_start(handle);

done:
  restore_mask(&previous);
  return error;
}

int uv_signal_start_oneshot(uv_signal_t *handle, uv_signal_cb callback, int signum)
{
  (void)handle;
  (void)callback;
  (void)signum;
  struct pyxis_signal_wait wait;
  if (pyxis_signal_wait_prepare(&wait)) {
    return uv_translate_sys_error(errno);
  }
  return UV_ENOSYS;
}

int uv_signal_stop(uv_signal_t *handle)
{
  sigset_t previous;
  int error = block_signals(&previous);
  if (error) {
    return error;
  }
  error = stop_locked(handle);
  restore_mask(&previous);
  return error;
}

int uv__pyxis_signal_pending(const uv_loop_t *loop)
{
  struct uv__queue *entry;
  uv__queue_foreach(entry, &loop->signal_handles) {
    uv_signal_t *handle = uv__queue_data(entry, uv_signal_t, queue);
    if (handle->pending) {
      return 1;
    }
  }
  return 0;
}

void uv__pyxis_signal_dispatch(uv_loop_t *loop)
{
  sigset_t previous;
  if (block_signals(&previous)) {
    abort();
  }
  struct uv__queue *entry;
  uv__queue_foreach(entry, &loop->signal_handles) {
    uv_signal_t *handle = uv__queue_data(entry, uv_signal_t, queue);
    handle->dispatch_pending = handle->pending;
    handle->pending = 0;
  }
  restore_mask(&previous);
  for (;;) {
    uv_signal_t *ready = NULL;
    uv__queue_foreach(entry, &loop->signal_handles) {
      uv_signal_t *handle = uv__queue_data(entry, uv_signal_t, queue);
      if (handle->dispatch_pending) {
        ready = handle;
        break;
      }
    }
    if (!ready) {
      break;
    }
    ready->dispatch_pending = 0;
    /* Rescan after callbacks: another watcher may have been stopped/closed. */
    ready->signal_cb(ready, ready->signum);
  }
}

int uv__pyxis_signal_wait(uv_loop_t *loop, uint64_t deadline,
    struct readiness_set_event *events, size_t *count, uint64_t *epoch)
{
  *epoch = loop->backend->epoch;
  *count = 0;
  struct pyxis_signal_wait wait;
  struct readiness_set_registration registrations[2];
  size_t registered;
  int error = prepare_sources(loop, &wait, registrations, &registered);
  if (error) {
    return error;
  }
  if (wait.pending || uv__pyxis_signal_pending(loop)) {
    deadline = 0;
  }
  error = uv__pyxis_set_wait(loop, deadline, events, count);
  if (!error) {
    size_t retained = 0;
    for (size_t index = 0; index < *count; ++index) {
      struct readiness_set_event event = events[index];
      if (event.tag >= UV__PYXIS_SIGNAL_TAG(1)) {
        size_t source = UINT64_MAX - event.tag;
        if (source >= registered ||
            event.registration.identity != registrations[source].identity ||
            event.registration.generation != registrations[source].generation ||
            event.status != CALL_OK ||
            (event.events & (WAIT_ERROR | WAIT_CLOSED | WAIT_PEER_FIN))) {
          error = UV_EIO;
          break;
        }
      } else {
        events[retained++] = event;
      }
    }
    *count = retained;
  }
  remove_sources(loop, registrations, registered, *epoch);
  if (error) {
    return error;
  }
  if (*epoch != loop->backend->epoch) {
    *count = 0;
  }
  /* No borrowed source or registration survives a handler or application call. */
  if (pyxis_signal_checkpoint()) {
    return uv_translate_sys_error(errno);
  }
  return 0;
}
