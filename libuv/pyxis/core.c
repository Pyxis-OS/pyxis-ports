#include "internal.h"
#include <clock.h>
#include <errno.h>
#include <handle.h>
#include <limits.h>
#include <pipe.h>
#include <pyxis/environment.h>
#include <pyxis/working_path.h>
#include <startup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

static uv_malloc_func allocate = malloc;
static uv_realloc_func reallocate = realloc;
static uv_calloc_func allocate_zero = calloc;
static uv_free_func release = free;
static uv_loop_t default_loop;
static int default_initialized;

void *uv__malloc(size_t size)
{
  return allocate(size ? size : 1);
}
void *uv__calloc(size_t count, size_t size)
{
  return allocate_zero(count, size);
}
void *uv__realloc(void *pointer, size_t size)
{
  return reallocate(pointer, size);
}
void uv__free(void *pointer)
{
  release(pointer);
}

void uv_freeaddrinfo(struct addrinfo *addresses)
{
  /* Match upstream's single-allocation cleanup through the selected allocator.
   * Native getaddrinfo still rejects before allocating any result. */
  uv__free(addresses);
}

void *uv__reallocf(void *pointer, size_t size)
{
  void *next = uv__realloc(pointer, size);
  if (!next && size) {
    uv__free(pointer);
  }
  return next;
}

char *uv__strndup(const char *text, size_t length)
{
  size_t count = strnlen(text, length);
  char *copy = uv__malloc(count + 1);
  if (copy) {
    memcpy(copy, text, count);
    copy[count] = 0;
  }
  return copy;
}

char *uv__strdup(const char *text)
{
  return uv__strndup(text, strlen(text));
}

int uv_replace_allocator(uv_malloc_func malloc_fn, uv_realloc_func realloc_fn,
    uv_calloc_func calloc_fn, uv_free_func free_fn)
{
  if (!malloc_fn || !realloc_fn || !calloc_fn || !free_fn) {
    return UV_EINVAL;
  }
  allocate = malloc_fn;
  reallocate = realloc_fn;
  allocate_zero = calloc_fn;
  release = free_fn;
  return 0;
}

int uv_translate_sys_error(int error)
{
  return error > 0 ? -error : error;
}

int uv__pyxis_status(enum call_status status)
{
  switch (status) {
    case CALL_OK: return 0;
    case CALL_BAD_HANDLE: return UV_EBADF;
    case CALL_DENIED: return UV_EACCES;
    case CALL_BAD_OPERATION: case CALL_UNAVAILABLE: return UV_ENOSYS;
    case CALL_BAD_REQUEST: case CALL_WRONG_TYPE: return UV_EINVAL;
    case CALL_BAD_BUFFER: return UV_EFAULT;
    case CALL_QUEUE_FULL: case CALL_WOULD_BLOCK: return UV_EAGAIN;
    case CALL_ENDPOINT_CLOSED: return UV_EPIPE;
    case CALL_BUSY: return UV_EBUSY;
    case CALL_NO_MEMORY: return UV_ENOMEM;
    case CALL_LIMIT: return UV_ENOSPC;
    case CALL_NOT_FOUND: return UV_ENOENT;
    case CALL_ALREADY_EXISTS: return UV_EEXIST;
    case CALL_READ_ONLY: return UV_EROFS;
    case CALL_TIMED_OUT: return UV_ETIMEDOUT;
    case CALL_NOT_EMPTY: return UV_ENOTEMPTY;
    case CALL_NO_SPACE: case CALL_QUOTA: return UV_ENOSPC;
    case CALL_FILE_TOO_LARGE: return UV_EFBIG;
    case CALL_LINK_NOT_FOLLOWED: return UV_ELOOP;
    case CALL_NAME_TOO_LONG: return UV_ENAMETOOLONG;
    default: return UV_EIO;
  }
}

const char *uv_err_name(int error)
{
  switch (error) {
#define XX(name, message) case UV_##name: return #name;
    UV_ERRNO_MAP(XX)
#undef XX
    default: return "UNKNOWN";
  }
}

const char *uv_strerror(int error)
{
  switch (error) {
#define XX(name, message) case UV_##name: return message;
    UV_ERRNO_MAP(XX)
#undef XX
    default: return "unknown error";
  }
}

char *uv_err_name_r(int error, char *buffer, size_t size)
{
  if (size) {
    uv__strscpy(buffer, uv_err_name(error), size);
  }
  return buffer;
}

char *uv_strerror_r(int error, char *buffer, size_t size)
{
  if (size) {
    uv__strscpy(buffer, uv_strerror(error), size);
  }
  return buffer;
}

size_t uv_handle_size(uv_handle_type type)
{
  switch (type) {
#define XX(upper, lower) case UV_##upper: return sizeof(uv_##lower##_t);
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default: return SIZE_MAX;
  }
}

size_t uv_req_size(uv_req_type type)
{
  switch (type) {
#define XX(upper, lower) case UV_##upper: return sizeof(uv_##lower##_t);
    UV_REQ_TYPE_MAP(XX)
#undef XX
    default: return SIZE_MAX;
  }
}

size_t uv_loop_size(void)
{
  return sizeof(uv_loop_t);
}
uv_buf_t uv_buf_init(char *base, unsigned length) { return (uv_buf_t){base, length}; }
void uv_ref(uv_handle_t *handle)
{
  uv__handle_ref(handle);
}
void uv_unref(uv_handle_t *handle)
{
  uv__handle_unref(handle);
}
int uv_has_ref(const uv_handle_t *handle)
{
  return uv__has_ref(handle);
}
int uv_is_active(const uv_handle_t *handle)
{
  return uv__is_active(handle);
}
int uv_is_closing(const uv_handle_t *handle)
{
  return uv__is_closing(handle);
}
uint64_t uv_now(const uv_loop_t *loop)
{
  return loop->time;
}
void uv_stop(uv_loop_t *loop)
{
  loop->stop_flag = 1;
}

int uv__pyxis_reserve(uv_loop_t *loop, unsigned count)
{
  if (count > WAIT_MAX_INTERESTS - 1 - loop->interest_count) {
    return UV_ENOSPC;
  }
  loop->interest_count += count;
  return 0;
}

void uv__pyxis_release(uv_loop_t *loop, unsigned count)
{
  assert(loop->interest_count >= count);
  loop->interest_count -= count;
}

uint64_t uv_hrtime(void)
{
  uint64_t now;
  enum call_status status = clock_now(startup_resource("clock"), &now);
  if (status != CALL_OK) {
    abort();
  }
  return now;
}

void uv_update_time(uv_loop_t *loop)
{
  uint64_t now;
  if (clock_now(loop->clock, &now) != CALL_OK) {
    abort();
  }
  loop->time = now / UINT64_C(1000000);
}

int uv_loop_init(uv_loop_t *loop)
{
  uint64_t now;
  handle_t clock = startup_resource("clock");
  enum call_status status = clock_now(clock, &now);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  struct pipe_create_reply wake;
  status = pipe_create(startup_resource("pipe"), &wake);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  memset(loop, 0, sizeof(*loop));
  loop->clock = clock;
  loop->time = now / UINT64_C(1000000);
  loop->wake_reader = wake.reader;
  loop->wake_writer = wake.writer;
  uv__queue_init(&loop->handle_queue);
  uv__queue_init(&loop->prepare_handles);
  uv__queue_init(&loop->check_handles);
  uv__queue_init(&loop->idle_handles);
  uv__queue_init(&loop->async_handles);
  return 0;
}

int uv_loop_close(uv_loop_t *loop)
{
  if (!uv__queue_empty(&loop->handle_queue) || loop->active_reqs.count ||
      loop->closing_handles) {
    return UV_EBUSY;
  }
  enum call_status first = handle_close(loop->wake_reader);
  enum call_status second = handle_close(loop->wake_writer);
  loop->wake_reader = HANDLE_INVALID;
  loop->wake_writer = HANDLE_INVALID;
  if (loop == &default_loop) {
    default_initialized = 0;
  }
  return uv__pyxis_status(first != CALL_OK ? first : second);
}

uv_loop_t *uv_default_loop(void)
{
  if (!default_initialized) {
    if (uv_loop_init(&default_loop)) {
      return NULL;
    }
    default_initialized = 1;
  }
  return &default_loop;
}

uv_loop_t *uv_loop_new(void)
{
  uv_loop_t *loop = uv__malloc(sizeof(*loop));
  if (loop && uv_loop_init(loop)) {
    uv__free(loop);
    return NULL;
  }
  return loop;
}

void uv_loop_delete(uv_loop_t *loop)
{
  if (uv_loop_close(loop)) {
    abort();
  }
  if (loop != &default_loop) {
    uv__free(loop);
  }
}

int uv_loop_alive(const uv_loop_t *loop)
{
  return loop->active_handles || loop->active_reqs.count || loop->closing_handles;
}

int uv_backend_fd(const uv_loop_t *loop)
{
  (void)loop;
  return UV_ENOSYS;
}
int uv_backend_timeout(const uv_loop_t *loop)
{
  if (loop->stop_flag || loop->closing_handles || !uv_loop_alive(loop) ||
      !uv__queue_empty(&loop->idle_handles)) {
    return 0;
  }
  return uv__next_timeout(loop);
}

int uv_loop_configure(uv_loop_t *loop, uv_loop_option option, ...)
{
  (void)loop;
  (void)option;
  return UV_ENOSYS;
}

void uv_walk(uv_loop_t *loop, uv_walk_cb callback, void *argument)
{
  struct uv__queue pending;
  uv__queue_move(&loop->handle_queue, &pending);
  while (!uv__queue_empty(&pending)) {
    struct uv__queue *entry = uv__queue_head(&pending);
    uv_handle_t *handle = uv__queue_data(entry, uv_handle_t, handle_queue);
    uv__queue_remove(entry);
    uv__queue_insert_tail(&loop->handle_queue, entry);
    callback(handle, argument);
  }
}

static void print_handle(uv_handle_t *handle, void *argument)
{
  FILE *stream = argument;
  const char *type = uv_handle_type_name(handle->type);
  fprintf(stream, "%-8s %p active=%d referenced=%d closing=%d\n",
      type ? type : "<unknown>", (void *)handle, uv_is_active(handle),
      uv_has_ref(handle), uv_is_closing(handle));
}

void uv_print_all_handles(uv_loop_t *loop, FILE *stream)
{
  if (!stream) {
    stream = stderr;
  }
  if (!loop) {
    loop = uv_default_loop();
    if (!loop) {
      fprintf(stream, "uv_default_loop() failed\n");
      return;
    }
  }
  uv_walk(loop, print_handle, stream);
}

int uv_async_init(uv_loop_t *loop, uv_async_t *handle, uv_async_cb callback)
{
  uv__handle_init(loop, (uv_handle_t *)handle, UV_ASYNC);
  handle->async_cb = callback;
  handle->pending = 0;
  uv__queue_insert_tail(&loop->async_handles, &handle->queue);
  uv__handle_start(handle);
  return 0;
}

int uv_async_send(uv_async_t *handle)
{
  if (uv__is_closing(handle)) {
    return UV_EINVAL;
  }
  if (handle->pending) {
    return 0;
  }
  size_t written;
  const unsigned char byte = 1;
  enum call_status status = pipe_try_write(handle->loop->wake_writer, &byte, 1, &written);
  if (status != CALL_OK && status != CALL_WOULD_BLOCK) {
    return uv__pyxis_status(status);
  }
  handle->pending = 1;
  return 0;
}

static void run_async(uv_loop_t *loop)
{
  unsigned char bytes[128];
  size_t count;
  enum call_status status;
  do {
    status = pipe_try_read(loop->wake_reader, bytes, sizeof(bytes), &count);
  } while (status == CALL_OK && count);
  struct uv__queue pending;
  uv__queue_move(&loop->async_handles, &pending);
  while (!uv__queue_empty(&pending)) {
    struct uv__queue *entry = uv__queue_head(&pending);
    uv_async_t *handle = uv__queue_data(entry, uv_async_t, queue);
    uv__queue_remove(entry);
    uv__queue_insert_tail(&loop->async_handles, entry);
    if (handle->pending && !uv__is_closing(handle)) {
      handle->pending = 0;
      if (handle->async_cb) {
        handle->async_cb(handle);
      }
    }
  }
}

void uv_close(uv_handle_t *handle, uv_close_cb callback)
{
  assert(!uv__is_closing(handle));
  switch (handle->type) {
    case UV_TIMER: uv__timer_close((uv_timer_t *)handle); break;
    case UV_PREPARE: uv_prepare_stop((uv_prepare_t *)handle); break;
    case UV_CHECK: uv_check_stop((uv_check_t *)handle); break;
    case UV_IDLE: uv_idle_stop((uv_idle_t *)handle); break;
    case UV_ASYNC: uv__queue_remove(&((uv_async_t *)handle)->queue); break;
    default: break;
  }
  uv__handle_stop(handle);
  handle->flags |= UV_HANDLE_CLOSING;
  handle->close_cb = callback;
  handle->next_closing = handle->loop->closing_handles;
  handle->loop->closing_handles = handle;
}

static void run_closing(uv_loop_t *loop)
{
  uv_handle_t *handles = loop->closing_handles;
  loop->closing_handles = NULL;
  while (handles) {
    uv_handle_t *handle = handles;
    handles = handle->next_closing;
    if (handle->type == UV_NAMED_PIPE || handle->type == UV_TTY) {
      uv__pyxis_stream_close((uv_stream_t *)handle);
    } else if (handle->type == UV_PROCESS) {
      uv__pyxis_process_close((uv_process_t *)handle);
    }
    uv__queue_remove(&handle->handle_queue);
    handle->flags |= UV_HANDLE_CLOSED;
    if (handle->close_cb) {
      handle->close_cb(handle);
    }
  }
}

static int poll_events(uv_loop_t *loop, int timeout)
{
  struct wait_interest interests[WAIT_MAX_INTERESTS];
  uv_handle_t *handles[WAIT_MAX_INTERESTS];
  uint64_t events[WAIT_MAX_INTERESTS];
  size_t count = 1;
  interests[0] = (struct wait_interest){loop->wake_reader, WAIT_READABLE, 0};
  handles[0] = NULL;
  struct uv__queue *entry;
  uv__queue_foreach(entry, &loop->handle_queue) {
    uv_handle_t *handle = uv__queue_data(entry, uv_handle_t, handle_queue);
    if (uv__is_closing(handle)) {
      continue;
    }
    if (handle->type == UV_NAMED_PIPE || handle->type == UV_TTY) {
      int result = uv__pyxis_stream_events((uv_stream_t *)handle, &interests[count]);
      if (result < 0) {
        return result;
      }
      if (!result) {
        continue;
      }
      if (result == 2) {
        timeout = 0;
      }
    } else if (handle->type == UV_PROCESS && uv__is_active(handle)) {
      interests[count] = (struct wait_interest){((uv_process_t *)handle)->observer,
          WAIT_COMPLETE, 0};
    } else {
      continue;
    }
    handles[count++] = handle;
  }
  uint64_t now;
  enum call_status status = clock_now(loop->clock, &now);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  uint64_t duration = timeout < 0 || timeout > 30000 ? WAIT_MAX_WAIT_NS :
      (uint64_t)timeout * UINT64_C(1000000);
  uint64_t deadline = timeout == 0 ? 0 : now + duration;
  if (deadline < now && timeout != 0) {
    deadline = UINT64_MAX;
  }
  memset(events, 0, count * sizeof(*events));
  status = wait_many(interests, count, deadline, events);
  if (status != CALL_OK && status != CALL_TIMED_OUT) {
    return uv__pyxis_status(status);
  }
  if (events[0]) {
    run_async(loop);
  }
  for (size_t i = 1; i < count; ++i) {
    uv_handle_t *handle = handles[i];
    if (uv__is_closing(handle)) {
      continue;
    }
    if (handle->type == UV_PROCESS) {
      if (events[i] & WAIT_COMPLETE) {
        uv__pyxis_process_dispatch((uv_process_t *)handle);
      }
    } else {
      uv__pyxis_stream_dispatch((uv_stream_t *)handle, events[i]);
    }
  }
  return 0;
}

int uv_run(uv_loop_t *loop, uv_run_mode mode)
{
  if (mode != UV_RUN_DEFAULT && mode != UV_RUN_ONCE && mode != UV_RUN_NOWAIT) {
    return UV_EINVAL;
  }
  int result = 0;
  int alive = uv_loop_alive(loop);
  if (!alive) {
    uv_update_time(loop);
  }
  if (mode == UV_RUN_DEFAULT && alive && !loop->stop_flag) {
    uv_update_time(loop);
    uv__run_timers(loop);
  }
  while (alive && !loop->stop_flag) {
    uv__run_idle(loop);
    uv__run_prepare(loop);
    int timeout = mode == UV_RUN_NOWAIT ? 0 : uv_backend_timeout(loop);
    result = poll_events(loop, timeout);
    if (result) {
      break;
    }
    uv__run_check(loop);
    run_closing(loop);
    uv_update_time(loop);
    uv__run_timers(loop);
    alive = uv_loop_alive(loop);
    if (mode != UV_RUN_DEFAULT) {
      break;
    }
  }
  loop->stop_flag = 0;
  return result ? result : alive;
}

void uv_library_shutdown(void)
{
  if (default_initialized && !uv_loop_alive(&default_loop)) {
    uv_loop_close(&default_loop);
  }
}

int uv_clock_gettime(uv_clock_id id, uv_timespec64_t *time)
{
  if (!time) {
    return UV_EINVAL;
  }
  uv_timespec64_t result;
  enum call_status status;
  if (id == UV_CLOCK_MONOTONIC) {
    uint64_t now;
    status = clock_now(startup_resource("clock"), &now);
    if (status == CALL_OK) {
      result.tv_sec = (int64_t)(now / UINT64_C(1000000000));
      result.tv_nsec = (int32_t)(now % UINT64_C(1000000000));
    }
  } else if (id == UV_CLOCK_REALTIME) {
    struct clock_wall_reading wall;
    status = clock_wall_now(startup_resource("clock"), &wall);
    if (status == CALL_OK) {
      result.tv_sec = wall.seconds;
      result.tv_nsec = (int32_t)wall.nanoseconds;
    }
  } else {
    return UV_EINVAL;
  }
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  *time = result;
  return 0;
}

int uv_gettimeofday(uv_timeval64_t *time)
{
  if (!time) {
    return UV_EINVAL;
  }
  uv_timespec64_t now;
  int result = uv_clock_gettime(UV_CLOCK_REALTIME, &now);
  if (!result) {
    *time = (uv_timeval64_t){now.tv_sec, now.tv_nsec / 1000};
  }
  return result;
}

void uv_sleep(unsigned milliseconds)
{
  if (clock_sleep_for(startup_resource("clock"), (uint64_t)milliseconds * UINT64_C(1000000)) != CALL_OK) {
    abort();
  }
}

int uv__pyxis_copy_string(const char *text, char *buffer, size_t *size)
{
  if (!buffer || !size) {
    return UV_EINVAL;
  }
  if (!text) {
    return UV_ENOENT;
  }
  size_t length = strlen(text);
  if (*size <= length) {
    *size = length + 1;
    return UV_ENOBUFS;
  }
  memcpy(buffer, text, length + 1);
  *size = length;
  return 0;
}

int uv_cwd(char *buffer, size_t *size)
{
  if (!buffer || !size || !*size) {
    return UV_EINVAL;
  }
  const struct path_context *context;
  enum call_status status = pyxis_working_context(&context);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  const char *path = pyxis_working_path();
  return path ? uv__pyxis_copy_string(path, buffer, size) : UV_ENOSYS;
}

int uv_os_getenv(const char *name, char *buffer, size_t *size)
{
  if (!name || !buffer || !size) {
    return UV_EINVAL;
  }
  const char *value;
  enum call_status status = pyxis_environment_get(name, &value);
  return status == CALL_OK ? uv__pyxis_copy_string(value, buffer, size) : uv__pyxis_status(status);
}

uv_pid_t uv_os_getpid(void)
{
  return UV_ENOSYS;
}
uv_pid_t uv_os_getppid(void)
{
  return UV_ENOSYS;
}
char **uv_setup_args(int count, char **arguments)
{
  (void)count;
  return arguments;
}
void uv_disable_stdio_inheritance(void)
{
  /* Native launches already install only explicitly selected streams. */
}
void uv_dlclose(uv_lib_t *library)
{
  (void)library;
}
const char *uv_dlerror(const uv_lib_t *library)
{
  (void)library;
  return "module loading is unsupported";
}

int uv_chdir(const char *dir)
{
  return chdir(dir) ? uv_translate_sys_error(errno) : 0;
}

int uv_os_setenv(const char *name, const char *value)
{
  return setenv(name, value, 1) ? uv_translate_sys_error(errno) : 0;
}

int uv_os_unsetenv(const char *name)
{
  return unsetenv(name) ? uv_translate_sys_error(errno) : 0;
}

void uv_os_free_environ(uv_env_item_t *envitems, int count)
{
  for (int i = 0; i < count; ++i) {
    uv__free(envitems[i].name);
  }
  uv__free(envitems);
}

int uv_os_environ(uv_env_item_t **envitems, int *count)
{
  if (!envitems || !count) {
    return UV_EINVAL;
  }
  *envitems = NULL;
  *count = 0;
  struct pyxis_environment_snapshot snapshot = {0};
  enum call_status status = pyxis_environment_snapshot_init(&snapshot);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  if (snapshot.count > INT_MAX || snapshot.count > SIZE_MAX / sizeof(uv_env_item_t)) {
    pyxis_environment_snapshot_close(&snapshot);
    return UV_EOVERFLOW;
  }
  uv_env_item_t *items = uv__calloc(snapshot.count ? snapshot.count : 1, sizeof(*items));
  int error = items ? 0 : UV_ENOMEM;
  size_t used = 0;
  while (!error && used < snapshot.count) {
    const char *name = (const char *)(uintptr_t)snapshot.variables[used].name;
    const char *value = (const char *)(uintptr_t)snapshot.variables[used].value;
    size_t name_size = strlen(name) + 1, value_size = strlen(value) + 1;
    if (value_size > SIZE_MAX - name_size) {
      error = UV_EOVERFLOW;
      break;
    }
    char *storage = uv__malloc(name_size + value_size);
    if (!storage) {
      error = UV_ENOMEM;
      break;
    }
    memcpy(storage, name, name_size);
    memcpy(storage + name_size, value, value_size);
    items[used++] = (uv_env_item_t){storage, storage + name_size};
  }
  pyxis_environment_snapshot_close(&snapshot);
  if (error) {
    uv_os_free_environ(items, (int)used);
    return error;
  }
  *envitems = items;
  *count = (int)used;
  return 0;
}
