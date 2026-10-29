# CMatrix on Pyxis

CMatrix 2.0 over static ncursesw. Build with an exported SDK and the staged
ncursesw development prefix:

```sh
lua build.lua cmatrix --sdk /path/to/sdk --ncursesw /path/to/ncursesw/stage/dev
```

Install `cmatrix`, then run `cmatrix` with the shell's `TERM=pyxis`.
Press `q` to quit, `p` to pause, `a` to toggle asynchronous scrolling,
`b`/`B`/`n` to select bold, `0` through `9` to change delay, and `r` to enable
rainbow colours. `-s` exits on the first key; `-h` lists upstream options.
There are no save files. The bundle requests memory and precise clock read/sleep.

The patch includes the standard strings header, excludes the unused optional
Linux font-loading helper, and queries the supplied stdin console directly with
`ioctl(TIOCGWINSZ)` on Pyxis instead of reopening a Unix tty pathname. Upstream
SIGWINCH handling, animation, options and exit behavior remain unchanged. SIGINT
uses libc's cooperative terminal delivery. SIGQUIT and SIGTSTP are unavailable.

Only `TERM=pyxis` is supported by this ncursesw build. The Linux font-loading
mode (`-l`) reports its upstream missing-helper error; `-f` selects an unsupported
terminal type. Font-dependent Japanese/X font options have no qualification.
Upstream clamps resize dimensions to at least 10 rows and columns, so keep the
terminal at least that large.

The owner-cache v2.0 tag archive is checksum-pinned; its upstream revision is
`adfdf1656f23e5ab3b52c7d7edf91249a4477e8d`. Upstream sources are GPL-3.0-or-later.
The bundle retains patched source, upstream licensing and ncursesw notices;
packaging also retains the recipe and patch. The target build passed with the
current SDK. Interactive Pyxis qualification is separate.
