/* Port-local: Pyxis libc has no BSD sockets. Links' socket code is compiled
 * but never reached, because every page loads through fopen; the functions in
 * network.c all fail with ENOTSUP. */
#ifndef LINKS_PYXIS_SYS_SOCKET_H
#define LINKS_PYXIS_SYS_SOCKET_H

#include <stdint.h>

typedef int socklen_t;
typedef unsigned short sa_family_t;

struct sockaddr {
  sa_family_t sa_family;
  char sa_data[14];
};

struct sockaddr_storage {
  sa_family_t ss_family;
  char ss_data[126];
} __attribute__((aligned(8)));

#define AF_UNSPEC 0
#define AF_UNIX 1
#define AF_INET 2
#define PF_UNSPEC AF_UNSPEC
#define PF_UNIX AF_UNIX
#define PF_INET AF_INET

#define SOCK_STREAM 1
#define SOCK_DGRAM 2

#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define SO_ERROR 4

int socket(int domain, int type, int protocol);
int connect(int socket, const struct sockaddr *address, socklen_t length);
int bind(int socket, const struct sockaddr *address, socklen_t length);
int listen(int socket, int backlog);
int accept(int socket, struct sockaddr *address, socklen_t *length);
int getsockname(int socket, struct sockaddr *address, socklen_t *length);
int getpeername(int socket, struct sockaddr *address, socklen_t *length);
int getsockopt(int socket, int level, int name, void *value, socklen_t *length);
int setsockopt(int socket, int level, int name, const void *value, socklen_t length);

#endif
