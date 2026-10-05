# BusyBox vi

[Upstream BusyBox](https://git.busybox.net/busybox) is pinned to
`f96d33d28a1f70fda5f27d221d5012b1ac0b7dad` (1.39.0.git) under GPL-2.0-only.
The recipe fetches that commit from the
[GitHub mirror](https://github.com/mirror/busybox), because upstream's server
refuses commits that no branch points at.
The recipe builds only `editors/vi.c` and stages `bin/vi.pxe` with
`share/licenses/busybox/LICENSE`. Retain the license when distributing the
executable. This recipe, its patch and the adapter are its corresponding source.

```text
vi hello.c
vi -R host://notes.txt
vi -c 'set ts=4' home://a.txt home://b.txt
```

Relative paths use the inherited working directory. vi needs the named `input`
and `output` console grants that the shell gives foreground commands, plus the
usual memory and directory grants. Ctrl+C reaches vi as input for the whole
session, so the shell cannot terminate it and discard unsaved edits.

## Build

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
write and the resize can leave old bytes after the new text. The initial
`home://` filesystem remains volatile across boots.

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
