/* Native I/O and bounded command interface for BusyBox tar.
 * SPDX-License-Identifier: GPL-2.0-only */
#include "tar_support.h"

#include <dirent.h>
#include <stdarg.h>

struct tar_entry {
  struct tar_entry *next;
  char *path;
  char *member;
  struct stat status;
  unsigned char *bytes;
  size_t data_offset;
};

static struct tar_entry *entries;
static struct tar_entry **entries_tail = &entries;
static unsigned char *archive_bytes;
static size_t archive_size;
static size_t archive_position;
char bb_common_bufsiz1[2 * TAR_BLOCK_SIZE];

void bb_simple_error_msg(const char *message)
{
  fprintf(stderr, "tar: %s\n", message);
}

void bb_simple_error_msg_and_die(const char *message)
{
  bb_simple_error_msg(message);
  exit(EXIT_FAILURE);
}

void bb_error_msg(const char *format, ...)
{
  va_list arguments;
  va_start(arguments, format);
  fputs("tar: ", stderr);
  vfprintf(stderr, format, arguments);
  fputc('\n', stderr);
  va_end(arguments);
}

void bb_error_msg_and_die(const char *format, ...)
{
  va_list arguments;
  va_start(arguments, format);
  fputs("tar: ", stderr);
  vfprintf(stderr, format, arguments);
  fputc('\n', stderr);
  va_end(arguments);
  exit(EXIT_FAILURE);
}

static void path_error(const char *path)
{
  bb_error_msg_and_die("%s: %s", path, strerror(errno));
}

void *xmalloc(size_t size)
{
  void *memory = malloc(size ? size : 1);
  if (!memory) {
    bb_simple_error_msg_and_die("out of memory");
  }
  return memory;
}

char *xstrndup(const char *text, size_t limit)
{
  size_t size = strnlen(text, limit);
  char *copy = xmalloc(size + 1);
  memcpy(copy, text, size);
  copy[size] = '\0';
  return copy;
}

char *xstrdup(const char *text)
{
  return xstrndup(text, strlen(text));
}

char *concat_path_file(const char *parent, const char *name)
{
  size_t parent_size = strlen(parent);
  size_t name_size = strlen(name);
  if (parent_size > SIZE_MAX - name_size - 2) {
    bb_simple_error_msg_and_die("path too long");
  }
  char *path = xmalloc(parent_size + name_size + 2);
  memcpy(path, parent, parent_size);
  path[parent_size] = '/';
  memcpy(path + parent_size + 1, name, name_size + 1);
  return path;
}

char *last_char_is(const char *text, int character)
{
  size_t size = strlen(text);
  return size && text[size - 1] == character ? (char *)text + size - 1 : NULL;
}

void llist_add_to(llist_t **list, char *data)
{
  llist_t *node = xmalloc(sizeof(*node));
  *node = (llist_t) { .link = *list, .data = data };
  *list = node;
}

void xwrite(int fd, const void *data, size_t size)
{
  const unsigned char *bytes = data;
  while (size) {
    ssize_t count = write(fd, bytes, size);
    if (count <= 0) {
      if (!count) {
        errno = EIO;
      }
      path_error("archive/file write");
    }
    bytes += count;
    size -= (size_t)count;
  }
}

static unsigned char *read_snapshot(const char *path, size_t *size)
{
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    path_error(path);
  }
  struct stat status;
  if (fstat(fd, &status) < 0) {
    path_error(path);
  }
  if (!S_ISREG(status.st_mode)) {
    bb_error_msg_and_die("%s: regular file required", path);
  }

  size_t capacity = 4096;
  size_t used = 0;
  unsigned char *bytes = xmalloc(capacity);
  for (;;) {
    if (used == capacity) {
      if (capacity > SIZE_MAX / 2) {
        bb_simple_error_msg_and_die("file too large to snapshot");
      }
      capacity *= 2;
      void *larger = realloc(bytes, capacity);
      if (!larger) {
        bb_simple_error_msg_and_die("out of memory");
      }
      bytes = larger;
    }
    ssize_t count = read(fd, bytes + used, capacity - used);
    if (count < 0) {
      path_error(path);
    }
    if (!count) {
      break;
    }
    used += (size_t)count;
  }
  if (close(fd) < 0) {
    path_error(path);
  }
  *size = used;
  return bytes;
}

ssize_t pyxis_tar_snapshot_read(int fd, void *data, size_t size)
{
  (void)fd;
  size_t available = archive_size - archive_position;
  if (size > available) {
    size = available;
  }
  memcpy(data, archive_bytes + archive_position, size);
  archive_position += size;
  return (ssize_t)size;
}

void pyxis_tar_snapshot_xread(int fd, void *data, size_t size)
{
  if ((size_t)pyxis_tar_snapshot_read(fd, data, size) != size) {
    bb_simple_error_msg_and_die("truncated tar archive");
  }
}

bool pyxis_tar_zero_block(const tar_header_t *header)
{
  const unsigned char *bytes = (const unsigned char *)header;
  for (size_t i = 0; i < sizeof(*header); i++) {
    if (bytes[i]) {
      return false;
    }
  }
  return true;
}

void seek_by_read(int fd, off_t amount)
{
  (void)fd;
  if (amount < 0 || (uint64_t)amount > archive_size - archive_position) {
    bb_simple_error_msg_and_die("truncated tar member data");
  }
  archive_position += (size_t)amount;
}

void data_align(archive_handle_t *archive, unsigned boundary)
{
  off_t padding = (-(uint64_t)archive->offset) & (boundary - 1);
  seek_by_read(archive->src_fd, padding);
  archive->offset += padding;
}

void data_skip(archive_handle_t *archive)
{
  seek_by_read(archive->src_fd, archive->file_header->size);
}

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

/* Canonical relative names make file/directory conflicts visible in preflight;
 * the safety check must run before removing dot or empty components. */
static char *member_path(const char *name)
{
  pyxis_tar_check_member(name);
  char *path = xmalloc(strlen(name) + 2);
  size_t used = 0;
  const char *component = name;
  while (*component) {
    const char *end = strchr(component, '/');
    size_t size = end ? (size_t)(end - component) : strlen(component);
    if (size && !(size == 1 && component[0] == '.')) {
      if (used) {
        path[used++] = '/';
      }
      memcpy(path + used, component, size);
      used += size;
    }
    if (!end) {
      break;
    }
    component = end + 1;
  }
  if (!used) {
    path[used++] = '.';
  }
  path[used] = '\0';
  return path;
}

static void append_entry(struct tar_entry *entry)
{
  *entries_tail = entry;
  entries_tail = &entry->next;
}

static void snapshot_source(const char *path, const char *name)
{
  char *member = member_path(name);
  if (strlen(member) >= NAME_SIZE) {
    bb_error_msg_and_die("%s: archive member name exceeds 99 bytes", path);
  }
  struct stat status;
  if (stat(path, &status) < 0) {
    path_error(path);
  }
  if (!S_ISREG(status.st_mode) && !S_ISDIR(status.st_mode)) {
    bb_error_msg_and_die("%s: only regular files and directories are supported", path);
  }

  struct tar_entry *entry = xmalloc(sizeof(*entry));
  *entry = (struct tar_entry) {
    .path = xstrdup(path), .member = member, .status = status,
  };
  if (S_ISREG(status.st_mode)) {
    if (!strcmp(member, ".")) {
      bb_error_msg_and_die("%s: invalid regular file name", path);
    }
    size_t size;
    entry->bytes = read_snapshot(path, &size);
    if (size > 0777777777777ULL) {
      bb_error_msg_and_die("%s: file exceeds ustar size field", path);
    }
    entry->status.st_size = (off_t)size;
  }
  append_entry(entry);
  if (S_ISDIR(status.st_mode)) {
    DIR *directory = opendir(path);
    if (!directory) {
      path_error(path);
    }
    for (;;) {
      errno = 0;
      struct dirent *child = readdir(directory);
      if (!child) {
        if (errno) {
          path_error(path);
        }
        break;
      }
      if (!strcmp(child->d_name, ".") || !strcmp(child->d_name, "..")) {
        continue;
      }
      if (child->d_type != DT_REG && child->d_type != DT_DIR) {
        bb_error_msg_and_die("%s/%s: unsupported directory entry", path, child->d_name);
      }
      char *child_path = concat_path_file(path, child->d_name);
      char *child_member = concat_path_file(member, child->d_name);
      snapshot_source(child_path, child_member);
      free(child_path);
      free(child_member);
    }
    if (closedir(directory) < 0) {
      path_error(path);
    }
  }
}

/* The creation list borrows each entry path pointer, whose allocation stays
 * alive until process exit. Pointer identity selects duplicate operands too. */
static struct tar_entry *find_source(const char *path)
{
  for (struct tar_entry *entry = entries; entry; entry = entry->next) {
    if (entry->path == path) {
      return entry;
    }
  }
  bb_simple_error_msg_and_die("source snapshot missing");
}

const char *pyxis_tar_source_name(const char *path)
{
  return find_source(path)->member;
}

void pyxis_tar_copy_source(const char *path, int fd, off_t size)
{
  struct tar_entry *entry = find_source(path);
  if (size != entry->status.st_size) {
    bb_simple_error_msg_and_die("source snapshot size changed");
  }
  xwrite(fd, entry->bytes, (size_t)size);
}

struct stat *pyxis_tar_source_stat(const char *path)
{
  return &find_source(path)->status;
}

static char accept_member(archive_handle_t *archive)
{
  (void)archive;
  return EXIT_SUCCESS;
}

static void remember_member(archive_handle_t *archive)
{
  file_header_t *header = archive->file_header;
  struct tar_entry *entry = xmalloc(sizeof(*entry));
  *entry = (struct tar_entry) {
    .member = member_path(header->name),
    .status = { .st_mode = header->mode, .st_size = header->size },
    .data_offset = archive_position,
  };
  if (!strcmp(entry->member, ".") && !S_ISDIR(header->mode)) {
    bb_simple_error_msg_and_die("invalid regular file member name");
  }
  append_entry(entry);
}

static bool path_descends_from(const char *path, const char *parent)
{
  size_t size = strlen(parent);
  return strncmp(path, parent, size) == 0 && path[size] == '/';
}

static void validate_member_tree(void)
{
  for (struct tar_entry *entry = entries; entry; entry = entry->next) {
    for (struct tar_entry *other = entry->next; other; other = other->next) {
      if ((!strcmp(entry->member, other->member)
          && S_ISDIR(entry->status.st_mode) != S_ISDIR(other->status.st_mode))
        || (S_ISREG(entry->status.st_mode)
          && path_descends_from(other->member, entry->member))
        || (S_ISREG(other->status.st_mode)
          && path_descends_from(entry->member, other->member))) {
        bb_error_msg_and_die("conflicting tar members: %s and %s", entry->member, other->member);
      }
    }
  }
}

static void preflight_archive(const char *path)
{
  archive_bytes = read_snapshot(path, &archive_size);
  file_header_t header = {0};
  archive_handle_t archive = {
    .file_header = &header,
    .filter = accept_member,
    .action_header = remember_member,
    .action_data = data_skip,
  };
  while (get_header_tar(&archive) == EXIT_SUCCESS) {
    continue;
  }
  validate_member_tree();
}

static void ensure_directory(const char *path)
{
  struct stat status;
  if (stat(path, &status) == 0) {
    if (!S_ISDIR(status.st_mode)) {
      bb_error_msg_and_die("%s: directory required", path);
    }
    return;
  }
  if (errno != ENOENT) {
    path_error(path);
  }
  if (mkdir(path, 0777) < 0) {
    path_error(path);
  }
}

static void extract_archive(void)
{
  for (struct tar_entry *entry = entries; entry; entry = entry->next) {
    char *path = entry->member;
    for (char *slash = strchr(path, '/'); slash; slash = strchr(slash + 1, '/')) {
      *slash = '\0';
      ensure_directory(path);
      *slash = '/';
    }
    if (S_ISDIR(entry->status.st_mode)) {
      ensure_directory(path);
    } else {
      int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (fd < 0) {
        path_error(path);
      }
      xwrite(fd, archive_bytes + entry->data_offset, (size_t)entry->status.st_size);
      if (close(fd) < 0) {
        path_error(path);
      }
    }
  }
}

static int create_archive(const char *archive_path, int count, char **paths)
{
  for (int i = 0; i < count; i++) {
    const char *name = paths[i];
    const char *scheme = strstr(name, "://");
    if (scheme) {
      name = scheme + 3;
      if (!*name) {
        bb_error_msg_and_die("%s: name a file or directory beneath the scheme root", paths[i]);
      }
    }
    snapshot_source(paths[i], name);
  }
  validate_member_tree();
  llist_t *files = NULL;
  llist_t **tail = &files;
  for (struct tar_entry *entry = entries; entry; entry = entry->next) {
    llist_t *node = xmalloc(sizeof(*node));
    *node = (llist_t) { .data = entry->path };
    *tail = node;
    tail = &node->link;
  }
  int fd = open(archive_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) {
    path_error(archive_path);
  }
  return pyxis_tar_write_archive(fd, files);
}

int tar_main(int argc, char **argv)
{
  if (argc < 3) {
    bb_simple_error_msg_and_die("usage: tar {tf|xf} ARCHIVE | tar cf ARCHIVE PATH...");
  }
  const char *options = argv[1];
  if (*options == '-') {
    options++;
  }
  if (!strcmp(argv[2], "-")) {
    bb_simple_error_msg_and_die("stdin/stdout archives are unsupported");
  }
  if (!strcmp(options, "cf")) {
    if (argc < 4) {
      bb_simple_error_msg_and_die("create requires at least one source path");
    }
    return create_archive(argv[2], argc - 3, argv + 3);
  }
  if (argc != 3 || (strcmp(options, "tf") && strcmp(options, "xf"))) {
    bb_simple_error_msg_and_die("usage: tar {tf|xf} ARCHIVE | tar cf ARCHIVE PATH...");
  }
  preflight_archive(argv[2]);
  if (!strcmp(options, "tf")) {
    for (struct tar_entry *entry = entries; entry; entry = entry->next) {
      if (printf("%s%s\n", entry->member, S_ISDIR(entry->status.st_mode) ? "/" : "") < 0) {
        path_error("listing output");
      }
    }
    if (fflush(stdout) != 0 || ferror(stdout)) {
      path_error("listing output");
    }
  } else {
    extract_archive();
  }
  return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
  return tar_main(argc, argv);
}
