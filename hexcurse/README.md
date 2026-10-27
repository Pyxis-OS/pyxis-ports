# Hexcurse on Pyxis

Hexcurse 1.60.0 over static ncursesw. Build with the current SDK and staged
ncursesw development directory:

```sh
lua build.lua hexcurse --sdk /path/to/sdk --ncursesw /path/to/ncursesw/stage/dev
```

Install the `hexcurse` package, then run `hexcurse home://file.bin`. The bundle
declares MEMORY MANAGE and precise clock read/sleep. Streams, cwd and exported
file roots use the existing bundle launch contract.

Ctrl+P opens help, Ctrl+O opens a file, Ctrl+S saves, Ctrl+Q quits, Tab switches
hex/ASCII, and Ctrl+Z undoes. Ctrl+F searches and Ctrl+G goes to an offset. These
existing control shortcuts label the UI because native function keys are
unavailable. Filename input keeps upstream editing behavior; names can also be
supplied in argv. Raw input owns PASSTHROUGH, so Ctrl+C is byte 3.

Native KEY_RESIZE rebuilds the view while retaining edits. A pane below 70
columns or seven content rows pauses until enlarged. Dialogs cancel and requeue
resize for the main loop. Native SCREEN ownership is released on exit.

Input must be seekable. Saving uses upstream's `r+` overwrite and `w+`
Save As logic through libc. There is no port-specific identity check,
exclusive-create policy or transaction/error rewrite. Upstream's alias and
partial-write limitations remain; an alias used for Save As can truncate the
input before copying it.

Open, filename editing, dirty/undo behavior, allocation and other generic
editor behavior follow upstream. Resize comes through the current native
KEY_RESIZE path; moving window-size change detection into ncurses and closing
the terminal API gaps are parked library-growth decisions.

The release archive is checksum-pinned through raw-github; tag 1.60.0 resolves
to `3ce605db4276310297f1cf871a38365650099f5d`. Source is GPL-2.0-or-later. The
bundle retains patched source, both upstream licence texts, ncursesw notices
and provenance. The package also retains the recipe, configuration and patch.
