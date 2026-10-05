/* Links' socket and DNS code is compiled but unreachable: http://, https://
 * and local pages all load through fopen (pyxis.c). Name lookup finds
 * nothing, so ftp:// and finger:// stop at "Host not found"; any socket call
 * fails with ENOTSUP.
 *
 * Part of the Pyxis port of Links; distributed under the GPL like Links
 * (see COPYING in the Links source). */
#include <errno.h>
#include <netdb.h>
#include <stddef.h>
#include <sys/socket.h>

static int unsupported(void)
{
  errno = ENOTSUP;
  return -1;
}

int socket(int domain, int type, int protocol)
{
  return unsupported();
}

int connect(int socket, const struct sockaddr *address, socklen_t length)
{
  return unsupported();
}

int bind(int socket, const struct sockaddr *address, socklen_t length)
{
  return unsupported();
}

int listen(int socket, int backlog)
{
  return unsupported();
}

int accept(int socket, struct sockaddr *address, socklen_t *length)
{
  return unsupported();
}

int getsockname(int socket, struct sockaddr *address, socklen_t *length)
{
  return unsupported();
}

int getpeername(int socket, struct sockaddr *address, socklen_t *length)
{
  return unsupported();
}

int getsockopt(int socket, int level, int name, void *value, socklen_t *length)
{
  return unsupported();
}

int setsockopt(int socket, int level, int name, const void *value, socklen_t length)
{
  return unsupported();
}

struct hostent *gethostbyname(const char *name)
{
  return NULL;
}
