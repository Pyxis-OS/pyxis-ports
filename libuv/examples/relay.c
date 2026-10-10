#include <uv.h>
#include <abi/process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct output_write {
  uv_write_t request;
  char bytes[];
};

static uv_loop_t loop;
static uv_pipe_t child_output;
static uv_tty_t input, output;
static uv_timer_t timer;
static uv_process_t child;
static unsigned pending_writes, ticks;
static bool child_done, output_done, finishing;
static int failed;

static void finish_if_drained(void)
{
  if (finishing || !child_done || !output_done || pending_writes) {
    return;
  }
  finishing = true;
  uv_timer_stop(&timer);
  uv_close((uv_handle_t *)&timer, NULL);
  if (!uv_is_closing((uv_handle_t *)&input)) {
    uv_close((uv_handle_t *)&input, NULL);
  }
  uv_close((uv_handle_t *)&output, NULL);
}

static void written(uv_write_t *request, int status)
{
  if (status) {
    failed = 1;
  }
  --pending_writes;
  free(request->data);
  finish_if_drained();
}

static void emit(const char *bytes, size_t length)
{
  struct output_write *write = malloc(sizeof(*write) + length);
  if (!write) {
    failed = 1;
    return;
  }
  memcpy(write->bytes, bytes, length);
  write->request.data = write;
  uv_buf_t buffer = uv_buf_init(write->bytes, (unsigned)length);
  int status = uv_write(&write->request, (uv_stream_t *)&output, &buffer, 1, written);
  if (status) {
    failed = 1;
    free(write);
  } else {
    ++pending_writes;
  }
}

static void allocate(uv_handle_t *handle, size_t suggested, uv_buf_t *buffer)
{
  (void)handle;
  (void)suggested;
  buffer->base = malloc(4096);
  buffer->len = buffer->base ? 4096 : 0;
}

static void received(uv_stream_t *stream, ssize_t count, const uv_buf_t *buffer)
{
  if (count > 0) {
    emit(buffer->base, (size_t)count);
  } else if (count < 0) {
    if (count != UV_EOF) {
      failed = 1;
    }
    uv_read_stop(stream);
    uv_close((uv_handle_t *)stream, NULL);
    if (stream == (uv_stream_t *)&child_output) {
      output_done = true;
    }
  }
  free(buffer->base);
  finish_if_drained();
}

static void tick(uv_timer_t *handle)
{
  (void)handle;
  char message[64];
  int length = snprintf(message, sizeof(message), "timer: %u\n", ++ticks);
  emit(message, (size_t)length);
}

static void exited(uv_process_t *process, int64_t status, int signal)
{
  char message[128];
  int length = snprintf(message, sizeof(message),
      "child: reason=%llu status=%lld signal=%d\n",
      (unsigned long long)process->exit_reason, (long long)status, signal);
  emit(message, (size_t)length);
  if (process->exit_reason != PROCESS_EXITED || status || signal) {
    failed = 1;
  }
  child_done = true;
  uv_close((uv_handle_t *)process, NULL);
  finish_if_drained();
}

static void close_handle(uv_handle_t *handle, void *argument)
{
  (void)argument;
  if (!uv_is_closing(handle)) {
    uv_close(handle, NULL);
  }
}

int main(int argc, char **argv)
{
  if (argc == 2 && !strcmp(argv[1], "--child")) {
    for (unsigned i = 0; i < 6; ++i) {
      printf("child chunk: %u\n", i + 1);
      uv_sleep(700);
    }
    return 0;
  }
  int status = uv_loop_init(&loop);
  if (status) {
    fprintf(stderr, "relay: loop: %s\n", uv_strerror(status));
    return 1;
  }
  if (!(status = uv_tty_init(&loop, &output, 1, 0)) &&
      !(status = uv_tty_init(&loop, &input, 0, 1)) &&
      !(status = uv_pipe_init(&loop, &child_output, 0)) &&
      !(status = uv_timer_init(&loop, &timer))) {
    char *arguments[] = {"app://bin/relay.pxe", "--child", NULL};
    uv_stdio_container_t streams[] = {
      {.flags = UV_IGNORE},
      {.flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE, .data.stream = (uv_stream_t *)&child_output},
      {.flags = UV_INHERIT_FD, .data.fd = 2},
    };
    uv_process_options_t options = {
      .exit_cb = exited,
      .file = arguments[0],
      .args = arguments,
      .stdio_count = 3,
      .stdio = streams,
    };
    status = uv_spawn(&loop, &child, &options);
    if (!status) {
      status = uv_read_start((uv_stream_t *)&child_output, allocate, received);
    }
    if (!status) {
      status = uv_read_start((uv_stream_t *)&input, allocate, received);
    }
    if (!status) {
      status = uv_timer_start(&timer, tick, 200, 200);
    }
  }
  if (status) {
    fprintf(stderr, "relay: setup: %s\n", uv_strerror(status));
    failed = 1;
    uv_walk(&loop, close_handle, NULL);
  }
  uv_run(&loop, UV_RUN_DEFAULT);
  if (uv_loop_close(&loop)) {
    failed = 1;
  }
  return failed;
}
