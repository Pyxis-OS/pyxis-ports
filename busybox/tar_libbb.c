/* Selected BusyBox libbb definitions, copied unchanged from the pinned
 * libbb/xfuncs_printf.c and libbb/read_printf.c. The full translation units
 * include unrelated mmap/socket/process operations absent from this profile.
 * Copyright (C) 1999-2004 Erik Andersen
 * Copyright (C) 2006 Rob Landley, Denys Vlasenko
 * SPDX-License-Identifier: GPL-2.0-only */
#include "tar_support.h"

void FAST_FUNC bb_die_memory_exhausted(void)
{
	bb_simple_error_msg_and_die(bb_msg_memory_exhausted);
}

void* FAST_FUNC xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL && size != 0)
		bb_die_memory_exhausted();
	return ptr;
}

void* FAST_FUNC xzalloc(size_t size)
{
	void *ptr = xmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}

char* FAST_FUNC xstrdup(const char *s)
{
	char *t;

	if (s == NULL)
		return NULL;

	t = strdup(s);

	if (t == NULL)
		bb_die_memory_exhausted();

	return t;
}

char* FAST_FUNC xstrndup(const char *s, size_t n)
{
	char *t;

	t = strndup(s, n);

	if (t == NULL)
		bb_die_memory_exhausted();

	return t;
}

int FAST_FUNC xopen3(const char *pathname, int flags, int mode)
{
	int ret;

	ret = open(pathname, flags, mode);
	if (ret < 0) {
		bb_perror_msg_and_die("can't open '%s'", pathname);
	}
	return ret;
}

int FAST_FUNC xopen(const char *pathname, int flags)
{
	return xopen3(pathname, flags, 0666);
}

int FAST_FUNC open3_or_warn(const char *pathname, int flags, int mode)
{
	int ret;

	ret = open(pathname, flags, mode);
	if (ret < 0) {
		bb_perror_msg("can't open '%s'", pathname);
	}
	return ret;
}

int FAST_FUNC open_or_warn(const char *pathname, int flags)
{
	return open3_or_warn(pathname, flags, 0666);
}

void FAST_FUNC xwrite(int fd, const void *buf, size_t count)
{
	if (count) {
		ssize_t size = full_write(fd, buf, count);
		if ((size_t)size != count) {
			/*
			 * Two cases: write error immediately;
			 * or some writes succeeded, then we hit an error.
			 * In either case, errno is set.
			 */
			bb_simple_perror_msg_and_die(
				size >= 0 ? "short write" : "write error"
			);
		}
	}
}

char* FAST_FUNC xasprintf(const char *format, ...)
{
	va_list p;
	int r;
	char *string_ptr;

	va_start(p, format);
	r = vasprintf(&string_ptr, format, p);
	va_end(p);

	if (r < 0)
		bb_die_memory_exhausted();
	return string_ptr;
}

int FAST_FUNC chdir_or_warn(const char *path)
{
	int r = chdir(path);
	if (r != 0)
		bb_perror_msg("can't change directory to '%s'", path);
	return r;
}

void FAST_FUNC xchdir(const char *path)
{
	if (chdir_or_warn(path) != 0)
		xfunc_die();
}

void FAST_FUNC xfstat(int fd, struct stat *stat_buf, const char *errmsg)
{
	/* errmsg is usually a file name, but not always:
	 * xfstat may be called in a spot where file name is no longer
	 * available, and caller may give e.g. "can't stat input file" string.
	 */
	if (fstat(fd, stat_buf))
		bb_simple_perror_msg_and_die(errmsg);
}

void FAST_FUNC xread(int fd, void *buf, size_t count)
{
	if (count) {
		ssize_t size = full_read(fd, buf, count);
		if ((size_t)size != count)
			bb_simple_error_msg_and_die("short read");
	}
}

int FAST_FUNC fflush_all(void)
{
	return fflush(NULL);
}
