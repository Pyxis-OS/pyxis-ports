#include "internal.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fs_begin(uv_loop_t *loop, uv_fs_t *request, uv_fs_type type, uv_fs_cb callback)
{
  if (!request) {
    return UV_EINVAL;
  }
  request->type = UV_FS;
  request->fs_type = type;
  request->loop = loop;
  request->cb = callback;
  request->result = 0;
  request->ptr = NULL;
  request->path = NULL;
  request->new_path = NULL;
  request->bufs = NULL;
  request->nbufs = 0;
  request->dir_names = NULL;
  request->dir_count = 0;
  request->dir_index = 0;
  memset(&request->statbuf, 0, sizeof(request->statbuf));
  if (callback) {
    request->result = UV_ENOSYS;
    return UV_ENOSYS;
  }
  return 0;
}

static int fs_result(uv_fs_t *request, int result)
{
  request->result = result < 0 ? uv_translate_sys_error(errno) : result;
  return (int)request->result;
}

static int fs_path(uv_fs_t *request, const char *path)
{
  if (!path) {
    request->result = UV_EINVAL;
  } else {
    request->path = uv__strdup(path);
    if (!request->path) {
      request->result = UV_ENOMEM;
    }
  }
  return (int)request->result;
}

static int fs_unsupported(uv_loop_t *loop, uv_fs_t *request, uv_fs_type type,
    uv_fs_cb callback)
{
  int result = fs_begin(loop, request, type, callback);
  if (!result) {
    request->result = UV_ENOSYS;
  }
  return result ? result : UV_ENOSYS;
}

int uv_fs_get_system_error(const uv_fs_t *request)
{
  return request->result < 0 ? (int)-request->result : 0;
}

void uv_fs_req_cleanup(uv_fs_t *request)
{
  if (!request) {
    return;
  }
  for (size_t i = 0; i < request->dir_count; ++i) {
    uv__free(request->dir_names[i]);
  }
  uv__free(request->dir_names);
  if (request->fs_type == UV_FS_SCANDIR) {
    uv__free(request->ptr);
  }
  uv__free((void *)request->path);
  uv__free((void *)request->new_path);
  request->path = NULL;
  request->new_path = NULL;
  request->ptr = NULL;
  request->dir_names = NULL;
  request->dir_count = 0;
  request->dir_index = 0;
}

int uv_fs_open(uv_loop_t *loop, uv_fs_t *request, const char *path, int flags,
    int mode, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_OPEN, callback);
  if (!result && (flags & ~(O_WRONLY | O_RDWR | O_CREAT | O_TRUNC | O_EXCL | O_APPEND))) {
    return request->result = UV_ENOSYS;
  }
  if (result || (result = fs_path(request, path))) {
    return result;
  }
  return fs_result(request, open(path, flags, (mode_t)mode));
}

int uv_fs_close(uv_loop_t *loop, uv_fs_t *request, uv_file file, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_CLOSE, callback);
  return result ? result : fs_result(request, close(file));
}

static ssize_t fs_transfer(uv_loop_t *loop, uv_fs_t *request, uv_file file,
    const uv_buf_t buffers[], unsigned count, int64_t offset, uv_fs_cb callback,
    bool writing)
{
  int result = fs_begin(loop, request, writing ? UV_FS_WRITE : UV_FS_READ, callback);
  if (result) {
    return result;
  }
  if (!buffers || !count || offset < -1) {
    return request->result = UV_EINVAL;
  }
  size_t total = 0;
  for (unsigned i = 0; i < count; ++i) {
    if (buffers[i].len > (size_t)INT_MAX - total ||
        (buffers[i].len && !buffers[i].base)) {
      return request->result = UV_EINVAL;
    }
    total += buffers[i].len;
  }
  if (offset >= 0 && total > (uint64_t)INT64_MAX - (uint64_t)offset) {
    return request->result = UV_EINVAL;
  }
  ssize_t completed = 0;
  for (unsigned i = 0; i < count; ++i) {
    ssize_t done;
    if (offset < 0) {
      done = writing ? write(file, buffers[i].base, buffers[i].len) :
          read(file, buffers[i].base, buffers[i].len);
    } else {
      done = writing ? pwrite(file, buffers[i].base, buffers[i].len, offset + completed) :
          pread(file, buffers[i].base, buffers[i].len, offset + completed);
    }
    if (done < 0) {
      if (!completed) {
        completed = uv_translate_sys_error(errno);
      }
      break;
    }
    completed += done;
    if ((size_t)done < buffers[i].len) {
      break;
    }
  }
  return request->result = completed;
}

int uv_fs_read(uv_loop_t *loop, uv_fs_t *request, uv_file file,
    const uv_buf_t buffers[], unsigned count, int64_t offset, uv_fs_cb callback)
{
  return (int)fs_transfer(loop, request, file, buffers, count, offset, callback, false);
}

int uv_fs_write(uv_loop_t *loop, uv_fs_t *request, uv_file file,
    const uv_buf_t buffers[], unsigned count, int64_t offset, uv_fs_cb callback)
{
  return (int)fs_transfer(loop, request, file, buffers, count, offset, callback, true);
}

static void copy_stat(uv_stat_t *result, const struct stat *metadata)
{
  *result = (uv_stat_t){
    .st_mode = metadata->st_mode,
    .st_size = metadata->st_size,
    .stat_valid = metadata->st_valid,
  };
  if (metadata->st_valid & STAT_DEV_VALID) {
    result->st_dev = metadata->st_dev;
  }
  if (metadata->st_valid & STAT_INO_VALID) {
    result->st_ino = metadata->st_ino;
  }
  if (metadata->st_valid & STAT_MTIME_VALID) {
    result->st_mtim.tv_sec = metadata->st_mtim.tv_sec;
    result->st_mtim.tv_nsec = metadata->st_mtim.tv_nsec;
  }
}

static int fs_stat(uv_loop_t *loop, uv_fs_t *request, const char *path, int file,
    uv_fs_cb callback, uv_fs_type type)
{
  int result = fs_begin(loop, request, type, callback);
  if (result || (type != UV_FS_FSTAT && (result = fs_path(request, path)))) {
    return result;
  }
  struct stat metadata;
  result = type == UV_FS_FSTAT ? fstat(file, &metadata) :
      type == UV_FS_LSTAT ? lstat(path, &metadata) : stat(path, &metadata);
  if (result < 0) {
    return fs_result(request, result);
  }
  copy_stat(&request->statbuf, &metadata);
  request->ptr = &request->statbuf;
  return 0;
}

int uv_fs_stat(uv_loop_t *loop, uv_fs_t *request, const char *path, uv_fs_cb callback)
{
  return fs_stat(loop, request, path, -1, callback, UV_FS_STAT);
}

int uv_fs_lstat(uv_loop_t *loop, uv_fs_t *request, const char *path, uv_fs_cb callback)
{
  return fs_stat(loop, request, path, -1, callback, UV_FS_LSTAT);
}

int uv_fs_fstat(uv_loop_t *loop, uv_fs_t *request, uv_file file, uv_fs_cb callback)
{
  return fs_stat(loop, request, NULL, file, callback, UV_FS_FSTAT);
}

int uv_fs_fsync(uv_loop_t *loop, uv_fs_t *request, uv_file file, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_FSYNC, callback);
  return result ? result : fs_result(request, fsync(file));
}

int uv_fs_fdatasync(uv_loop_t *loop, uv_fs_t *request, uv_file file, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_FDATASYNC, callback);
  return result ? result : fs_result(request, fsync(file));
}

int uv_fs_ftruncate(uv_loop_t *loop, uv_fs_t *request, uv_file file, int64_t length,
    uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_FTRUNCATE, callback);
  return result ? result : fs_result(request, ftruncate(file, length));
}

int uv_fs_access(uv_loop_t *loop, uv_fs_t *request, const char *path, int mode,
    uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_ACCESS, callback);
  if (result || (result = fs_path(request, path))) {
    return result;
  }
  return fs_result(request, access(path, mode));
}

int uv_fs_mkdir(uv_loop_t *loop, uv_fs_t *request, const char *path, int mode,
    uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_MKDIR, callback);
  if (result || (result = fs_path(request, path))) {
    return result;
  }
  return fs_result(request, mkdir(path, mode));
}

int uv_fs_rmdir(uv_loop_t *loop, uv_fs_t *request, const char *path, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_RMDIR, callback);
  if (result || (result = fs_path(request, path))) {
    return result;
  }
  return fs_result(request, rmdir(path));
}

int uv_fs_unlink(uv_loop_t *loop, uv_fs_t *request, const char *path, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_UNLINK, callback);
  if (result || (result = fs_path(request, path))) {
    return result;
  }
  return fs_result(request, unlink(path));
}

int uv_fs_rename(uv_loop_t *loop, uv_fs_t *request, const char *path,
    const char *new_path, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_RENAME, callback);
  if (result || (result = fs_path(request, path))) {
    return result;
  }
  if (!new_path) {
    return request->result = UV_EINVAL;
  }
  request->new_path = uv__strdup(new_path);
  if (!request->new_path) {
    return request->result = UV_ENOMEM;
  }
  return fs_result(request, rename(path, new_path));
}

static uv_dirent_type_t directory_type(unsigned char type)
{
  switch (type) {
  case DT_REG: return UV_DIRENT_FILE;
  case DT_DIR: return UV_DIRENT_DIR;
  case DT_LNK: return UV_DIRENT_LINK;
  default: return UV_DIRENT_UNKNOWN;
  }
}

static int compare_entries(const void *first, const void *second)
{
  const uv_dirent_t *a = first, *b = second;
  return strcmp(a->name, b->name);
}

int uv_fs_scandir(uv_loop_t *loop, uv_fs_t *request, const char *path, int flags,
    uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_SCANDIR, callback);
  if (result) {
    return result;
  }
  if (flags) {
    return request->result = UV_EINVAL;
  }
  if ((result = fs_path(request, path))) {
    return result;
  }
  DIR *directory = opendir(path);
  if (!directory) {
    return fs_result(request, -1);
  }
  size_t capacity = 0;
  struct dirent *entry;
  int error = 0;
  for (;;) {
    errno = 0;
    entry = readdir(directory);
    if (!entry) {
      error = errno ? uv_translate_sys_error(errno) : 0;
      break;
    }
    if (request->dir_count == (size_t)INT_MAX) {
      error = UV_EOVERFLOW;
      break;
    }
    if (request->dir_count == capacity) {
      size_t next = capacity ? capacity * 2 : 16;
      if (next > (size_t)INT_MAX) {
        next = INT_MAX;
      }
      uv_dirent_t *entries = uv__realloc(request->ptr, next * sizeof(*entries));
      if (!entries) {
        error = UV_ENOMEM;
        break;
      }
      request->ptr = entries;
      char **names = uv__realloc(request->dir_names, next * sizeof(*names));
      if (!names) {
        error = UV_ENOMEM;
        break;
      }
      request->dir_names = names;
      capacity = next;
    }
    char *name = uv__strdup(entry->d_name);
    if (!name) {
      error = UV_ENOMEM;
      break;
    }
    request->dir_names[request->dir_count] = name;
    ((uv_dirent_t *)request->ptr)[request->dir_count++] =
        (uv_dirent_t){.name = name, .type = directory_type(entry->d_type)};
  }
  if (closedir(directory) < 0 && !error) {
    error = uv_translate_sys_error(errno);
  }
  if (error) {
    return request->result = error;
  }
  qsort(request->ptr, request->dir_count, sizeof(uv_dirent_t), compare_entries);
  return request->result = (int)request->dir_count;
}

int uv_fs_scandir_next(uv_fs_t *request, uv_dirent_t *entry)
{
  if (!request || !entry || request->fs_type != UV_FS_SCANDIR || request->result < 0) {
    return UV_EINVAL;
  }
  if (request->dir_index == request->dir_count) {
    return UV_EOF;
  }
  *entry = ((uv_dirent_t *)request->ptr)[request->dir_index++];
  return 0;
}

int uv_fs_opendir(uv_loop_t *loop, uv_fs_t *request, const char *path, uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_OPENDIR, callback);
  if (result || (result = fs_path(request, path))) {
    return result;
  }
  uv_dir_t *directory = uv__calloc(1, sizeof(*directory));
  if (!directory) {
    return request->result = UV_ENOMEM;
  }
  directory->dir = opendir(path);
  if (!directory->dir) {
    uv__free(directory);
    return fs_result(request, -1);
  }
  request->ptr = directory;
  return 0;
}

int uv_fs_readdir(uv_loop_t *loop, uv_fs_t *request, uv_dir_t *directory,
    uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_READDIR, callback);
  if (result) {
    return result;
  }
  if (!directory || !directory->dir || !directory->dirents || !directory->nentries ||
      directory->nentries > INT_MAX) {
    return request->result = UV_EINVAL;
  }
  request->dir_names = uv__calloc(directory->nentries, sizeof(char *));
  if (!request->dir_names) {
    return request->result = UV_ENOMEM;
  }
  while (request->dir_count < directory->nentries) {
    errno = 0;
    struct dirent *entry = readdir(directory->dir);
    if (!entry) {
      if (errno) {
        return fs_result(request, -1);
      }
      break;
    }
    char *name = uv__strdup(entry->d_name);
    if (!name) {
      return request->result = UV_ENOMEM;
    }
    size_t index = request->dir_count++;
    request->dir_names[index] = name;
    directory->dirents[index] = (uv_dirent_t){.name = name, .type = directory_type(entry->d_type)};
  }
  return request->result = (int)request->dir_count;
}

int uv_fs_closedir(uv_loop_t *loop, uv_fs_t *request, uv_dir_t *directory,
    uv_fs_cb callback)
{
  int result = fs_begin(loop, request, UV_FS_CLOSEDIR, callback);
  if (result) {
    return result;
  }
  if (!directory || !directory->dir) {
    return request->result = UV_EINVAL;
  }
  result = closedir(directory->dir);
  uv__free(directory);
  return fs_result(request, result);
}

int uv_fs_copyfile(uv_loop_t *loop, uv_fs_t *request, const char *path, const char *new_path, int flags,
    uv_fs_cb callback)
{
  (void)path;
  (void)new_path;
  (void)flags;
  return fs_unsupported(loop, request, UV_FS_COPYFILE, callback);
}

int uv_fs_mkdtemp(uv_loop_t *loop, uv_fs_t *request, const char *template,
    uv_fs_cb callback)
{
  (void)template;
  return fs_unsupported(loop, request, UV_FS_MKDTEMP, callback);
}

int uv_fs_mkstemp(uv_loop_t *loop, uv_fs_t *request, const char *template,
    uv_fs_cb callback)
{
  (void)template;
  return fs_unsupported(loop, request, UV_FS_MKSTEMP, callback);
}

int uv_fs_sendfile(uv_loop_t *loop, uv_fs_t *request, uv_file output, uv_file input, int64_t offset, size_t length,
    uv_fs_cb callback)
{
  (void)output;
  (void)input;
  (void)offset;
  (void)length;
  return fs_unsupported(loop, request, UV_FS_SENDFILE, callback);
}

int uv_fs_chmod(uv_loop_t *loop, uv_fs_t *request, const char *path, int mode,
    uv_fs_cb callback)
{
  (void)path;
  (void)mode;
  return fs_unsupported(loop, request, UV_FS_CHMOD, callback);
}

int uv_fs_fchmod(uv_loop_t *loop, uv_fs_t *request, uv_file file, int mode,
    uv_fs_cb callback)
{
  (void)file;
  (void)mode;
  return fs_unsupported(loop, request, UV_FS_FCHMOD, callback);
}

int uv_fs_utime(uv_loop_t *loop, uv_fs_t *request, const char *path, double atime, double mtime,
    uv_fs_cb callback)
{
  (void)path;
  (void)atime;
  (void)mtime;
  return fs_unsupported(loop, request, UV_FS_UTIME, callback);
}

int uv_fs_futime(uv_loop_t *loop, uv_fs_t *request, uv_file file, double atime, double mtime,
    uv_fs_cb callback)
{
  (void)file;
  (void)atime;
  (void)mtime;
  return fs_unsupported(loop, request, UV_FS_FUTIME, callback);
}

int uv_fs_lutime(uv_loop_t *loop, uv_fs_t *request, const char *path, double atime, double mtime,
    uv_fs_cb callback)
{
  (void)path;
  (void)atime;
  (void)mtime;
  return fs_unsupported(loop, request, UV_FS_LUTIME, callback);
}

int uv_fs_link(uv_loop_t *loop, uv_fs_t *request, const char *path, const char *new_path,
    uv_fs_cb callback)
{
  (void)path;
  (void)new_path;
  return fs_unsupported(loop, request, UV_FS_LINK, callback);
}

int uv_fs_symlink(uv_loop_t *loop, uv_fs_t *request, const char *path, const char *new_path, int flags,
    uv_fs_cb callback)
{
  (void)path;
  (void)new_path;
  (void)flags;
  return fs_unsupported(loop, request, UV_FS_SYMLINK, callback);
}

int uv_fs_readlink(uv_loop_t *loop, uv_fs_t *request, const char *path,
    uv_fs_cb callback)
{
  (void)path;
  return fs_unsupported(loop, request, UV_FS_READLINK, callback);
}

int uv_fs_realpath(uv_loop_t *loop, uv_fs_t *request, const char *path,
    uv_fs_cb callback)
{
  (void)path;
  return fs_unsupported(loop, request, UV_FS_REALPATH, callback);
}

int uv_fs_chown(uv_loop_t *loop, uv_fs_t *request, const char *path, uv_uid_t uid, uv_gid_t gid,
    uv_fs_cb callback)
{
  (void)path;
  (void)uid;
  (void)gid;
  return fs_unsupported(loop, request, UV_FS_CHOWN, callback);
}

int uv_fs_fchown(uv_loop_t *loop, uv_fs_t *request, uv_file file, uv_uid_t uid, uv_gid_t gid,
    uv_fs_cb callback)
{
  (void)file;
  (void)uid;
  (void)gid;
  return fs_unsupported(loop, request, UV_FS_FCHOWN, callback);
}

int uv_fs_lchown(uv_loop_t *loop, uv_fs_t *request, const char *path, uv_uid_t uid, uv_gid_t gid,
    uv_fs_cb callback)
{
  (void)path;
  (void)uid;
  (void)gid;
  return fs_unsupported(loop, request, UV_FS_LCHOWN, callback);
}

int uv_fs_statfs(uv_loop_t *loop, uv_fs_t *request, const char *path,
    uv_fs_cb callback)
{
  (void)path;
  return fs_unsupported(loop, request, UV_FS_STATFS, callback);
}
