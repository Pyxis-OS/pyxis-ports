#include "internal.h"
#include <handle.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>

static enum call_status reply_status(struct syscall_result result, size_t size)
{
  if (result.status >= CALL_STATUS_COUNT ||
      result.reply_size != (result.status == CALL_OK ? size : 0)) {
    return CALL_IO;
  }
  return result.status;
}

int uv__pyxis_set_register(uv_loop_t *loop, const struct wait_interest *interest,
    uint64_t allowed, uint64_t tag, struct readiness_set_registration *registration,
    struct call_limit *limit)
{
  struct readiness_set_register_request request = {
    .header = {PROTOCOL_READINESS_SET, READINESS_SET_REGISTER},
    .target = interest->handle, .allowed_events = allowed,
    .active_events = interest->events,
    .observed_generation = interest->observed_generation, .tag = tag,
  };
  struct syscall_result result = syscall_call_with_limit(loop->backend->set,
      &request, sizeof(request), registration, sizeof(*registration), limit);
  enum call_status status = reply_status(result, sizeof(*registration));
  if (result.status >= CALL_STATUS_COUNT ||
      result.reply_size != (result.status == CALL_OK ? sizeof(*registration) : 0) ||
      (status == CALL_OK && (!registration->identity || !registration->generation))) {
    /* A registration may have taken effect without a usable removal key. */
    uv__pyxis_backend_reset(loop);
    return UV_EIO;
  }
  return uv__pyxis_status(status);
}

void uv__pyxis_backend_reset(uv_loop_t *loop)
{
  struct uv__pyxis_backend *backend = loop->backend;
  /* Last CONTROL closure logically withdraws observations before returning;
   * producer storage may drain later. Never continue with an uncertain close. */
  if (backend->set != HANDLE_INVALID && handle_close(backend->set) != CALL_OK) {
    abort();
  }
  backend->set = HANDLE_INVALID;
  backend->rebuild = 1;
  /* Zero permanently exhausts this backend incarnation rather than reusing
   * a generation that a staged reply could still carry. Cleanup stays valid. */
  if (backend->epoch) {
    ++backend->epoch;
  }
  for (size_t index = 0; index < backend->capacity; ++index) {
    if (backend->watches[index]) {
      backend->watches[index]->registration = (struct readiness_set_registration){0};
    }
  }
}

void uv__pyxis_set_remove(uv_loop_t *loop,
    struct readiness_set_registration registration)
{
  if (!registration.identity || loop->backend->set == HANDLE_INVALID) {
    return;
  }
  struct readiness_set_remove_request request = {
    .header = {PROTOCOL_READINESS_SET, READINESS_SET_REMOVE},
    .registration = registration,
  };
  enum call_status status = reply_status(syscall_call(loop->backend->set,
      &request, sizeof(request), NULL, 0), 0);
  if (status != CALL_OK) {
    uv__pyxis_backend_reset(loop);
  }
}

int uv__pyxis_backend_ensure(uv_loop_t *loop, struct call_limit *limit)
{
  struct uv__pyxis_backend *backend = loop->backend;
  if (!backend->epoch) {
    return UV_EOVERFLOW;
  }
  if (!backend->rebuild) {
    return 0;
  }
  if (backend->set == HANDLE_INVALID) {
    struct readiness_set_create_reply reply;
    enum call_status status = reply_status(syscall_readiness_set_create_with_limit(
        &reply, sizeof(reply), limit), sizeof(reply));
    if (status != CALL_OK) {
      return uv__pyxis_status(status);
    }
    backend->set = reply.handle;
  }
  for (size_t index = 0; index < backend->capacity; ++index) {
    struct uv__pyxis_watch *watch = backend->watches[index];
    if (!watch || watch->registration.identity || watch->error ||
        watch->target == HANDLE_INVALID ||
        (watch->owner && uv__is_closing(watch->owner))) {
      continue;
    }
    struct wait_interest interest = {watch->target, watch->active, 0};
    int error = uv__pyxis_set_register(loop, &interest, watch->allowed, watch->tag,
        &watch->registration, limit);
    if (error) {
      return error;
    }
  }
  backend->rebuild = 0;
  return 0;
}

int uv__pyxis_watch_new(uv_loop_t *loop, uv_handle_t *owner, handle_t target,
    uint64_t allowed, uint64_t active, struct call_limit *limit,
    struct uv__pyxis_watch **out)
{
  *out = NULL;
  int error = uv__pyxis_backend_ensure(loop, limit);
  if (error) {
    return error;
  }
  struct uv__pyxis_backend *backend = loop->backend;
  size_t index = 0;
  while (index < backend->capacity && backend->watches[index]) {
    ++index;
  }
  if (index == backend->capacity) {
    size_t capacity = backend->capacity ? backend->capacity * 2 : 16;
    if (capacity < backend->capacity || capacity > SIZE_MAX / sizeof(*backend->watches) ||
        capacity >= UV__PYXIS_SIGNAL_TAG(1)) {
      return UV_EOVERFLOW;
    }
    void *grown = uv__realloc(backend->watches, capacity * sizeof(*backend->watches));
    if (!grown) {
      return UV_ENOMEM;
    }
    backend->watches = grown;
    memset(backend->watches + backend->capacity, 0,
        (capacity - backend->capacity) * sizeof(*backend->watches));
    backend->capacity = capacity;
  }
  struct uv__pyxis_watch *watch = uv__calloc(1, sizeof(*watch));
  if (!watch) {
    return UV_ENOMEM;
  }
  watch->loop = loop;
  watch->owner = owner;
  watch->target = target;
  watch->tag = index + 1;
  watch->allowed = allowed;
  watch->active = active;
  uv__queue_init(&watch->ready_queue);
  if (target != HANDLE_INVALID) {
    struct wait_interest interest = {target, active, 0};
    error = uv__pyxis_set_register(loop, &interest, allowed, watch->tag,
        &watch->registration, limit);
    if (error) {
      uv__free(watch);
      return error;
    }
  }
  backend->watches[index] = watch;
  *out = watch;
  return 0;
}

void uv__pyxis_watch_bind(struct uv__pyxis_watch *watch, handle_t target,
    struct readiness_set_registration registration)
{
  assert(target != HANDLE_INVALID && registration.identity && registration.generation);
  watch->target = target;
  watch->registration = registration;
}

int uv__pyxis_watch_update(struct uv__pyxis_watch *watch, uint64_t active)
{
  if (!watch) {
    return UV_EBADF;
  }
  if (watch->error) {
    if (!active) {
      watch->active = 0;
      uv__pyxis_watch_unready(watch);
    }
    return active ? watch->error : 0;
  }
  int error = uv__pyxis_backend_ensure(watch->loop, NULL);
  if (error) {
    return error;
  }
  if (active == watch->active) {
    return 0;
  }
  struct readiness_set_update_request request = {
    .header = {PROTOCOL_READINESS_SET, READINESS_SET_UPDATE},
    .registration = watch->registration, .active_events = active,
  };
  struct readiness_set_registration registration;
  enum call_status status = reply_status(syscall_call(watch->loop->backend->set,
      &request, sizeof(request), &registration, sizeof(registration)), sizeof(registration));
  if (status != CALL_OK) {
    watch->error = uv__pyxis_status(status);
    uv__pyxis_set_remove(watch->loop, watch->registration);
    watch->registration = (struct readiness_set_registration){0};
    if (watch->active) {
      uv__pyxis_watch_ready(watch);
    }
    return watch->error;
  }
  watch->registration = registration;
  watch->active = active;
  return 0;
}

void uv__pyxis_watch_unready(struct uv__pyxis_watch *watch)
{
  if (watch && watch->queued) {
    uv__queue_remove(&watch->ready_queue);
    uv__queue_init(&watch->ready_queue);
    watch->queued = 0;
  }
}

void uv__pyxis_watch_ready(struct uv__pyxis_watch *watch)
{
  if (watch && !watch->queued) {
    uv__queue_insert_tail(&watch->loop->backend->ready_queue, &watch->ready_queue);
    watch->queued = 1;
  }
}

void uv__pyxis_watch_close(struct uv__pyxis_watch **entry)
{
  struct uv__pyxis_watch *watch = *entry;
  if (!watch) {
    return;
  }
  uv_loop_t *loop = watch->loop;
  uv__pyxis_watch_unready(watch);
  uv__pyxis_set_remove(loop, watch->registration);
  loop->backend->watches[watch->tag - 1] = NULL;
  *entry = NULL;
  uv__free(watch);
}

struct uv__pyxis_watch *uv__pyxis_watch_find(uv_loop_t *loop,
    const struct readiness_set_event *event, uint64_t epoch)
{
  struct uv__pyxis_backend *backend = loop->backend;
  if (!backend->epoch || epoch != backend->epoch || !event->tag || event->tag > backend->capacity ||
      !event->registration.identity || !event->registration.generation) {
    return NULL;
  }
  struct uv__pyxis_watch *watch = backend->watches[event->tag - 1];
  if (!watch || watch->registration.identity != event->registration.identity ||
      watch->registration.generation != event->registration.generation ||
      (watch->owner && uv__is_closing(watch->owner))) {
    return NULL;
  }
  return watch;
}

int uv__pyxis_set_wait(uv_loop_t *loop, uint64_t deadline,
    struct readiness_set_event *events, size_t *count)
{
  struct readiness_set_wait_request request = {
    .header = {PROTOCOL_READINESS_SET, READINESS_SET_WAIT},
    .deadline = deadline, .events_address = (uintptr_t)events,
    .capacity = READINESS_SET_BATCH_MAX,
  };
  struct readiness_set_wait_reply reply;
  enum call_status status = reply_status(syscall_call(loop->backend->set,
      &request, sizeof(request), &reply, sizeof(reply)), sizeof(reply));
  *count = 0;
  if (status == CALL_TIMED_OUT) {
    return 0;
  }
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  if (reply.count > READINESS_SET_BATCH_MAX || (reply.flags & ~READINESS_SET_WOKEN)) {
    return UV_EIO;
  }
  *count = reply.count;
  return 0;
}

int uv__pyxis_backend_init(uv_loop_t *loop)
{
  struct uv__pyxis_backend *backend = uv__calloc(1, sizeof(*backend));
  if (!backend) {
    return UV_ENOMEM;
  }
  backend->set = HANDLE_INVALID;
  backend->epoch = 1;
  backend->rebuild = 1;
  uv__queue_init(&backend->ready_queue);
  uv__queue_init(&backend->dispatch_queue);
  loop->backend = backend;
  return uv__pyxis_watch_new(loop, NULL, loop->wake_reader, WAIT_READABLE,
      WAIT_READABLE, NULL, &backend->wake);
}

void uv__pyxis_backend_close(uv_loop_t *loop)
{
  struct uv__pyxis_backend *backend = loop->backend;
  if (!backend) {
    return;
  }
  uv__pyxis_watch_close(&backend->wake);
  if (backend->set != HANDLE_INVALID && handle_close(backend->set) != CALL_OK) {
    abort();
  }
  uv__free(backend->watches);
  uv__free(backend);
  loop->backend = NULL;
}
