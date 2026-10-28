/* Links platform layer for Pyxis, modelled on dos.c: one process, virtual
 * pipes between Links' internal "threads", terminal input through libc,
 * and every page loaded through libc.
 *
 * Part of the Pyxis port of Links; distributed under the GPL like Links
 * (see COPYING in the Links source). */
#include "links.h"
#include "pyxis_console.h"
#include <http_navigation.h>
#include <pyxis/stdio.h>
#include <poll.h>

#define VIRTUAL_PIPE_SIZE	512

/* Virtual pipe ends are numbered from the upper half of the descriptor range.
 * Libc numbers its descriptors from zero, lowest free first, and Links never
 * holds hundreds of files open, so the two ranges do not meet. */
#define PIPE_NUMBER_BASE	(FD_SETSIZE / 2)

#undef read
#undef write
#undef pipe
#undef close
#undef select

static unsigned char pipe_number_used[FD_SETSIZE - PIPE_NUMBER_BASE];

static int pyxis_pipe_number(void)
{
	int i;
	for (i = 0; i < FD_SETSIZE - PIPE_NUMBER_BASE; i++) {
		if (!pipe_number_used[i]) {
			pipe_number_used[i] = 1;
			return PIPE_NUMBER_BASE + i;
		}
	}
	errno = EMFILE;
	return -1;
}

static void pyxis_pipe_release(int fd)
{
	pipe_number_used[fd - PIPE_NUMBER_BASE] = 0;
}

static inline void pipe_lock(void)
{
}

static inline void pipe_unlock(void)
{
}

static inline void pipe_unlock_wait(void)
{
}

static inline void pipe_wake(void)
{
}

#include "vpipe.inc"

/* Descriptors 0 and 1 are the named console grants when the shell gave them;
 * otherwise they stay libc's standard streams (for example with -dump). */
static int console_available;

/* Observe the owned keyboard descriptor without consuming input. */
static int input_ready(struct timeval *timeout)
{
	struct pollfd input = { .fd = pyxis_console_input_fd(), .events = POLLIN };
	struct timespec span;
	if (timeout) {
		span.tv_sec = timeout->tv_sec;
		span.tv_nsec = timeout->tv_usec * 1000;
	}
	return ppoll(&input, 1, timeout ? &span : NULL, NULL);
}

int pyxis_pipe(int fd[2])
{
	return vpipe_create(fd);
}

int pyxis_read(int fd, void *buf, size_t size)
{
	int r = vpipe_read(fd, buf, size);
	if (r != -2)
		return r;
	if (fd == 0 && console_available)
		fd = pyxis_console_input_fd();
	return (int)read(fd, buf, size);
}

int pyxis_write(int fd, const void *buf, size_t size)
{
	int r = vpipe_write(fd, buf, size);
	if (r != -2)
		return r;
	if (fd == 1 && console_available) {
		if (!pyxis_console_write(buf, size))
			return -1;
		return (int)size;
	}
	return (int)write(fd, buf, size);
}

int pyxis_close(int fd)
{
	int r = vpipe_close(fd);
	if (r != -2)
		return r;
	return close(fd);
}

/* Virtual pipes and console input are the waiting sources in this profile.
 * Ordinary FILE descriptors are always ready; their backing I/O can still wait. */
int pyxis_select(int n, fd_set *rs, fd_set *ws, fd_set *es, struct timeval *t)
{
	fd_set ready_rs, ready_ws;
	int count = 0;
	int i;
	FD_ZERO(&ready_rs);
	FD_ZERO(&ready_ws);
	for (i = 0; i < n; i++) {
		if (rs && FD_ISSET(i, rs)) {
			int ready;
			if (pipe_desc[i])
				ready = vpipe_may_read(i);
			else if (i == 0 && console_available) {
				struct timeval zero = {0};
				ready = input_ready(&zero);
				if (ready < 0)
					return -1;
			} else
				ready = 1;
			if (ready)
				FD_SET(i, &ready_rs), count++;
		}
		if (ws && FD_ISSET(i, ws)) {
			if (!pipe_desc[i] || vpipe_may_write(i))
				FD_SET(i, &ready_ws), count++;
		}
	}
	/* Nothing else runs while Links waits, so no pipe can become ready:
	 * only console input or the timeout can end the wait. */
	if (!count) {
		if (rs && n > 0 && FD_ISSET(0, rs) && console_available) {
			int ready = input_ready(t);
			if (ready < 0)
				return -1;
			if (ready)
				FD_SET(0, &ready_rs), count++;
		} else {
			if (!t)
				internal_error("pyxis_select: waiting for nothing without a timeout");
			if (select(0, NULL, NULL, NULL, t) < 0)
				return -1;
		}
	}
	if (rs)
		*rs = ready_rs;
	if (ws)
		*ws = ready_ws;
	if (es)
		FD_ZERO(es);
	return count;
}

/* Console input is already raw and unechoed. Raw mode holds Ctrl+C so it
 * reaches Links as a key instead of ending the program. */
int setraw(int ctl, int save)
{
	pyxis_console_hold_ctrl_c(1);
	return 0;
}

void setcooked(int ctl)
{
	pyxis_console_hold_ctrl_c(0);
}

void get_terminal_size(int *x, int *y)
{
	if (!pyxis_console_size(x, y)) {
		*x = 80;
		*y = 24;
	}
}

/* There is no resize notification; the size is read once per terminal. */
void handle_terminal_resize(void (*fn)(int, int), int *x, int *y)
{
	get_terminal_size(x, y);
}

void unhandle_terminal_resize(void)
{
}

uttime get_time(void)
{
	uint64_t ms;
	if (!pyxis_clock_ms(&ms))
		fatal_exit("Pyxis clock unavailable");
	return (uttime)ms;
}

void init_os(void)
{
	console_available = pyxis_console_open();
}

void terminate_osdep(void)
{
	pyxis_console_hold_ctrl_c(0);
}

static int looks_like_html(unsigned char *data, int len)
{
	static_const char * const starts[] = {
		"<!doctype html", "<html", "<head", "<body", "<title", "<!--",
	};
	int i;
	if (len >= 3 && data[0] == 0xef && data[1] == 0xbb && data[2] == 0xbf)
		data += 3, len -= 3;
	while (len && WHITECHAR(*data))
		data++, len--;
	for (i = 0; i < (int)array_elements(starts); i++) {
		int l = (int)strlen(starts[i]);
		if (len >= l && !casecmp(data, cast_uchar starts[i], l))
			return 1;
	}
	return 0;
}

/* The opt-in navigation FILE carries its final URL and media type as a
 * bounded prefix. Cache only the raw body; aliases retain redirect behavior. */
static void provider_func(struct connection *c)
{
	struct cache_entry *e;
	struct http_navigation response;
	FILE *f;
	unsigned char *data;
	int len = 0, capacity = 4096, r;
	unsigned char *head;
	unsigned char *url = NULL, *position;
	int head_len = 0, redirected = 0;

	f = http_navigation_fopen(cast_const_char c->url, &response);
	if (!f) {
		setcstate(c, get_error_from_errno(errno));
		abort_connection(c);
		return;
	}
	data = mem_alloc(capacity);
	while (1) {
		size_t got;
		if (len == capacity) {
			if (capacity > MAXINT / 2) {
				mem_free(data);
				fclose(f);
				setcstate(c, S_LARGE_FILE);
				abort_connection(c);
				return;
			}
			capacity *= 2;
			data = mem_realloc(data, capacity);
		}
		got = fread(data + len, 1, capacity - len, f);
		len += (int)got;
		if (got)
			continue;
		if (ferror(f)) {
			int er = errno;
			mem_free(data);
			fclose(f);
			setcstate(c, get_error_from_errno(er));
			abort_connection(c);
			return;
		}
		break;
	}
	fclose(f);

	head = init_str();
	if (*response.media_type) {
		add_to_str(&head, &head_len, cast_uchar "\r\nContent-Type: ");
		add_to_str(&head, &head_len, cast_uchar response.media_type);
		add_to_str(&head, &head_len, cast_uchar "\r\n");
	} else if (looks_like_html(data, len)) {
		add_to_str(&head, &head_len, cast_uchar "\r\nContent-Type: text/html\r\n");
	}
	if (response.status) {
		url = stracpy(cast_uchar response.effective_url);
		if ((position = extract_position(url))) mem_free(position);
		redirected = strcmp(cast_const_char url, cast_const_char c->url) != 0;
	}
	if (!c->cache) {
		if (get_connection_cache_entry(c)) {
			mem_free(data);
			mem_free(head);
			if (url) mem_free(url);
			setcstate(c, S_OUT_OF_MEM);
			abort_connection(c);
			return;
		}
		c->cache->refcount--;
	}
	e = c->cache;
	if (redirected) {
		if (find_in_cache(url, &e) && new_cache_entry(url, &e)) {
			mem_free(data);
			mem_free(head);
			mem_free(url);
			setcstate(c, S_OUT_OF_MEM);
			abort_connection(c);
			return;
		}
	}
	if (url) mem_free(url);
	if (e->redirect) mem_free(e->redirect), e->redirect = NULL;
	if (e->pyxis_redirect) mem_free(e->pyxis_redirect), e->pyxis_redirect = NULL;
	e->incomplete = 1;
	e->http_code = response.status ? (int)response.status : -1;
	if (e->head) mem_free(e->head);
	e->head = head;
	if ((r = add_fragment(e, 0, data, len)) < 0) {
		mem_free(data);
		setcstate(c, r);
		abort_connection(c);
		if (redirected) e->refcount--;
		return;
	}
	truncate_entry(e, len, 1);
	mem_free(data);
	e->incomplete = 0;
	if (redirected) {
		/* Never change the inline cache key. Keep the body pinned while the
		 * completion callbacks adopt it, without pinning it for alias life. */
		struct cache_entry *alias = c->cache;
		delete_entry_content(alias);
		if (alias->head) mem_free(alias->head);
		alias->head = stracpy(cast_uchar "");
		if (alias->redirect) mem_free(alias->redirect), alias->redirect = NULL;
		if (alias->pyxis_redirect) mem_free(alias->pyxis_redirect);
		alias->pyxis_redirect = stracpy(cast_uchar response.effective_url);
		alias->http_code = e->http_code;
		alias->incomplete = 0;
		finish_cache_entry(e);
	}
	setcstate(c, S__OK);
	abort_connection(c);
	if (redirected) e->refcount--;
}

/* http://, https:// and every scheme Links does not know. A startup root such
 * as host:// is a native directory tree, loaded by file.c with its directory
 * listings; any other scheme reaches a namespace provider. Loads block. */
void pyxis_func(struct connection *c)
{
	unsigned char *scheme = get_protocol_name(c->url);
	int root = scheme && pyxis_scheme_is_root(cast_const_char scheme);
	if (scheme)
		mem_free(scheme);
	if (root)
		file_func(c);
	else
		provider_func(c);
}
