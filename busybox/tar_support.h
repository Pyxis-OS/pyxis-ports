/* Pyxis support for the selected BusyBox ustar implementation.
 * SPDX-License-Identifier: GPL-2.0-only */
#ifndef PYXIS_BUSYBOX_TAR_SUPPORT_H
#define PYXIS_BUSYBOX_TAR_SUPPORT_H

#include <dirent.h>
#include <stdarg.h>
#include <signal.h>
#include <time.h>
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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
/* Unix bits are only decoded for archive listings, never native permissions. */
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#define S_IFBLK 0060000
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#define ALIGN1 __attribute__((aligned(1)))
#define ALIGN4 __attribute__((aligned(4)))
#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))
#define ENABLE_UNICODE_SUPPORT 0
#define ALIGN_PTR __attribute__((aligned(sizeof(void *))))
#define ALIGN_SUFFIX __attribute__((aligned(4)))
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define UNUSED_PARAM __attribute__((unused))
#define MAIN_EXTERNALLY_VISIBLE
#define PUSH_AND_SET_FUNCTION_VISIBILITY_TO_HIDDEN
#define POP_SAVED_FUNCTION_VISIBILITY
#define ENABLE_LONG_OPTS 0
#define GETOPT_RESET() (optind = 0)
#define FAST_FUNC
#define NOINLINE __attribute__((noinline))
#define FIX_ALIASING __attribute__((may_alias))
#define OFF_FMT "l"
#define TRUE 1
#define FALSE 0
#define SKIP 2
#define ENABLE_FEATURE_TAR_CREATE 1
#define ENABLE_FEATURE_TAR_LONG_OPTIONS 0
#define ENABLE_FEATURE_TAR_NOPRESERVE_TIME 0
#define ENABLE_FEATURE_TAR_TO_COMMAND 0
#define ENABLE_FEATURE_SEAMLESS_GZ 0
#define ENABLE_FEATURE_SEAMLESS_BZ2 0
#define ENABLE_FEATURE_SEAMLESS_XZ 0
#define ENABLE_FEATURE_SEAMLESS_LZMA 0
#define ENABLE_FEATURE_SEAMLESS_Z 0
#define USE_FOR_MMU(...) __VA_ARGS__
#define USE_FOR_NOMMU(...)
#define IF_DESKTOP(...)
#define IF_FEATURE_TAR_CREATE(...) __VA_ARGS__
#define IF_NOT_FEATURE_TAR_CREATE(...)
#define IF_FEATURE_TAR_FROM(...)
#define IF_FEATURE_TAR_TO_COMMAND(...)
#define IF_FEATURE_TAR_LONG_OPTIONS(...)
#define IF_FEATURE_TAR_NOPRESERVE_TIME(...)
#define IF_FEATURE_SEAMLESS_GZ(...)
#define IF_FEATURE_SEAMLESS_BZ2(...)
#define IF_FEATURE_SEAMLESS_XZ(...)
#define IF_FEATURE_SEAMLESS_LZMA(...)
#define IF_FEATURE_SEAMLESS_Z(...)
#define setup_common_bufsiz() ((void)0)
#define init_unicode() ((void)0)
#define LONE_DASH(s) ((s)[0] == '-' && !(s)[1])
#define ENABLE_FEATURE_TAR_FROM 0
/* GNU long-name records: the writer emits ././@LongLink for names and link
 * targets over 99 bytes and the reader consumes them. */
#define ENABLE_FEATURE_TAR_GNU_EXTENSIONS 1
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
typedef signed char smallint;
void overlapping_strcpy(char *dest, const char *source);

typedef struct llist_t {
  struct llist_t *link;
  char *data;
} llist_t;

/* Selected members from pinned BusyBox bb_archive.h; archive numeric IDs
 * are values from the header, not native user/group identities. */
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
  time_t mtime;
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
  llist_t *link_placeholders;
  void (*seek)(int fd, off_t amount);
  off_t offset;
  smallint tar__end;
  /* GNU long-name records: the name or link target of the next member. */
  char *tar__longname;
  char *tar__linkname;
} archive_handle_t;
#define ARCHIVE_RESTORE_DATE (1 << 0)
#define ARCHIVE_CREATE_LEADING_DIRS (1 << 1)
#define ARCHIVE_UNLINK_OLD (1 << 2)
#define ARCHIVE_EXTRACT_NEWER (1 << 3)
#define ARCHIVE_DONT_RESTORE_OWNER (1 << 4)
#define ARCHIVE_DONT_RESTORE_PERM (1 << 5)
#define ARCHIVE_NUMERIC_OWNER (1 << 6)
#define ARCHIVE_O_TRUNC (1 << 7)
#define ARCHIVE_REMEMBER_NAMES (1 << 8)

extern char bb_common_bufsiz1[2 * TAR_BLOCK_SIZE];
void *xmalloc(size_t size);
char *xstrdup(const char *text);
char *xstrndup(const char *text, size_t limit);
void *xzalloc(size_t size);
char *xasprintf(const char *format, ...);
char *concat_path_file(const char *parent, const char *name);
char *last_char_is(const char *text, int character);
void bb_simple_error_msg(const char *message);
void bb_simple_error_msg_and_die(const char *message) __attribute__((noreturn));
void bb_error_msg(const char *format, ...) __attribute__((format(printf, 1, 2)));
void bb_error_msg_and_die(const char *format, ...) __attribute__((noreturn, format(printf, 1, 2)));
void xwrite(int fd, const void *data, size_t size);
void llist_add_to(llist_t **list, void *data);
void llist_add_to_end(llist_t **list, void *data);
void *llist_pop(llist_t **list);
void llist_unlink(llist_t **head, llist_t *elm);
void llist_free(llist_t *elm, void (*freeit)(void *));
llist_t *llist_rev(llist_t *list);
llist_t *llist_find_str(llist_t *list, const char *str);
void data_align(archive_handle_t *archive, unsigned boundary);
void data_skip(archive_handle_t *archive);
void seek_by_read(int fd, off_t amount);

/* Native URI names and directory entries supply the creation callbacks. */
const char *pyxis_tar_source_name(const char *path);
void bb_copyfd_exact_size(int source, int destination, off_t size);
ssize_t full_read(int fd, void *data, size_t size);
void xread(int fd, void *data, size_t size);
const char *skip_unsafe_prefix(const char *name);
void strip_unsafe_prefix(char *name);
void pyxis_tar_check_member(const char *name);
char get_header_tar(archive_handle_t *archive);
void chksum_and_xwrite_tar_header(int fd, tar_header_t *header);
int tar_main(int argc, char **argv);

struct suffix_mult {
  char suffix[4];
  uint32_t mult;
};
#include "xatonum.h"
int xatoi_positive(const char *text);
unsigned string_array_len(char **argv);
extern uint32_t option_mask32;
uint32_t getopt32(char **argv, const char *options, ...);
void bb_show_usage(void) __attribute__((noreturn));
void bb_simple_perror_msg_and_die(const char *message) __attribute__((noreturn));
void bb_perror_msg_and_die(const char *format, ...) __attribute__((noreturn));
int xopen3(const char *path, int flags, int mode);
int open3_or_warn(const char *path, int flags, int mode);
int open_or_warn(const char *path, int flags);
int xopen(const char *path, int flags);
void xfstat(int fd, struct stat *status, const char *message);
void bb_perror_msg(const char *format, ...);
void xfunc_die(void) __attribute__((noreturn));
void bb_die_memory_exhausted(void) __attribute__((noreturn));
#define bb_msg_memory_exhausted "out of memory"
#define bb_msg_read_error "read error"
#define bb_msg_write_error "write error"
#define CONFIG_FEATURE_COPYBUF_KB 4
#define ENABLE_FEATURE_USE_SENDFILE 0
#define LONE_CHAR(s,c) ((s)[0] == (c) && !(s)[1])
#define FILEUTILS_RECUR (1 << 2)
#define FILEUTILS_VERBOSE 0
#define FILEUTILS_IGNORE_CHMOD_ERR (1U << 31)
int bb_make_directory(char *path, long mode, int flags);
void create_or_remember_link(llist_t **list, const char *target, const char *name, int hard_link);
ssize_t safe_read(int fd, void *buffer, size_t count);
ssize_t safe_write(int fd, const void *buffer, size_t count);
ssize_t full_write(int fd, const void *buffer, size_t count);
off_t bb_copyfd_size(int source, int destination, off_t size);
off_t bb_copyfd_eof(int source, int destination);
int chdir_or_warn(const char *path);
void xchdir(const char *path);
char *is_prefixed_with(const char *text, const char *prefix);
char *is_suffixed_with(const char *text, const char *suffix);
const char *printable_string(const char *text);
char *auto_string(char *text);
typedef struct {
  unsigned byte_count;
  unsigned unicode_count;
  unsigned unicode_width;
} uni_stat_t;
char *bb_mode_string(char *buffer, mode_t mode);
char *concat_subpath_file(const char *path, const char *name);
enum {
  ACTION_RECURSE = 1 << 0,
  ACTION_FOLLOWLINKS = 1 << 1,
  ACTION_FOLLOWLINKS_L0 = 1 << 2,
  ACTION_DEPTHFIRST = 1 << 3,
  ACTION_QUIET = 1 << 4,
  ACTION_DANGLING_OK = 1 << 5,
};
typedef struct recursive_state {
  unsigned flags;
  unsigned depth;
  void *userData;
  int (*fileAction)(struct recursive_state *, const char *, struct stat *);
  int (*dirAction)(struct recursive_state *, const char *, struct stat *);
} recursive_state_t;
int recursive_action(const char *path, unsigned flags,
  int (*file_action)(recursive_state_t *, const char *, struct stat *),
  int (*dir_action)(recursive_state_t *, const char *, struct stat *), void *data);
void bb_simple_perror_msg(const char *message);
archive_handle_t *init_handle(void);
void header_skip(archive_handle_t *archive);
void header_list(archive_handle_t *archive);
void header_verbose_list(archive_handle_t *archive);
char filter_accept_all(archive_handle_t *archive);
char filter_accept_reject_list(archive_handle_t *archive);
const llist_t *find_list_entry(const llist_t *list, const char *filename);
const llist_t *find_list_entry2(const llist_t *list, const char *filename);
void data_extract_all(archive_handle_t *archive);
void data_extract_to_stdout(archive_handle_t *archive);
void seek_by_jump(int fd, off_t amount);
void create_links_from_list(llist_t *links);
extern int bb_got_signal;
extern const char *applet_name;
extern uint8_t xfunc_error_retval;
extern void (*die_func)(void);
#define LOGMODE_STDIO 1
#define ENABLE_FEATURE_SYSLOG 0
void bb_verror_msg(const char *format, va_list arguments, const char *error);
int fflush_all(void);
/* Unselected compression branches need declarations, never implementations. */
typedef struct transformer_state_t transformer_state_t;
int open_zipped(const char *path, int fail_if_not_compressed);
void fork_transformer(int fd, int signature, int (*xformer)(transformer_state_t *));
#define fork_transformer_with_sig(fd, transformer, transform_prog) fork_transformer((fd), 0, (transformer))
void check_errors_in_children(int signal_number);
#endif
