# Kilo

[Upstream](https://github.com/antirez/kilo) is pinned to
`323d93b29bd89a2cb446de90c4ed4fea1764176e` under BSD-2-Clause.
The recipe stages `bin/kilo.pxe` and `share/licenses/kilo/LICENSE` together;
retain the license when distributing the executable.

Run `kilo filename` from the Pyxis shell after packaging the executable in
`app://`. Relative paths use the inherited working directory. The process
needs the usual console input/output, memory and directory startup grants,
plus a `clock` grant with READ authority.

- Ctrl-S saves; Ctrl-Q quits, with repeated confirmation for unsaved edits.
- Ctrl-F searches; arrows choose matches, Enter accepts, Escape cancels.
- Arrows, Home, End, Page Up, Page Down, Backspace and Delete navigate/edit.

`0001` replaces POSIX terminal and descriptor operations with libterm and
Pyxis libc streams. It retains syntax highlighting and search, uses startup
console grants and fixed terminal dimensions, and restores the cursor/style
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

This is an ASCII text editor. It reads LF and CRLF, rejects NUL bytes, and
saves LF with a final newline for each row. Saves use create/truncate/write;
an error can leave a partial file. Read-only files can be viewed but not saved.
Allocation failure exits with a diagnostic and loses unsaved edits. There is
no autosave, resize handling or atomic replacement. The initial `home://`
filesystem remains volatile across boots.

This port requires Pyxis's 64 KiB initial userspace stack; the earlier 4 KiB
stack cannot accommodate file loading and its nested library calls.
