# Links

[Links](http://links.twibright.com/) 2.30 is pinned to the release archive
`links-2.30.tar.bz2`, SHA-256
`c4631c6b5a11527cdc3cb7872fc23b7f2b25c2b021d596be410dadb40315f166`. Links has
no public Git history, so the archive checksum is the pin. Each source file says
it is "released under GPL" without naming a version, and `COPYING` contains GPL
version 2. The recipe stages `bin/links.pxe` with
`share/licenses/links/COPYING`. Retain that license when distributing the
executable. This recipe, its patches and the files beside them are its
corresponding source, under the same terms as Links.

```text
links host://docs/index.html
links index.html
links https://example.com/
links -dump host://notes.html
```

Links needs the named `input` and `output` console grants that the shell gives
foreground commands, the named `clock`, and the usual memory and directory
grants. It runs in text mode on the framebuffer console and through the remote
terminal.

## Build

Configure cannot run against the SDK, so the recipe supplies:

- `config.h`, listing only what Pyxis libc provides. Everything else takes
  Links' own fallback or the Pyxis platform block.
- `include/`, declarations for Unix interfaces that Links compiles but this
  port never reaches: socket and name-lookup types, signal numbers and
  `fd_set`.
- `network.c`, where every socket call fails with ENOTSUP and
  `gethostbyname` finds nothing.
- `pyxis.c`, the platform layer, modelled on Links' `dos.c`.
- `pyxis_console.c`, its libterm and clock side. It is a separate file because
  Links and libterm both define `struct terminal`.

The graphics, SSL and compression code is not configured. Those files compile
to empty units.

## Patches

- **0001 Pyxis platform.**
  - **Platform block.** A `PYXIS` block in `os_dep.h`/`os_depx.h` is keyed on
    `__pyxis__`. Like DOS, it has no fork, signals, asynchronous DNS, SMB,
    shell or file-security checks.
  - **Redirected calls.** `read`, `write`, `pipe`, `close` and `select` go to
    `pyxis.c`.
  - **Nonblocking pipes.** Virtual pipes take descriptor numbers from
    `pyxis.c` instead of opening `/dev/null`, and are nonblocking as on DOS.
  - **No other programs.** `exe` fails, since Links starts no other programs.
    There is no textual working directory, so relative `file://` URLs stay
    relative.
  - **Exit sequence.** The exit sequence homes the cursor, because the Pyxis
    terminal has no saved cursor. Mouse-mode sequences are not sent.
- **0002 Narrow file metadata.** Libc's `struct stat` has only a type and a
  size. Directory listings leave the date column blank, and the bookmark file
  change check compares sizes only.
- **0003 Load pages through libc.**
  - **Protocol table.** `http` and `https`, and any `scheme://` missing from
    the table, go to `pyxis_func`. Unknown schemes are hierarchical with no
    host part, so relative links resolve against the path.
  - **Native roots.** A startup root such as `host://` is a native directory
    tree. It loads through `file.c` with libc `stat`, `open` and
    `opendir`/`readdir`, including Links' directory listings.
  - **Providers.** Any other scheme is a namespace provider, read to its end
    with `fopen`. Response metadata supplies Content-Type when present. Without
    it, a page that starts like HTML (`<!doctype html`, `<html`, `<head`,
    `<body`, `<title` or `<!--`) is marked as HTML. Anything else is typed by
    its extension, as for local files.
- **0004 Save configuration under `home://links/`.** Startup finds or makes
  that directory and loads `links.cfg`, `html.cfg` and `user.cfg` from it.
  Options, HTML options, bookmarks, cookies and the URL history are saved there.
  Links' own save (an exclusive temporary file, `fsync`, then `rename`) replaces
  the old file atomically; the patch only creates the temporary with mode 0666,
  the one mode Pyxis creation accepts. If the directory cannot be found or made,
  nothing is loaded and saving reports "Home directory inaccessible"; any other
  failure reports the error of the failing call. There is no directory sync
  (libc cannot open a directory), so after a crash the new name may be lost
  and the old file remains.
- **0005 Adopt provider redirect snapshots.** Libc follows HTTP redirects before
  returning the FILE. Links copies its response metadata before closing the
  stream and uses the final URL for the document base, relative links, navigation
  history and downloads. A final fragment selects the document position, taking
  precedence over an initial fragment. The final body has its own cache entry;
  requested URLs alias that snapshot without changing an inline cache key or
  fetching it again. Aliases do not keep bodies alive indefinitely: after the
  body is evicted, opening a requested URL fetches a new snapshot. Native files
  and inherited streams have no response metadata and retain their behavior.
- **Downloads.** Patch 0001 no longer defines an unsupported `O_EXCL`: libc
  now has exclusive creation, so Links' download path works. The save dialog
  takes any path the program's roots allow, relative to the inherited working
  directory. An existing file offers Continue, Overwrite, Rename or Cancel.
  Overwrite truncates in place and a download is written under its final
  name, so neither is atomic.

## Event loop

Links' internal threads talk through virtual pipes in one process, as on DOS.

- **Waiting.** `pyxis_select` returns ready pipes at once. Otherwise it waits
  for console input with the next timer as its timeout, and keeps the bytes it
  read for the next `read`. Without terminal input to wait for, it sleeps on
  the clock until the timeout.
- **Ctrl+C.** Raw mode holds libterm Ctrl+C passthrough. Ctrl+C reaches Links
  as a key, and Links quits on it as it does upstream.
- **Screen size.** The size is read when the terminal starts. There is no
  resize notification.

## Limits

- **Loading blocks.** Every load blocks the interface, network fetches
  included. Only the HTTP provider's own deadlines bound them.
- **HTTP.**
  - The HTTP provider follows bounded redirects and accepts a final 200/204.
    Other final statuses and rejected redirect chains remain open errors.
  - GET forms work as URLs with a query string. There is no POST, and no
    cookies or request headers.
  - Response Content-Type takes precedence over HTML sniffing and URL extensions.
- **Local paths.** Native paths are literal bytes, except that Links decodes
  `%XX` escapes in local URLs as it does for `file://`. Symlinks are listed but
  cannot be opened. Directories show size 0.
- **Other protocols.** `ftp://` and `finger://` need Links' own name lookup
  and sockets. `gethostbyname` always fails, so they report "Host not found".
- **Display.** ASCII only: non-ASCII characters are approximated, and there
  are no images.
