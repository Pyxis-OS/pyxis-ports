/* Port-local: descriptor sets for pyxis_select (pyxis.c). Only the virtual
 * pipes and the terminal input (descriptor 0) are ever waited on. */
#ifndef LINKS_PYXIS_SYS_SELECT_H
#define LINKS_PYXIS_SYS_SELECT_H

#include <time.h>

#define FD_SETSIZE 1024
#define LINKS_FD_WORD_BITS (8 * (int)sizeof(unsigned long))

typedef struct {
  unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(unsigned long))];
} fd_set;

#define FD_ZERO(set) __builtin_memset((set), 0, sizeof(fd_set))
#define FD_SET(fd, set) ((set)->fds_bits[(fd) / LINKS_FD_WORD_BITS] |= 1UL << ((fd) % LINKS_FD_WORD_BITS))
#define FD_CLR(fd, set) ((set)->fds_bits[(fd) / LINKS_FD_WORD_BITS] &= ~(1UL << ((fd) % LINKS_FD_WORD_BITS)))
#define FD_ISSET(fd, set) (((set)->fds_bits[(fd) / LINKS_FD_WORD_BITS] >> ((fd) % LINKS_FD_WORD_BITS)) & 1)

#endif
