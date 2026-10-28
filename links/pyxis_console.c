/* Pyxis side of Links' platform layer; see pyxis_console.h.
 *
 * Part of the Pyxis port of Links; distributed under the GPL like Links
 * (see COPYING in the Links source). */

#include <clock.h>
#include <errno.h>
#include <handle.h>
#include <startup.h>
#include <environment.h>
#include <abi/directory.h>
#include <term.h>
#include <unistd.h>
#include <pyxis/descriptor.h>
#include <pyxis/terminal.h>
#include "pyxis_console.h"

#define NANOSECONDS_PER_MILLISECOND UINT64_C(1000000)

static struct terminal terminal;
static handle_t clock_handle;
static int input_fd = -1;
static struct pyxis_terminal_state *raw_state;

bool pyxis_console_open(void)
{
  if (input_fd >= 0) {
    close(input_fd);
    input_fd = -1;
  }
  pyxis_terminal_state_free(raw_state);
  raw_state = NULL;
  if (terminal.output != HANDLE_INVALID) {
    handle_close(terminal.output);
  }
  if (clock_handle != HANDLE_INVALID) {
    handle_close(clock_handle);
  }
  handle_t input = HANDLE_INVALID;
  environment_capability(startup_environment_handle(), "input", &input);
  if (input != HANDLE_INVALID) {
    input_fd = pyxis_descriptor_adopt(&input, PYXIS_DESCRIPTOR_READ);
    if (input_fd < 0) {
      handle_close(input);
    }
  }
  environment_capability(startup_environment_handle(), "output", &terminal.output);
  environment_capability(startup_environment_handle(), "clock", &clock_handle);
  return input_fd >= 0 && terminal.output != HANDLE_INVALID;
}

int pyxis_console_input_fd(void)
{
  return input_fd;
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
  if (hold && !raw_state) {
    /* Without passthrough Links still runs; Ctrl+C then ends it. */
    pyxis_terminal_make_raw(input_fd, &raw_state);
  } else if (!hold && raw_state) {
    pyxis_terminal_restore(input_fd, raw_state);
    pyxis_terminal_state_free(raw_state);
    raw_state = NULL;
  }
}

bool pyxis_clock_ms(uint64_t *milliseconds)
{
  uint64_t now;
  if (clock_now(clock_handle, &now) != CALL_OK) {
    return false;
  }
  *milliseconds = now / NANOSECONDS_PER_MILLISECOND;
  return true;
}

bool pyxis_scheme_is_root(const char *scheme)
{
  handle_t source = HANDLE_INVALID;
  if (environment_capability(startup_environment_handle(), scheme, &source) != CALL_OK) {
    return false;
  }
  struct handle_info info;
  bool directory = handle_query(source, &info) == CALL_OK && info.protocol == PROTOCOL_DIRECTORY;
  handle_close(source);
  return directory;
}
