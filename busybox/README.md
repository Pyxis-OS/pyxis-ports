# BusyBox vi, less and tar

[Upstream BusyBox](https://git.busybox.net/busybox) is pinned to
`f96d33d28a1f70fda5f27d221d5012b1ac0b7dad` (1.39.0.git) under GPL-2.0-only.
The recipe fetches that commit from the
[GitHub mirror](https://github.com/mirror/busybox), because upstream's server
refuses commits that no branch points at.
The recipe selects vi, less and uncompressed tar separately. It stages
`bin/vi.pxe`, `bin/less.pxe` and `bin/tar.pxe`, with
`share/licenses/busybox/LICENSE`. Retain the license when distributing the
executable. This recipe, its ordered patches and adapters are their corresponding source.

```text
vi hello.c
vi -R host://notes.txt
vi -c 'set ts=4' home://a.txt home://b.txt
```

Relative paths use the inherited working directory. vi needs the named `input`
and `output` console grants that the shell gives foreground commands, plus the
usual memory and directory grants. Ctrl+C reaches vi as input for the whole
session, so the shell cannot terminate it and discard unsaved edits.

## vi build

BusyBox's Kconfig and libbb are not built. The recipe replaces them:

- `vi_config.h` selects colon commands, yank/marks, literal search, dot repeat,
  read-only mode, `:set` options, undo with its queue, verbose status and
  screen-size queries. Regex search, signals, the cursor-position size probe,
  `:!` and 8-bit display stay off: Pyxis has no `regex.h`, signals, shell
  command execution or non-ASCII renderer for them.
- `libbb.h` declares the libbb helpers vi references.
- `busybox_pyxis.c` supplies `main` and those helpers over libc and libterm.

## Patch 0001

The patch replaces termios, poll and stat:

- **Raw mode.** It holds libterm Ctrl+C passthrough instead of switching
  termios. Console input is already raw and unechoed. Backspace accepts BS and
  DEL rather than the termios erase character.
- **Input.** Keys come from libterm's decoder: a standalone Escape after
  100 ms, arrows, Home/End, Delete and Page Up/Down. The input-pending check is a
  timed key read that keeps the key for the next read. Unsupported escape
  sequences are dropped.
- **Output.** Screen output is buffered and written to the output console
  before vi waits for input. The screen is cleared on start and exit, since
  there is no alternate screen.
- **Loading.** Files are read to EOF instead of sized with `fstat`. Opening a
  directory fails with the native error.
- **Read-only marker.** A file is marked `[Readonly]` when it cannot be opened
  for WRITE now. That probe replaces `access` and mode bits.
- **`:w NAME`.** It refuses to replace another file unless opening NAME reports
  that it does not exist; `:w!` overrides.
- **`~/.exrc`.** It is not read, because its safety check needs Unix owners
  and modes. `EXINIT` still supplies startup commands.

## Patch 0002

Upstream never clears the per-file read-only bit. After one read-only file,
every later file opened with `:n` or `:e` also showed `[Readonly]` and refused
`:w`. Loading a buffer now clears that bit before the new file is checked.
`-R` uses a separate mode bit and still applies to every file.

## Saves

A save opens the file without truncation, writes the buffer, then calls libc
`ftruncate` (native RESIZE) to the number of bytes written. This is upstream's
order and is unchanged. A short write reports an error, but the file has
already been overwritten up to that point and truncated there. Upstream also
ignores the `ftruncate` result. Saves are not atomic: a crash between the
write and the resize can leave old bytes after the new text. `home://` persists
on installed systems and is RAM on live boots.

## Limits

- **Display:** ASCII only. Control characters display as `^X`, and bytes
  above 127 display as `.`.
- **Screen size:** re-read at each redraw. There is no resize notification.
- **Search:** `/`, `?` and `:s` are literal, not regex.
- **Shell:** there are no shell filters and no `:!`.
- **Input EOF:** ends vi with "can't read user input", as upstream does.
  Unsaved edits are lost.
- **Errors:** allocation failure or a terminal output failure exits with a
  diagnostic, and unsaved edits are lost.

## less

`less_config.h` selects literal search, line/page movement, numbered positions,
multiple files, `-N` line numbers and `-m`/`-M` status. Patch 0003 adapts upstream
`miscutils/less.c`: keys and drawing use the same libterm console adapter as vi,
while content uses a separate libc descriptor. It normalizes Enter as CR or LF,
uses case-sensitive literal search in place of regex and clears matches when
changing files. Output is buffered before each key read. Ctrl+C passthrough stays
held until exit. Search highlights, marks, bracket matching, raw escapes,
shell commands, log saving, signals and automatic resizing are disabled.

```text
less boot://share/hwdata/pci.ids
ls boot:// | less
less < tmp://notes.txt
```

Space/Page Down and `b`/Page Up move pages; arrows or `j`/`k` move lines;
`g`/`G` or Home/End select the beginning/end. `/`, `?`, `n` and `N` search;
`q` exits. The shell supplies the final foreground stage's named input with
READ alone when stdin is a file/pipe and stdout is a console. No physical
keyboard, pointer or interrupt-arming right accompanies that grant. Both ends
reading the console, such as `cat | less`, compete and are unsupported.

Content reads block when more data is needed to fill a page or find a match.
Cached navigation does not fetch further input. There is no live refresh or
nonblocking pipe readiness; a stalled producer can delay key handling during a
refill. Read lines stay in memory for backward paging, up to the selected
9,999,999-line limit; exceeding it fails instead of silently truncating input.
The screen size is measured at startup. The renderer is ASCII; search is literal
and case-sensitive without highlighting. The added named-input grant does not apply to downstream intermediate stages or
file/pipe stdin with non-console stdout; those pager forms fail explicitly. The
first stage's original console-input rule remains intact, and drawing always uses
the named output console, independently of stdout.

## tar

Patch 0004 retains the upstream ustar header, checksum, padding and creation
algorithms, with a tar-specific libbb subset and libc snapshot adapter. It builds
`archival/tar.c`, `archival/libarchive/get_header_tar.c` and
`archival/chksum_and_xwrite_tar_header.c`. See [the tar notes](TAR.md) for exact
validation, path/metadata behavior and memory/write limits. Both new applets use
only the inherited working directory, scheme roots and explicit shell grants.
