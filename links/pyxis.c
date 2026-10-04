/* Links platform layer for Pyxis, modelled on dos.c: one process, virtual
 * pipes between Links' internal "threads", terminal input through libterm,
 * and every page loaded through libc.
 *
 * Part of the Pyxis port of Links; distributed under the GPL like Links
 * (see COPYING in the Links source). */
#include "links.h"
#include "pyxis_console.h"

#define VIRTUAL_PIPE_SIZE	512

/* Virtual pipe ends are numbered from the upper half of the descriptor range.
 * Libc numbers its descriptors from zero, lowest free first, and Links never
 * holds hundreds of files open, so the two ranges do not meet. */
#define PIPE_NUMBER_BASE	(FD_SETSIZE / 2)

/* Bytes taken from the console by a wait, kept for the next read. */
#define INPUT_CAPACITY		64

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

static unsigned char input[INPUT_CAPACITY];
static size_t input_start;
static size_t input_count;
static int input_ended;
static int input_error;

/* Returns 0 if the wait timed out, 1 once there is input, EOF or an error. */
static int fill_input(long timeout_ms)
{
	long r = pyxis_console_read(input, sizeof input, timeout_ms);
	if (r == -1)
		return 0;
	if (r == -2) {
		input_error = errno;
	} else if (r == 0) {
		input_ended = 1;
	} else {
		input_start = 0;
		input_count = (size_t)r;
	}
	return 1;
}

static int input_ready(void)
{
	return input_count || input_ended || input_error;
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
	if (fd == 0 && console_available) {
		if (!input_ready())
			fill_input(-1);
		if (input_count) {
			if (size > input_count)
				size = input_count;
			memcpy(buf, input + input_start, size);
			input_start += size;
			input_count -= size;
			return (int)size;
		}
		if (input_error) {
			errno = input_error;
			return -1;
		}
		return 0;
	}
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

static long timeout_ms(struct timeval *t)
{
	if (t->tv_sec < 0 || (t->tv_sec == 0 && t->tv_usec <= 0))
		return 0;
	if (t->tv_sec >= LONG_MAX / 1000 - 1)
		return LONG_MAX;
	return t->tv_sec * 1000 + (t->tv_usec + 999) / 1000;
}

/* Only virtual pipes and console input can be waited on. Other descriptors
 * are libc files, which never block, so they are always reported ready. */
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
			else if (i == 0 && console_available)
				ready = input_ready();
			else
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
			if (fill_input(t ? timeout_ms(t) : -1))
				FD_SET(0, &ready_rs), count++;
		} else {
			if (!t)
				internal_error("pyxis_select: waiting for nothing without a timeout");
			if (!pyxis_sleep_ms(timeout_ms(t)))
				fatal_exit("Pyxis clock cannot sleep");
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

/* Read a provider resource to its end. The provider gives no media type, so
 * a page that starts like HTML is marked as HTML; anything else is typed by
 * its URL's extension, as for a local file. */
static void provider_func(struct connection *c)
{
	struct cache_entry *e;
	FILE *f;
	unsigned char *data;
	int len = 0, capacity = 4096, r;
	unsigned char *head;

	f = fopen(cast_const_char c->url, "r");
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

	head = stracpy(cast_uchar(looks_like_html(data, len) ? "\r\nContent-Type: text/html\r\n" : ""));
	if (!c->cache) {
		if (get_connection_cache_entry(c)) {
			mem_free(data);
			mem_free(head);
			setcstate(c, S_OUT_OF_MEM);
			abort_connection(c);
			return;
		}
		c->cache->refcount--;
	}
	e = c->cache;
	if (e->head) mem_free(e->head);
	e->head = head;
	if ((r = add_fragment(e, 0, data, len)) < 0) {
		mem_free(data);
		setcstate(c, r);
		abort_connection(c);
		return;
	}
	truncate_entry(e, len, 1);
	mem_free(data);
	c->cache->incomplete = 0;
	setcstate(c, S__OK);
	abort_connection(c);
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
