#include "internal.h"
#include <abi/console.h>
#include <abi/file.h>
#include <abi/pipe.h>
#include <console.h>
#include <errno.h>
#include <handle.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WRITE_TRANSFERS_PER_TURN 32

static uv_tty_t *raw_ttys;

static void update_activity(uv_stream_t *stream)
{
  if ((stream->flags & UV_HANDLE_READING) || !uv__queue_empty(&stream->write_queue) ||
      !uv__queue_empty(&stream->write_completed_queue) || stream->shutdown_req ||
      (stream->type == UV_TTY && ((uv_tty_t *)stream)->resize_cb)) {
    uv__handle_start(stream);
  } else {
    uv__handle_stop(stream);
  }
}

void uv__pyxis_stream_init(uv_loop_t *loop, uv_stream_t *stream, uv_handle_type type)
{
  uv__handle_init(loop, (uv_handle_t *)stream, type);
  stream->write_queue_size = 0;
  stream->alloc_cb = NULL;
  stream->read_cb = NULL;
  stream->shutdown_req = NULL;
  stream->fd = -1;
  stream->access = 0;
  stream->admitted = 0;
  uv__queue_init(&stream->write_queue);
  uv__queue_init(&stream->write_completed_queue);
}

int uv_pipe_init(uv_loop_t *loop, uv_pipe_t *handle, int ipc)
{
  if (ipc) {
    return UV_ENOSYS;
  }
  uv__pyxis_stream_init(loop, (uv_stream_t *)handle, UV_NAMED_PIPE);
  handle->ipc = 0;
  return 0;
}

int uv_pipe_open(uv_pipe_t *handle, uv_file fd)
{
  if (uv__is_closing(handle) || handle->fd >= 0) {
    return UV_EINVAL;
  }
  struct pyxis_descriptor_binding binding;
  if (pyxis_descriptor_borrow(fd, &binding)) {
    return uv_translate_sys_error(errno);
  }
  if (binding.info.protocol != PROTOCOL_PIPE || binding.info.kind != HANDLE_KIND_NATIVE ||
      binding.access == (PYXIS_DESCRIPTOR_READ | PYXIS_DESCRIPTOR_WRITE)) {
    return UV_ENOSYS;
  }
  int error = uv__pyxis_reserve(handle->loop, 1);
  if (error) {
    return error;
  }
  handle->fd = fd;
  handle->access = binding.access;
  handle->admitted = 1;
  if (binding.access & PYXIS_DESCRIPTOR_READ) {
    handle->flags |= UV_HANDLE_READABLE;
  }
  if (binding.access & PYXIS_DESCRIPTOR_WRITE) {
    handle->flags |= UV_HANDLE_WRITABLE;
  }
  return 0;
}

int uv_tty_init(uv_loop_t *loop, uv_tty_t *handle, uv_file fd, int readable)
{
  struct pyxis_descriptor_binding binding;
  if (pyxis_descriptor_borrow(fd, &binding)) {
    return uv_translate_sys_error(errno);
  }
  unsigned access = readable ? PYXIS_DESCRIPTOR_READ : PYXIS_DESCRIPTOR_WRITE;
  if (binding.info.protocol != PROTOCOL_CONSOLE || binding.info.kind != HANDLE_KIND_NATIVE ||
      !(binding.access & access)) {
    return UV_EINVAL;
  }
  int error = uv__pyxis_reserve(loop, 1);
  if (error) {
    return error;
  }
  uv__pyxis_stream_init(loop, (uv_stream_t *)handle, UV_TTY);
  handle->fd = fd;
  handle->access = access;
  handle->admitted = 1;
  handle->flags |= readable ? UV_HANDLE_READABLE : UV_HANDLE_WRITABLE;
  handle->mode = UV_TTY_MODE_NORMAL;
  handle->passthrough = HANDLE_INVALID;
  handle->raw_next = NULL;
  handle->resize_generation = 0;
  handle->resize_cb = NULL;
  return 0;
}

uv_handle_type uv_guess_handle(uv_file fd)
{
  struct pyxis_descriptor_binding binding;
  if (pyxis_descriptor_borrow(fd, &binding)) {
    return UV_UNKNOWN_HANDLE;
  }
  if (binding.info.protocol == PROTOCOL_CONSOLE) {
    return UV_TTY;
  }
  if (binding.info.protocol == PROTOCOL_PIPE) {
    return UV_NAMED_PIPE;
  }
  if (binding.info.protocol == PROTOCOL_FILE) {
    return UV_FILE;
  }
  return UV_UNKNOWN_HANDLE;
}

int uv_fileno(const uv_handle_t *handle, uv_os_fd_t *fd)
{
  if (!fd || uv__is_closing(handle)) {
    return UV_EINVAL;
  }
  if (handle->type != UV_NAMED_PIPE && handle->type != UV_TTY) {
    return UV_ENOSYS;
  }
  int descriptor = ((const uv_stream_t *)handle)->fd;
  if (descriptor < 0) {
    return UV_EBADF;
  }
  *fd = descriptor;
  return 0;
}

int uv_is_readable(const uv_stream_t *stream)
{
  return !uv__is_closing(stream) && (stream->flags & UV_HANDLE_READABLE) != 0;
}

int uv_is_writable(const uv_stream_t *stream)
{
  return !uv__is_closing(stream) && (stream->flags & UV_HANDLE_WRITABLE) != 0 &&
      !stream->shutdown_req;
}

int uv_read_start(uv_stream_t *stream, uv_alloc_cb allocate, uv_read_cb callback)
{
  if (!allocate || !callback || !uv_is_readable(stream)) {
    return UV_EINVAL;
  }
  if (stream->flags & UV_HANDLE_READING) {
    return UV_EALREADY;
  }
  stream->alloc_cb = allocate;
  stream->read_cb = callback;
  stream->flags |= UV_HANDLE_READING;
  update_activity(stream);
  return 0;
}

int uv_read_stop(uv_stream_t *stream)
{
  stream->flags &= ~UV_HANDLE_READING;
  update_activity(stream);
  return 0;
}

static void finish_write(uv_stream_t *stream, uv_write_t *request, int error)
{
  size_t remaining = 0;
  for (unsigned i = request->write_index; i < request->nbufs; ++i) {
    remaining += request->bufs[i].len;
  }
  if (request->write_index < request->nbufs) {
    remaining -= request->write_offset;
  }
  stream->write_queue_size -= remaining;
  uv__queue_remove(&request->queue);
  request->error = error;
  uv__queue_insert_tail(&stream->write_completed_queue, &request->queue);
}

static void write_progress(uv_stream_t *stream)
{
  unsigned transfers = 0;
  while (!uv__queue_empty(&stream->write_queue) && !uv__is_closing(stream)) {
    uv_write_t *request = uv__queue_data(uv__queue_head(&stream->write_queue), uv_write_t, queue);
    while (request->write_index < request->nbufs) {
      uv_buf_t *buffer = &request->bufs[request->write_index];
      size_t remaining = buffer->len - request->write_offset;
      if (!remaining) {
        ++request->write_index;
        request->write_offset = 0;
        continue;
      }
      if (transfers == WRITE_TRANSFERS_PER_TURN) {
        return;
      }
      ssize_t written = pyxis_descriptor_try_write(stream->fd,
          buffer->base + request->write_offset, remaining);
      ++transfers;
      if (written < 0) {
        if (errno == EAGAIN) {
          return;
        }
        finish_write(stream, request, uv_translate_sys_error(errno));
        break;
      }
      if (!written) {
        finish_write(stream, request, UV_EIO);
        break;
      }
      request->write_offset += (size_t)written;
      stream->write_queue_size -= (size_t)written;
    }
    if (request->write_index == request->nbufs) {
      finish_write(stream, request, 0);
    }
  }
}

static void write_callbacks(uv_stream_t *stream)
{
  struct uv__queue pending;
  uv__queue_move(&stream->write_completed_queue, &pending);
  while (!uv__queue_empty(&pending)) {
    uv_write_t *request = uv__queue_data(uv__queue_head(&pending),
        uv_write_t, queue);
    uv__queue_remove(&request->queue);
    if (request->bufs != request->bufsml) {
      uv__free(request->bufs);
    }
    request->bufs = NULL;
    uv__req_unregister(stream->loop);
    if (request->cb) {
      request->cb(request, request->error);
    }
  }
}

int uv_write(uv_write_t *request, uv_stream_t *stream, const uv_buf_t buffers[],
    unsigned count, uv_write_cb callback)
{
  if (!request || !buffers || !count || !uv_is_writable(stream)) {
    return UV_EINVAL;
  }
  size_t total = 0;
  for (unsigned i = 0; i < count; ++i) {
    if ((!buffers[i].base && buffers[i].len) || buffers[i].len > SSIZE_MAX ||
        total > SIZE_MAX - buffers[i].len) {
      return UV_EINVAL;
    }
    total += buffers[i].len;
  }
  if (stream->write_queue_size > SIZE_MAX - total) {
    return UV_EOVERFLOW;
  }
  uv_buf_t *copy = request->bufsml;
  if (count > ARRAY_SIZE(request->bufsml)) {
    copy = uv__malloc(count * sizeof(*copy));
    if (!copy) {
      return UV_ENOMEM;
    }
  }
  memcpy(copy, buffers, count * sizeof(*copy));
  uv__req_init(stream->loop, request, UV_WRITE);
  request->handle = stream;
  request->send_handle = NULL;
  request->cb = callback;
  request->bufs = copy;
  request->nbufs = count;
  request->write_index = 0;
  request->write_offset = 0;
  request->error = 0;
  stream->write_queue_size += total;
  uv__queue_insert_tail(&stream->write_queue, &request->queue);
  update_activity(stream);
  write_progress(stream);
  return 0;
}

int uv_write2(uv_write_t *request, uv_stream_t *stream, const uv_buf_t buffers[],
    unsigned count, uv_stream_t *send_handle, uv_write_cb callback)
{
  if (send_handle) {
    return UV_ENOSYS;
  }
  return uv_write(request, stream, buffers, count, callback);
}

int uv_try_write(uv_stream_t *stream, const uv_buf_t buffers[], unsigned count)
{
  if (!buffers || !count || !uv_is_writable(stream)) {
    return UV_EINVAL;
  }
  if (!uv__queue_empty(&stream->write_queue)) {
    return UV_EAGAIN;
  }
  for (unsigned i = 0; i < count; ++i) {
    if ((!buffers[i].base && buffers[i].len) || buffers[i].len > INT_MAX) {
      return UV_EINVAL;
    }
  }
  for (unsigned i = 0; i < count; ++i) {
    if (buffers[i].len) {
      ssize_t written = pyxis_descriptor_try_write(stream->fd, buffers[i].base, buffers[i].len);
      return written < 0 ? uv_translate_sys_error(errno) : (int)written;
    }
  }
  return 0;
}

int uv_try_write2(uv_stream_t *stream, const uv_buf_t buffers[], unsigned count,
    uv_stream_t *send_handle)
{
  return send_handle ? UV_ENOSYS : uv_try_write(stream, buffers, count);
}

int uv_shutdown(uv_shutdown_t *request, uv_stream_t *stream, uv_shutdown_cb callback)
{
  if (!request || !uv_is_writable(stream) || stream->type != UV_NAMED_PIPE) {
    return UV_EINVAL;
  }
  uv__req_init(stream->loop, request, UV_SHUTDOWN);
  request->handle = stream;
  request->cb = callback;
  stream->shutdown_req = request;
  update_activity(stream);
  return 0;
}

int uv__pyxis_stream_events(uv_stream_t *stream, struct wait_interest *interest)
{
  if (stream->fd < 0) {
    return 0;
  }
  uint64_t events = 0;
  if (stream->flags & UV_HANDLE_READING) {
    events |= WAIT_READABLE;
  }
  if (!uv__queue_empty(&stream->write_queue) ||
      !uv__queue_empty(&stream->write_completed_queue) || stream->shutdown_req) {
    events |= WAIT_WRITABLE;
  }
  uv_tty_t *tty = stream->type == UV_TTY ? (uv_tty_t *)stream : NULL;
  if (tty && tty->resize_cb) {
    events |= WAIT_RESIZED;
  }
  if (!events) {
    return 0;
  }
  struct pyxis_descriptor_binding binding;
  if (pyxis_descriptor_borrow(stream->fd, &binding)) {
    return uv_translate_sys_error(errno);
  }
  *interest = (struct wait_interest){binding.handle, events,
      tty ? tty->resize_generation : 0};
  return ((stream->flags & UV_HANDLE_READING) && binding.buffered_read) ||
      !uv__queue_empty(&stream->write_completed_queue) ||
      (stream->shutdown_req && uv__queue_empty(&stream->write_queue)) ? 2 : 1;
}

void uv__pyxis_stream_dispatch(uv_stream_t *stream, uint64_t events)
{
  if (stream->flags & UV_HANDLE_READING) {
    struct pyxis_descriptor_binding binding;
    int error = pyxis_descriptor_borrow(stream->fd, &binding);
    if (error || binding.buffered_read ||
        (events & (WAIT_READABLE | WAIT_PEER_FIN | WAIT_ERROR))) {
      uv_buf_t buffer = {0};
      stream->alloc_cb((uv_handle_t *)stream, 65536, &buffer);
      ssize_t count;
      if (!buffer.base || !buffer.len) {
        count = UV_ENOBUFS;
      } else {
        count = pyxis_descriptor_try_read(stream->fd, buffer.base, buffer.len);
        if (count < 0) {
          count = errno == EAGAIN ? 0 : uv_translate_sys_error(errno);
        } else if (!count) {
          stream->flags &= ~UV_HANDLE_READABLE;
          count = UV_EOF;
        }
      }
      if (count < 0) {
        uv_read_stop(stream);
      }
      stream->read_cb(stream, count, &buffer);
    }
  }
  if (!uv__is_closing(stream)) {
    write_progress(stream);
  }
  write_callbacks(stream);
  if (stream->shutdown_req && uv__queue_empty(&stream->write_queue) &&
      !uv__is_closing(stream)) {
    uv_shutdown_t *request = stream->shutdown_req;
    stream->shutdown_req = NULL;
    int status = close(stream->fd) ? uv_translate_sys_error(errno) : 0;
    stream->fd = -1;
    stream->flags &= ~UV_HANDLE_WRITABLE;
    uv__pyxis_release(stream->loop, stream->admitted);
    stream->admitted = 0;
    uv__req_unregister(stream->loop);
    if (request->cb) {
      request->cb(request, status);
    }
  }
  if (!uv__is_closing(stream)) {
    if (stream->type == UV_TTY) {
      uv_tty_t *tty = (uv_tty_t *)stream;
      if (tty->resize_cb && (events & (WAIT_RESIZED | WAIT_ERROR))) {
        struct pyxis_descriptor_binding binding;
        struct console_size_reply size;
        int error = pyxis_descriptor_borrow(tty->fd, &binding) ?
            uv_translate_sys_error(errno) :
            uv__pyxis_status(console_size(binding.handle, &size));
        if (!error && (size.columns > INT_MAX || size.rows > INT_MAX)) {
          error = UV_EOVERFLOW;
        }
        if (error || size.generation != tty->resize_generation) {
          uv_pyxis_tty_resize_cb callback = tty->resize_cb;
          if (error) {
            tty->resize_cb = NULL;
          } else {
            tty->resize_generation = size.generation;
          }
          callback(tty, error, error ? 0 : (int)size.columns,
              error ? 0 : (int)size.rows);
        }
      }
    }
  }
  if (!uv__is_closing(stream)) {
    update_activity(stream);
  }
}

static int tty_normal(uv_tty_t *tty)
{
  if (tty->passthrough == HANDLE_INVALID) {
    return 0;
  }
  enum call_status status = handle_close(tty->passthrough);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  uv_tty_t **entry = &raw_ttys;
  while (*entry != tty) {
    entry = &(*entry)->raw_next;
  }
  *entry = tty->raw_next;
  tty->raw_next = NULL;
  tty->passthrough = HANDLE_INVALID;
  tty->mode = UV_TTY_MODE_NORMAL;
  return 0;
}

void uv__pyxis_stream_close(uv_stream_t *stream)
{
  if (stream->type == UV_TTY) {
    uv_tty_t *tty = (uv_tty_t *)stream;
    tty->resize_cb = NULL;
    /* This grant is private to the adapter and cannot be independently closed. */
    if (tty_normal(tty)) {
      abort();
    }
  }
  while (!uv__queue_empty(&stream->write_queue)) {
    uv_write_t *request = uv__queue_data(uv__queue_head(&stream->write_queue), uv_write_t, queue);
    finish_write(stream, request, UV_ECANCELED);
  }
  write_callbacks(stream);
  if (stream->shutdown_req) {
    uv_shutdown_t *request = stream->shutdown_req;
    stream->shutdown_req = NULL;
    uv__req_unregister(stream->loop);
    if (request->cb) {
      request->cb(request, UV_ECANCELED);
    }
  }
  if (stream->fd >= 0) {
    close(stream->fd);
    stream->fd = -1;
  }
  uv__pyxis_release(stream->loop, stream->admitted);
  stream->admitted = 0;
}

int uv_stream_set_blocking(uv_stream_t *stream, int blocking)
{
  if (uv__is_closing(stream)) {
    return UV_EINVAL;
  }
  return blocking ? UV_ENOSYS : 0;
}

int uv_tty_get_winsize(uv_tty_t *handle, int *width, int *height)
{
  if (!handle || handle->type != UV_TTY || !width || !height || uv__is_closing(handle)) {
    return UV_EINVAL;
  }
  struct pyxis_descriptor_binding binding;
  if (pyxis_descriptor_borrow(handle->fd, &binding)) {
    return uv_translate_sys_error(errno);
  }
  struct console_size_reply size;
  enum call_status status = console_size(binding.handle, &size);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  if (size.columns > INT_MAX || size.rows > INT_MAX) {
    return UV_EOVERFLOW;
  }
  *width = (int)size.columns;
  *height = (int)size.rows;
  return 0;
}

int uv_tty_set_mode(uv_tty_t *handle, uv_tty_mode_t mode)
{
  if (!handle || handle->type != UV_TTY || uv__is_closing(handle)) {
    return UV_EINVAL;
  }
  if (mode == UV_TTY_MODE_NORMAL) {
    return tty_normal(handle);
  }
  if (mode != UV_TTY_MODE_RAW) {
    return UV_ENOSYS;
  }
  if (!(handle->access & PYXIS_DESCRIPTOR_READ)) {
    return UV_EINVAL;
  }
  if (handle->mode == UV_TTY_MODE_RAW) {
    return 0;
  }
  struct pyxis_descriptor_binding binding;
  if (pyxis_descriptor_borrow(handle->fd, &binding)) {
    return uv_translate_sys_error(errno);
  }
  handle_t passthrough;
  enum call_status status = console_passthrough(binding.handle, &passthrough);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  handle->passthrough = passthrough;
  handle->mode = UV_TTY_MODE_RAW;
  handle->raw_next = raw_ttys;
  raw_ttys = handle;
  return 0;
}

int uv_tty_reset_mode(void)
{
  while (raw_ttys) {
    int error = tty_normal(raw_ttys);
    if (error) {
      return error;
    }
  }
  return 0;
}

int uv_pyxis_tty_resize_start(uv_tty_t *handle, uv_pyxis_tty_resize_cb callback)
{
  if (!handle || handle->type != UV_TTY || uv__is_closing(handle) || !callback) {
    return UV_EINVAL;
  }
  if (handle->resize_cb) {
    return UV_EALREADY;
  }
  struct pyxis_descriptor_binding binding;
  if (pyxis_descriptor_borrow(handle->fd, &binding)) {
    return uv_translate_sys_error(errno);
  }
  struct console_size_reply size;
  enum call_status status = console_size(binding.handle, &size);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  if (size.columns > INT_MAX || size.rows > INT_MAX) {
    return UV_EOVERFLOW;
  }
  handle->resize_generation = size.generation;
  handle->resize_cb = callback;
  update_activity((uv_stream_t *)handle);
  return 0;
}

int uv_pyxis_tty_resize_stop(uv_tty_t *handle)
{
  if (!handle || handle->type != UV_TTY || uv__is_closing(handle)) {
    return UV_EINVAL;
  }
  handle->resize_cb = NULL;
  update_activity((uv_stream_t *)handle);
  return 0;
}
