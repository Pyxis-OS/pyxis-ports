# ncursesw on Pyxis

Static ncurses 6.6 with one embedded `TERM=pyxis` entry and a native terminal
backend. Build with the current SDK, then link `pkg-config --cflags --libs
ncursesw`; `ncurses` is an alias. Install `ncursesw-dev` for headers and the
library under `sdk://`. `ncurses-demo` is a separate program package.

The byte input and output FILEs selected by `newterm` must be native consoles.
There is no ambient input fallback. The caller excludes concurrent use or
destruction of a SCREEN and its descriptors. The backend observes input and
output geometry together; it never reads data through borrowed native handles.
Input loss clears partial key assembly and returns ERR; EOF/hangup and output
failures remain errors. Mode snapshots retain storage, not a raw grant.

- `raw()` holds PASSTHROUGH: Ctrl+C is byte 3. `cbreak()` releases this scope's
  full passthrough and keeps ordinary foreground interruption (cooperative first
  press for an opted-in process; forceful second; non-opted processes stop on
  the first). Other holders' passthrough still wins. No Unix signals.
- Echo, line editing, timed and nonblocking input remain curses logic. No kernel
  cooked mode, hardware speed, signal queue flushing, flow control or tty ioctls.
- SIZE/RESIZED drives KEY_RESIZE without SIGWINCH. Partial Escape input retains
  its original ambiguity deadline across resize, without consuming a key.
- Explicit input flush drains currently available bytes until native EAGAIN,
  without an added byte-budget policy.
- Native try-write plus readiness handles backpressure. Uncertain/failed writes
  latch an error; updates and endwin report it rather than claiming a redraw.
- Only the embedded `pyxis` terminfo entry is supported. No TERMINFO/HOME lookup,
  queries, mouse protocol, unsupported xterm controls or Unicode width claim.
  The profile exposes 16 colours and restores default colours through its SGR
  39/49 `op` capability; the native driver accepts that capability for
  `use_default_colors`. SCREEN owns a copied fallback termtype, so destruction
  releases its arrays without freeing the embedded static entry. The existing single-cell repertoire and libc's ASCII C-locale
  printable classification are unchanged.

`ncurses-demo` shows dimensions and keys. `r` switches raw/cbreak, `e` leaves and
re-enters curses, and `q` quits. `--cooperate` opts into first-press notification
and displays key -3 while staying alive for a second-press force stop. In raw
mode Ctrl+C displays key 3. Resize locally or through a mux/Remote session.

The development tree also retains the entry source and compiled form, upstream
COPYING and source provenance. Build tools, menu/form/panel and C++/Ada bindings,
extended colour-pair allocation and Unix signal/terminal backends are not built.
