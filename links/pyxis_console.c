/* Pyxis side of Links' platform layer; see pyxis_console.h.
 *
 * Part of the Pyxis port of Links; distributed under the GPL like Links
 * (see COPYING in the Links source). */

#include <clock.h>
#include <errno.h>
#include <handle.h>
#include <startup.h>
#include <term.h>
#include "pyxis_console.h"

#define NANOSECONDS_PER_MILLISECOND UINT64_C(1000000)

static struct terminal terminal;
static handle_t passthrough = HANDLE_INVALID;

bool pyxis_console_open(void)
{
  terminal.input = startup_resource("input");
  terminal.output = startup_resource("output");
  return terminal.input != HANDLE_INVALID && terminal.output != HANDLE_INVALID;
}

long pyxis_console_read(void *bytes, size_t capacity, long timeout_ms)
{
  size_t count = 0;
  enum call_status status;
  if (timeout_ms < 0) {
    status = term_read(&terminal, bytes, capacity, &count);
  } else {
    uint32_t bound = timeout_ms > UINT32_MAX ? UINT32_MAX : (uint32_t)timeout_ms;
    status = term_read_timeout(&terminal, bytes, capacity, bound, &count);
  }
  if (status == CALL_TIMED_OUT) {
    return -1;
  }
  if (status != CALL_OK) {
    errno = EIO;
    return -2;
  }
  return (long)count;
}

bool pyxis_console_write(const void *bytes, size_t size)
{
  if (term_write_all(&terminal, bytes, size) != CALL_OK) {
    errno = EIO;
    return false;
  }
  return true;
}

bool pyxis_console_size(int *columns, int *rows)
{
  size_t width, height;
  if (term_size(&terminal, &width, &height) != CALL_OK || !width || !height ||
      width > INT32_MAX || height > INT32_MAX) {
    return false;
  }
  *columns = (int)width;
  *rows = (int)height;
  return true;
}

void pyxis_console_hold_ctrl_c(bool hold)
{
  if (hold && passthrough == HANDLE_INVALID) {
    /* Without passthrough Links still runs; Ctrl+C then ends it. */
    if (term_passthrough(&terminal, &passthrough) != CALL_OK) {
      passthrough = HANDLE_INVALID;
    }
  } else if (!hold && passthrough != HANDLE_INVALID) {
    handle_close(passthrough);
    passthrough = HANDLE_INVALID;
  }
}

bool pyxis_clock_ms(uint64_t *milliseconds)
{
  uint64_t now;
  if (clock_now(startup_resource("clock"), &now) != CALL_OK) {
    return false;
  }
  *milliseconds = now / NANOSECONDS_PER_MILLISECOND;
  return true;
}

bool pyxis_sleep_ms(uint64_t milliseconds)
{
  if (milliseconds > UINT64_MAX / NANOSECONDS_PER_MILLISECOND) {
    milliseconds = UINT64_MAX / NANOSECONDS_PER_MILLISECOND;
  }
  return clock_sleep_for(startup_resource("clock"),
      milliseconds * NANOSECONDS_PER_MILLISECOND) == CALL_OK;
}

bool pyxis_scheme_is_root(const char *scheme)
{
  return startup_root(scheme) != HANDLE_INVALID;
}
