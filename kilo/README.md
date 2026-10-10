# Kilo

[Upstream](https://github.com/antirez/kilo) is pinned to
`323d93b29bd89a2cb446de90c4ed4fea1764176e` under BSD-2-Clause.
The recipe stages `bin/kilo.pxe` and `share/licenses/kilo/LICENSE` together;
retain the license when distributing the executable.

Run `kilo filename` from the Pyxis shell after packaging the executable in
`boot://`. Relative paths use the inherited working directory. The process
needs the usual console input/output, memory and directory startup grants,
plus a `clock` grant with READ authority.

- Ctrl-S saves; Ctrl-Q quits, with repeated confirmation for unsaved edits.
- Ctrl-F searches; arrows choose matches, Enter accepts, Escape cancels.
- Arrows, Home, End, Page Up, Page Down, Backspace and Delete navigate/edit.

`0001` replaces POSIX terminal and descriptor operations with libterm and
Pyxis libc streams. It retains syntax highlighting and search, uses startup
console grants and native terminal dimensions, and restores the cursor/style
on normal exit or reported errors.

`0002` checks allocation failures and size arithmetic, bounds syntax scanning,
replaces recursive comment propagation so stack use does not grow with the
number of lines in a file. It corrects row indices,
deletion updates, tab/cursor/search alignment and margin rendering. Upstream
formatting and editor structure are retained.

`0003` uses the native monotonic clock to expire ordinary status messages after
five seconds, including while idle. It waits for input with libterm's timed
key-read helper until the deadline, redraws on expiry, then blocks indefinitely
again. Escape-sequence decoding can finish after that deadline. Search prompts
stay visible until the search ends. No calendar clock or polling loop is needed.

`0004` handles terminal input EOF explicitly, including inside search prompts.
It exits cleanly when there are no unsaved edits; otherwise it reports their loss
and exits unsuccessfully. It never treats EOF as a key or retries it indefinitely.
Ctrl-D remains an ordinary input byte, not terminal closure.

`0005` bounds Page Up/Down navigation at the EOF row, including when a file is
shorter than the screen. Left-arrow row lookup also checks this bound before
reading the previous row.

`0006` requests Ctrl+C passthrough on the editor's input for the whole session.
An armed Pyxis shell would otherwise terminate a foreground Kilo on Ctrl+C and
discard unsaved edits. Kilo's own Ctrl+C handling is unchanged: it ignores the
key. The request is made before any file is loaded. If it fails, Kilo exits
with a diagnostic rather than edit unprotected, and process exit withdraws it.
While Kilo runs, Ctrl+C cannot end it; use Ctrl-Q. `term_passthrough` comes
from libterm, so the editor needs the matching SDK.

`0007` waits for typed libterm resize events while idle and in search prompts,
re-queries the viewport and redraws without a keypress. File text, logical cursor,
search text/highlighting and the original five-second message deadline survive.
Partial Escape/CSI sequences retain their decoder state and byte deadline across
resize redraws. The existing minimum of two columns and three rows still applies;
smaller dimensions report an error rather than continuing with an invalid layout.

`0008` edits on the alternate screen through libterm's `term_alternate_screen`,
instead of clearing the screen at start and exit, so quitting returns to the
shell's screen and cursor as they were. It needs the matching SDK.

This is an ASCII text editor. It reads LF and CRLF, rejects NUL bytes, and
saves LF with a final newline for each row. Saves use create/truncate/write;
an error can leave a partial file. Read-only files can be viewed but not saved.
Allocation failure exits with a diagnostic and loses unsaved edits. There is
no autosave or atomic replacement. `home://` persists on
installed systems and is RAM on live boots.

Pyxis supplies a guarded 1 MiB initial userspace stack. The earlier 4 KiB
stack cannot accommodate file loading and its nested library calls.
