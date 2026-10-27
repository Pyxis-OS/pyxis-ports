/* Native scheme/metadata adapter and standalone entry for BusyBox tar.
 * SPDX-License-Identifier: GPL-2.0-only */
#include "tar_support.h"

char bb_common_bufsiz1[2 * TAR_BLOCK_SIZE];
int bb_got_signal;
const char *applet_name = "tar";

void pyxis_tar_check_member(const char *name)
{
  if (!name || !*name || *name == '/' || strstr(name, "://")) {
    bb_error_msg_and_die("unsafe tar member: %s", name ? name : "(empty)");
  }
  const char *component = name;
  while (*component) {
    const char *end = strchr(component, '/');
    size_t size = end ? (size_t)(end - component) : strlen(component);
    if (size == 2 && component[0] == '.' && component[1] == '.') {
      bb_error_msg_and_die("unsafe tar member: %s", name);
    }
    if (!end) {
      break;
    }
    component = end + 1;
  }
}

/* BusyBox appletlib.c helper; applet dispatch itself is not built. */
unsigned FAST_FUNC string_array_len(char **argv)
{
	char **start = argv;

	while (*argv)
		argv++;

	return argv - start;
}

const char *pyxis_tar_source_name(const char *path)
{
  const char *scheme = strstr(path, "://");
  return skip_unsafe_prefix(scheme ? scheme + 3 : path);
}

void bb_show_usage(void)
{
  bb_simple_error_msg_and_die("usage: tar {c|t|x}[vOf] [ARCHIVE] [PATH...]");
}

int main(int argc, char **argv)
{
  return tar_main(argc, argv);
}
