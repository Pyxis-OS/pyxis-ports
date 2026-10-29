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

static enum call_status reply_status(struct syscall_result result, size_t size)
{
  if (result.status >= CALL_STATUS_COUNT ||
      result.reply_size != (result.status == CALL_OK ? size : 0)) {
    return CALL_IO;
  }
  return result.status;
}

static enum call_status set_register(handle_t set, const struct wait_interest *interest,
    uint64_t tag, struct readiness_set_registration *registration)
{
  struct readiness_set_register_request request = {
    .header = {PROTOCOL_READINESS_SET, READINESS_SET_REGISTER},
    .target = interest->handle,
    .allowed_events = interest->events,
    .active_events = interest->events,
    .observed_generation = interest->observed_generation,
    .tag = tag,
  };
  return reply_status(syscall_call(set, &request, sizeof(request), registration,
      sizeof(*registration)), sizeof(*registration));
}

static int set_init(uv_loop_t *loop)
{
  if (loop->signal_set != HANDLE_INVALID) {
    return 0;
  }
  struct readiness_set_create_reply reply;
  enum call_status status = reply_status(syscall_readiness_set_create(&reply,
      sizeof(reply)), sizeof(reply));
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  struct wait_interest wake = {loop->wake_reader, WAIT_READABLE, 0};
  status = set_register(reply.handle, &wake, 0, &loop->wake_registration);
  if (status != CALL_OK) {
    handle_close(reply.handle);
    return uv__pyxis_status(status);
  }
  loop->signal_set = reply.handle;
  return 0;
}

static int remove_sources(uv_loop_t *loop,
    struct readiness_set_registration registrations[2], size_t count)
{
  int error = 0;
  for (size_t index = 0; index < count; ++index) {
    struct readiness_set_remove_request request = {
      .header = {PROTOCOL_READINESS_SET, READINESS_SET_REMOVE},
      .registration = registrations[index],
    };
    enum call_status status = reply_status(syscall_call(loop->signal_set,
        &request, sizeof(request), NULL, 0), 0);
    if (status != CALL_OK && !error) {
      error = uv__pyxis_status(status);
    }
  }
  if (error) {
    /* Retire every borrowed reference even if explicit removal failed. */
    if (handle_close(loop->signal_set)) {
      abort();
    }
    loop->signal_set = HANDLE_INVALID;
  }
  return error;
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
  for (size_t index = 0; index < wait->count; ++index) {
    enum call_status status = set_register(loop->signal_set, &wait->interests[index],
        index + 1, &registrations[index]);
    if (status != CALL_OK) {
      int error = uv__pyxis_status(status);
      int cleanup = remove_sources(loop, registrations, *count);
      *count = 0;
      return cleanup ? cleanup : error;
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
    error = set_init(handle->loop);
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
    error = prepare_sources(handle->loop, &wait, registrations, &count);
    if (!error) {
      error = remove_sources(handle->loop, registrations, count);
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

int uv__pyxis_signal_wait(uv_loop_t *loop, struct wait_interest *interests,
    size_t count, uint64_t deadline, uint64_t *events)
{
  int error = set_init(loop);
  if (error) {
    return error;
  }
  struct pyxis_signal_wait wait;
  struct readiness_set_registration registrations[2];
  size_t registered;
  error = prepare_sources(loop, &wait, registrations, &registered);
  if (error) {
    return error;
  }
  interests[0] = (struct wait_interest){loop->signal_set, WAIT_READABLE | WAIT_CLOSED, 0};
  if (wait.pending || uv__pyxis_signal_pending(loop)) {
    deadline = 0;
  }
  enum call_status status = wait_many(interests, count, deadline, events);
  if (status != CALL_OK && status != CALL_TIMED_OUT) {
    error = uv__pyxis_status(status);
  } else if (events[0] & (WAIT_CLOSED | WAIT_ERROR)) {
    error = UV_EIO;
  } else if (events[0] & WAIT_READABLE) {
    struct readiness_set_event ready[3];
    struct readiness_set_wait_request request = {
      .header = {PROTOCOL_READINESS_SET, READINESS_SET_WAIT},
      .events_address = (uintptr_t)ready, .capacity = 3,
    };
    struct readiness_set_wait_reply reply;
    status = reply_status(syscall_call(loop->signal_set, &request, sizeof(request),
        &reply, sizeof(reply)), sizeof(reply));
    events[0] = 0;
    if (status != CALL_OK || reply.count > 3 || reply.flags) {
      error = status != CALL_OK ? uv__pyxis_status(status) : UV_EIO;
    } else {
      for (size_t index = 0; index < reply.count; ++index) {
        struct readiness_set_event *event = &ready[index];
        const struct readiness_set_registration *expected = event->tag == 0 ?
            &loop->wake_registration : event->tag <= registered ?
            &registrations[event->tag - 1] : NULL;
        if (!expected || event->registration.identity != expected->identity ||
            event->registration.generation != expected->generation ||
            event->status != CALL_OK ||
            (event->events & (WAIT_ERROR | WAIT_CLOSED | WAIT_PEER_FIN))) {
          error = UV_EIO;
          break;
        }
        if (event->tag == 0) {
          events[0] = event->events;
        }
      }
    }
  }
  int cleanup = remove_sources(loop, registrations, registered);
  if (cleanup) {
    return cleanup;
  }
  if (error) {
    return error;
  }
  /* No borrowed source or registration survives a handler or application call. */
  if (pyxis_signal_checkpoint()) {
    return uv_translate_sys_error(errno);
  }
  return 0;
}
