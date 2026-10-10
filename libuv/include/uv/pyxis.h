#ifndef UV_PYXIS_H
#define UV_PYXIS_H

#include <abi/handle.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "uv/threadpool.h"

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct addrinfo;

typedef struct uv_buf_t {
  char *base;
  size_t len;
} uv_buf_t;

typedef int uv_file;
typedef int uv_os_fd_t;
typedef int uv_os_sock_t;
typedef int uv_pid_t;
typedef unsigned uv_uid_t;
typedef unsigned uv_gid_t;
typedef uintptr_t uv_thread_t;
typedef struct { unsigned state; } uv_once_t;
typedef struct { unsigned depth; unsigned recursive; } uv_mutex_t;
typedef struct { void *value; unsigned initialized; } uv_key_t;
typedef struct { unsigned state; } uv_rwlock_t;
typedef struct { unsigned state; } uv_cond_t;
typedef struct { unsigned state; } uv_sem_t;
typedef struct { unsigned state; } uv_barrier_t;
typedef struct dirent uv__dirent_t;
typedef struct { void *handle; char *errmsg; } uv_lib_t;

#define UV_ONCE_INIT {0}
#define UV_DYNAMIC
#define UV_STAT_DEV_VALID STAT_DEV_VALID
#define UV_STAT_INO_VALID STAT_INO_VALID
#define UV_STAT_MTIME_VALID STAT_MTIME_VALID

#define UV_LOOP_PRIVATE_FIELDS \
  uv_handle_t *closing_handles; \
  struct uv__queue prepare_handles; \
  struct uv__queue check_handles; \
  struct uv__queue idle_handles; \
  struct uv__queue async_handles; \
  struct { void *min; unsigned nelts; } timer_heap; \
  uint64_t timer_counter; \
  uint64_t time; \
  handle_t clock; \
  handle_t wake_reader; \
  handle_t wake_writer; \
  unsigned interest_count;

#define UV_HANDLE_PRIVATE_FIELDS \
  uv_handle_t *next_closing; \
  unsigned flags;

#define UV_STREAM_PRIVATE_FIELDS \
  uv_shutdown_t *shutdown_req; \
  struct uv__queue write_queue; \
  struct uv__queue write_completed_queue; \
  int fd; \
  unsigned access; \
  unsigned admitted;

#define UV_WRITE_PRIVATE_FIELDS \
  struct uv__queue queue; \
  unsigned write_index; \
  size_t write_offset; \
  uv_buf_t *bufs; \
  unsigned nbufs; \
  int error; \
  uv_buf_t bufsml[4];

#define UV_PROCESS_PRIVATE_FIELDS \
  handle_t observer; \
  uint64_t exit_reason; \
  unsigned admitted;

#define UV_TIMER_PRIVATE_FIELDS \
  uv_timer_cb timer_cb; \
  union { void *heap[3]; struct uv__queue queue; } node; \
  uint64_t timeout; \
  uint64_t repeat; \
  uint64_t start_id;

#define UV_ASYNC_PRIVATE_FIELDS \
  uv_async_cb async_cb; \
  struct uv__queue queue; \
  int pending;

#define UV_PREPARE_PRIVATE_FIELDS uv_prepare_cb prepare_cb; struct uv__queue queue;
#define UV_CHECK_PRIVATE_FIELDS uv_check_cb check_cb; struct uv__queue queue;
#define UV_IDLE_PRIVATE_FIELDS uv_idle_cb idle_cb; struct uv__queue queue;
#define UV_DIR_PRIVATE_FIELDS DIR *dir;
#define UV_FS_PRIVATE_FIELDS \
  const char *new_path; \
  char **dir_names; \
  size_t dir_count; \
  size_t dir_index; \
  uv_file file; \
  int flags; \
  mode_t mode; \
  unsigned nbufs; \
  uv_buf_t *bufs; \
  off_t off; \
  uv_uid_t uid; \
  uv_gid_t gid; \
  double atime; \
  double mtime; \
  uv_buf_t bufsml[4];

#define UV_REQ_TYPE_PRIVATE
#define UV_REQ_PRIVATE_FIELDS
#define UV_PRIVATE_REQ_TYPES
#define UV_CONNECT_PRIVATE_FIELDS
#define UV_SHUTDOWN_PRIVATE_FIELDS
#define UV_UDP_SEND_PRIVATE_FIELDS
#define UV_TCP_PRIVATE_FIELDS
#define UV_UDP_PRIVATE_FIELDS
#define UV_PIPE_PRIVATE_FIELDS
#define UV_POLL_PRIVATE_FIELDS
#define UV_GETADDRINFO_PRIVATE_FIELDS struct addrinfo *addrinfo;
#define UV_GETNAMEINFO_PRIVATE_FIELDS
#define UV_WORK_PRIVATE_FIELDS
#define UV_TTY_PRIVATE_FIELDS \
  int mode; \
  handle_t passthrough; \
  uv_tty_t *raw_next; \
  uint64_t resize_generation; \
  void (*resize_cb)(uv_tty_t *, int, int, int);
#define UV_SIGNAL_PRIVATE_FIELDS
#define UV_FS_EVENT_PRIVATE_FIELDS

#define UV_FS_O_RDONLY O_RDONLY
#define UV_FS_O_WRONLY O_WRONLY
#define UV_FS_O_RDWR O_RDWR
#define UV_FS_O_CREAT O_CREAT
#define UV_FS_O_TRUNC O_TRUNC
#define UV_FS_O_EXCL O_EXCL
#define UV_FS_O_APPEND O_APPEND
#define UV_FS_O_NOFOLLOW O_NOFOLLOW
/* Unsupported flags remain distinguishable so open rejects before effects.
 * They stay clear of libc's open flags, which end at O_NOFOLLOW (1 << 12). */
#define UV_FS_O_DIRECT (1 << 13)
#define UV_FS_O_DIRECTORY (1 << 14)
#define UV_FS_O_DSYNC (1 << 15)
#define UV_FS_O_EXLOCK (1 << 16)
#define UV_FS_O_NOATIME (1 << 17)
#define UV_FS_O_NOCTTY (1 << 18)
#define UV_FS_O_NONBLOCK (1 << 19)
#define UV_FS_O_SYMLINK (1 << 20)
#define UV_FS_O_SYNC (1 << 21)
#define UV_FS_O_FILEMAP (1 << 22)
#define UV_FS_O_RANDOM (1 << 23)
#define UV_FS_O_SHORT_LIVED (1 << 24)
#define UV_FS_O_SEQUENTIAL (1 << 25)
#define UV_FS_O_TEMPORARY (1 << 26)

#endif
