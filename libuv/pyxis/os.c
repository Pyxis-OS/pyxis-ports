#include "internal.h"
#include <startup.h>
#include <string.h>
#include <system_info.h>

/* Operating-system queries that native objects can answer. Passwd, group and
 * priority queries have no native meaning and stay in unsupported.c. */

#define PYXIS_SYSNAME "Pyxis"
/* Pyxis runs only on x86-64; used when the identity query is unavailable. */
#define PYXIS_MACHINE "x86_64"

/* The directory is the caller's own root; a program that was not given it has
 * no such directory, which libuv reports as UV_ENOENT. */
static int root_directory(const char *root, const char *path, char *buffer, size_t *size)
{
  if (!buffer || !size) {
    return UV_EINVAL;
  }
  if (startup_root(root) == HANDLE_INVALID) {
    return UV_ENOENT;
  }
  return uv__pyxis_copy_string(path, buffer, size);
}

int uv_os_homedir(char *buffer, size_t *size)
{
  return root_directory("home", "home://", buffer, size);
}

int uv_os_tmpdir(char *buffer, size_t *size)
{
  return root_directory("tmp", "tmp://", buffer, size);
}

int uv_os_gethostname(char *buffer, size_t *size)
{
  if (!buffer || !size) {
    return UV_EINVAL;
  }
  handle_t system_info = startup_resource("system_info");
  if (system_info == HANDLE_INVALID) {
    return UV_EACCES;
  }
  struct system_info_hostname hostname;
  enum call_status status = system_info_get_hostname(system_info, &hostname);
  if (status != CALL_OK) {
    return uv__pyxis_status(status);
  }
  return uv__pyxis_copy_string(hostname.name, buffer, size);
}

/* sysname and machine are Pyxis facts. release is the running kernel's source
 * commit and version its name, both from SYSTEM_INFO identity; without a READ
 * grant, or when the kernel carries no revision, they are empty rather than
 * invented, and the call still succeeds so callers can index the result. */
int uv_os_uname(uv_utsname_t *buffer)
{
  if (!buffer) {
    return UV_EINVAL;
  }
  memset(buffer, 0, sizeof(*buffer));
  uv__strscpy(buffer->sysname, PYXIS_SYSNAME, sizeof(buffer->sysname));
  uv__strscpy(buffer->machine, PYXIS_MACHINE, sizeof(buffer->machine));

  handle_t system_info = startup_resource("system_info");
  struct system_info_identity identity;
  if (system_info == HANDLE_INVALID ||
      system_info_get_identity(system_info, &identity) != CALL_OK) {
    return 0;
  }
  if (identity.architecture[0]) {
    uv__strscpy(buffer->machine, identity.architecture, sizeof(buffer->machine));
  }
  uv__strscpy(buffer->release, identity.build_revision, sizeof(buffer->release));
  uv__strscpy(buffer->version, identity.kernel_name, sizeof(buffer->version));
  return 0;
}
