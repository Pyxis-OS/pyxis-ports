/* Pyxis support for the selected BusyBox ustar implementation.
 * SPDX-License-Identifier: GPL-2.0-only */
#ifndef PYXIS_BUSYBOX_TAR_SUPPORT_H
#define PYXIS_BUSYBOX_TAR_SUPPORT_H

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FAST_FUNC
#define NOINLINE __attribute__((noinline))
#define FIX_ALIASING __attribute__((may_alias))
#define OFF_FMT "l"
#define TRUE 1
#define FALSE 0
#define SKIP 2
#define ENABLE_FEATURE_TAR_CREATE 1
#define ENABLE_FEATURE_TAR_FROM 0
#define ENABLE_FEATURE_TAR_GNU_EXTENSIONS 0
#define ENABLE_FEATURE_TAR_SELINUX 0
#define ENABLE_FEATURE_TAR_OLDSUN_COMPATIBILITY 0
#define ENABLE_FEATURE_TAR_OLDGNU_COMPATIBILITY 0
#define ENABLE_FEATURE_TAR_UNAME_GNAME 0
#define ENABLE_FEATURE_TAR_AUTODETECT 0
#define ENABLE_DESKTOP 0
#define ENABLE_FEATURE_CLEAN_UP 0
#define SEAMLESS_COMPRESSION 0
#define IF_FEATURE_TAR_OLDSUN_COMPATIBILITY(...)
#define IF_FEATURE_TAR_OLDGNU_COMPATIBILITY(...)
#define IF_FEATURE_TAR_AUTODETECT(...)

typedef uint64_t uoff_t;

typedef struct llist_t {
  struct llist_t *link;
  char *data;
} llist_t;

/* Layout copied from the pinned BusyBox include/bb_archive.h. */
#define TAR_BLOCK_SIZE 512
#define NAME_SIZE 100
#define NAME_SIZE_STR "100"
typedef struct tar_header_t {
  char name[NAME_SIZE];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[NAME_SIZE];
  char magic[8];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char padding[12];
} tar_header_t;
_Static_assert(sizeof(tar_header_t) == TAR_BLOCK_SIZE, "ustar header size");

typedef struct file_header_t {
  char *name;
  char *link_target;
  off_t size;
  uint64_t uid;
  uint64_t gid;
  mode_t mode;
  uint64_t mtime;
} file_header_t;

typedef struct archive_handle_t {
  unsigned ah_flags;
  int src_fd;
  file_header_t *file_header;
  llist_t *accept;
  llist_t *reject;
  llist_t *passed;
  char (*filter)(struct archive_handle_t *);
  void (*action_header)(struct archive_handle_t *);
  void (*action_data)(struct archive_handle_t *);
  off_t offset;
} archive_handle_t;
#define ARCHIVE_REMEMBER_NAMES (1 << 8)

extern char bb_common_bufsiz1[2 * TAR_BLOCK_SIZE];
void *xmalloc(size_t size);
char *xstrdup(const char *text);
char *xstrndup(const char *text, size_t limit);
char *concat_path_file(const char *parent, const char *name);
char *last_char_is(const char *text, int character);
void bb_simple_error_msg(const char *message);
void bb_simple_error_msg_and_die(const char *message) __attribute__((noreturn));
void bb_error_msg(const char *format, ...) __attribute__((format(printf, 1, 2)));
void bb_error_msg_and_die(const char *format, ...) __attribute__((noreturn, format(printf, 1, 2)));
void xwrite(int fd, const void *data, size_t size);
void llist_add_to(llist_t **list, char *data);
void data_align(archive_handle_t *archive, unsigned boundary);
void data_skip(archive_handle_t *archive);
void seek_by_read(int fd, off_t amount);

/* Snapshot interfaces replace only I/O in the upstream algorithms. All source
 * bytes were read through libc before the first extraction/archive write. */
const char *pyxis_tar_source_name(const char *path);
struct stat *pyxis_tar_source_stat(const char *path);
void pyxis_tar_copy_source(const char *path, int fd, off_t size);
ssize_t pyxis_tar_snapshot_read(int fd, void *data, size_t size);
void pyxis_tar_snapshot_xread(int fd, void *data, size_t size);
bool pyxis_tar_zero_block(const tar_header_t *header);
void pyxis_tar_check_member(const char *name);
int pyxis_tar_write_archive(int fd, const llist_t *files);
char get_header_tar(archive_handle_t *archive);
void chksum_and_xwrite_tar_header(int fd, tar_header_t *header);
int tar_main(int argc, char **argv);

#endif
