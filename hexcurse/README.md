# Hexcurse on Pyxis

Hexcurse 1.60.0 over the static ncursesw native terminal profile. Build with the
current SDK and staged ncursesw development directory:

```sh
lua build.lua hexcurse --sdk /path/to/sdk --ncursesw /path/to/ncursesw/stage/dev
```

Install the `hexcurse` package, then run `hexcurse home://file.bin`. It is outside
the boot archive. The bundle declares MEMORY MANAGE and precise clock read/sleep;
streams, cwd and exported file roots use the existing bundle launch contract.
No launcher, network factory or device authority is requested.

- Ctrl+P help, Ctrl+O open, Ctrl+S save, Ctrl+Q quit, Tab hex/ASCII, Ctrl+Z undo.
  Ctrl+F searches, Ctrl+G goes to an offset; arrows and Page Up/Down navigate.
  Control shortcuts replace the unsupported function-key labels. In a mux,
  its prefix keeps its usual ownership; Home/End also work.
- Raw input owns PASSTHROUGH; Ctrl+C is byte 3, not a signal. KEY_RESIZE rebuilds
  the view while retaining edits. A pane below 70 columns or seven content rows
  pauses until enlarged. Dialogs cancel on resize without discarding changes.
- Input must be a seekable file. Save needs known file identity and actual write
  authority. Ordinary Save reopens and verifies the same retained file; aliases
  edit that object in place. Save As copies only to a proven distinct file,
  never truncates an alias first and never falls back to the source on refusal.
- Seek/read/write/flush/sync/close failures are visible and keep pending edits.
  Success follows `fsync`; this has the backing store's native durability.
  Saves are in place, not atomic against faults or concurrent writers: an I/O
  failure can leave partial destination changes. Unknown identities and prepared
  provider transactions refuse; pending transactions are abandoned, not committed.
- Cancelled or failed Open retains the current file, name and edit buffer.
  Successful Open adopts the already validated stream; unsaved edits require
  confirmation. This editor changes existing bytes, not file length.

The release archive is checksum-pinned through raw-github; its tag resolves to
`3ce605db4276310297f1cf871a38365650099f5d`. Source is GPL-2.0-or-later. The bundle
retains its patched source, both upstream licence texts, ncursesw notices and
provenance; the package also retains the recipe/configuration/patches. Curses's
C-locale width and Pyxis's existing terminal repertoire remain unchanged.
