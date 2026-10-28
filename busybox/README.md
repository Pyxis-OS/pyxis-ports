# BusyBox vi, less, tar and printf

[Upstream BusyBox](https://git.busybox.net/busybox) is pinned to
`f96d33d28a1f70fda5f27d221d5012b1ac0b7dad` (1.39.0.git) under GPL-2.0-only.
Upstream's server refuses commits that no branch points at, and the
[GitHub mirror](https://github.com/mirror/busybox) stopped in July 2024, so
the recipe fetches the commit from the internal mirror of upstream.
The recipe selects vi, less, uncompressed tar and printf separately. It stages
`bin/vi.pxe`, `bin/less.pxe`, `bin/tar.pxe` and `bin/printf.pxe`, with
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

Keyboard input uses the upstream read_key decoder and libc poll on an adopted
named input descriptor, separate from stdin. A narrow no-signals branch selects
ordinary poll; no signal mask or handler is supplied. The descriptor owns the
Ctrl+C raw hold through libc's existing native terminal API.

## vi build

The recipe builds selected upstream applet/libbb sources without Kconfig or the
multicall dispatcher:

- `vi_config.h` selects colon commands, yank/marks, BRE search, dot repeat,
  read-only mode, `:set` options, undo with its queue, verbose status and
  screen-size queries. Signals, the cursor-position size probe, `:!` and 8-bit
  display stay off: signal delivery, a POSIX shell interface and a non-ASCII
  renderer are unavailable to these applets.
- `libbb.h` declares the libbb helpers vi references.
- `busybox_pyxis.c` supplies the standalone entry and console adapter. Option,
  list, numeric, string and vector helpers use their upstream sources.

## Patch 0001

The patch preserves native console ownership and replaces Unix permission checks:

- **Raw mode.** The adopted keyboard descriptor holds Ctrl+C passthrough
  through libc's native terminal API. Console input is already unechoed.
  Backspace accepts BS and DEL rather than the termios erase character.
- **Input.** Upstream read_key decodes bytes and escape sequences, using its
  50 ms continuation waits and vi's own input buffer. Upstream mysleep polls
  the keyboard descriptor without consuming or predecoding a key. The narrow
  signal-free helper uses libc poll; signals and job control stay disabled.
- **Output.** Screen output is buffered and written to the output console
  before vi waits for input.
- **Loading.** Upstream `fstat` validates a regular file and sizes its read;
  the current SDK supplies its type and size.
- **Read-only marker.** Upstream `access(W_OK)` queries native WRITE authority;
  there are no Unix mode bits to inspect.
- **`:w NAME`.** The upstream `stat` check refuses an existing different file;
  `:w!` overrides.
- **`~/.exrc`.** It is not read, because its safety check needs Unix owners
  and modes. `EXINIT` still supplies startup commands.
- **Regex.** GNU `re_compile_pattern`/`re_search` and `REG_STARTEND` are replaced
  with libc `regcomp`/`regexec` over bounded, NUL-terminated copies. Forward
  search chooses the first match; backward search retains the last eligible
  match, including overlaps. Substitution captures remain byte offsets into
  the original line. Compile errors use `regerror`, preserve the cursor and
  never free an unsuccessfully compiled pattern. Substitution progression
  otherwise follows upstream.

## Patch 0002

Upstream never clears the per-file read-only bit. After one read-only file,
every later file opened with `:n` or `:e` also showed `[Readonly]` and refused
`:w`. Loading a buffer now clears that bit before the new file is checked.
`-R` uses a separate mode bit and still applies to every file.

## Alternate screen

vi retains upstream's `CSI ? 1049 h/l` alternate-screen sequences. less retains
upstream's clear-screen and status-line cleanup behavior.

## Saves

vi retains upstream's in-place `file_write`: open with `O_WRONLY | O_CREAT`,
write the selected bytes, then `ftruncate` to the number written. Existing
files need their native WRITE authority; no temporary-file CREATE/REMOVE or
rename authority is added. New files need parent CREATE. There is no atomic
replacement or rollback: a short write can leave partial contents. `home://`
persists on installed systems and is RAM on live boots.

## Limits

- **Display:** ASCII only. Control characters display as `^X`, and bytes
  above 127 display as `.`.
- **Screen size:** re-read at each redraw. There is no resize notification.
- **Search:** BRE through libc, with ASCII-only classes/folding and the pinned
  TRE back-reference limits. Owner decision, 2026-10-07: bounded copies stop
  regex matching at an embedded NUL within the slice; no GNU libc API is added.
- **Shell:** there are no shell filters and no `:!`.
- **Input EOF:** ends vi with "can't read user input", as upstream does.
  Unsaved edits are lost.
- **Errors:** allocation failure or a terminal output failure exits with a
  diagnostic, and unsaved edits are lost.

## less

`less_config.h` selects BRE search/highlighting, line/page movement, numbered
positions, multiple files, `-I` ASCII case folding, `-N` line numbers and
`-m`/`-M` status. Patch `0003-less-native-console.patch` adapts upstream
`miscutils/less.c`: keys use upstream's decoder over an adopted named-console
descriptor, drawing keeps the same named output adapter as vi, and content uses
a separate libc descriptor. It normalizes Enter as CR or LF,
retains upstream regex matching and file-change state. The small highlight-loop
fix checks match status before reading unspecified offsets after a failed match;
this is a generic correction, separate from native console support.
Upstream poll loops and temporary content O_NONBLOCK handle content separately
from keyboard input. Native PIPE values remain ordinary file bytes; backing FILE
I/O may still wait. Output is buffered before each key read. Ctrl+C passthrough stays
held until exit. Marks, bracket matching, raw escapes,
shell commands, signals and automatic resizing are disabled. Upstream `s` log
saving and ignored `-s`/`-X` options are retained. Redirected output and a
console too small for paging use the upstream-style plain-copy fallback.

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
9,999,999-line limit; the upstream limit behaves as end of input.
The screen size is measured at startup. The renderer is ASCII; regex uses libc's
ASCII-only classes/folding and pinned back-reference limits. The added named-input grant does not apply to downstream intermediate stages or
file/pipe stdin with non-console stdout; redirected output uses the plain-copy
fallback without console grants. The first stage's original console-input rule
remains intact, and interactive drawing uses the named output console.

## tar

Patch 0004 retains the upstream ustar header, checksum, padding and creation
algorithms, with a tar-specific libbb subset and native I/O adapter. It builds
the upstream `tar_main`, recursive walker, header reader/writer,
member filters and listing helpers. The selected vi/less/tar applets all use
upstream `libbb/getopt32.c` over libc getopt; the port defines no option-parser
globals. See [the tar notes](TAR.md) for exact
validation, path/metadata behavior and memory/write limits. less and tar use only
the inherited working directory, scheme roots and explicit shell grants.

## printf

Patch 0007 selects upstream `coreutils/printf.c`, escape decoding and numeric
conversion helpers with `printf_support.h`. The standalone entry point and
remaining libbb helpers use libc; no terminal, HTTP or request-control grant is
needed. The shell supplies ordinary stdout and stderr, including redirected
files and pipes. The standalone entry returns the upstream applet result; output finalization
follows the SDK stdio policy without a port-specific diagnostic or status rewrite.

```text
printf '{}'
printf '%s:%d\n' value 7
printf '%b' 'first\nsecond\n' > tmp://lines.txt
```

Upstream string, integer and floating formats, `%b`, escapes, optional `--` and
format reuse are retained. Invalid formats or numeric arguments print a diagnostic
and return failure. The embedded-shell `fcntl(F_GETFL)` check is removed:
this standalone applet uses the normal libc stream lifetime.
