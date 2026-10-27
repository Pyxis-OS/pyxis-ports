# Kilo

[Upstream](https://github.com/antirez/kilo) is pinned to
`323d93b29bd89a2cb446de90c4ed4fea1764176e` under BSD-2-Clause.
The recipe stages `bin/kilo.pxe` and `share/licenses/kilo/LICENSE` together;
retain the license when distributing the executable.

```sh
lua build.lua kilo --sdk /path/to/build/sdk
```

Run `kilo filename` from the Pyxis shell. Relative paths use the inherited
working directory. The process needs console input/output, memory and directory
startup grants, plus a clock with precise READ authority. Its inherited stdin
and stdout must be consoles; there is no named-console fallback for redirected
streams.

Ctrl-S saves, Ctrl-Q quits with repeated confirmation for unsaved edits, and
Ctrl-F searches. Arrow keys, Page Up/Down and Delete use upstream navigation.
Pinned upstream decodes Home/End but does not dispatch those soft keys; End was
observed inserting byte 0xee instead of moving the cursor. This upstream defect
is unchanged. The port preserves upstream byte insertion,
syntax highlighting, line reading and descriptor saving. Creation requests use
native mode 0666; authority comes from the supplied filesystem grants.

The single patch keeps upstream termios raw setup and restoration, including
TCSAFLUSH, the byte escape parser and ioctl window sizing. The SDK supplies the
raw byte profile and its 100 ms read timeout. A small native wait hook handles
resize alongside input while SIGWINCH remains unavailable; it keeps the same
Escape timeout budget across resize. Pyxis Enter's LF byte is mapped to CR for
the upstream editor. The port preserves logical file coordinates across resize
and refuses a viewport below two columns or three rows. The margin adjustment
avoids erasing the last printed cell under Pyxis terminal semantics.

Status messages use upstream expiry on the next keypress. Ctrl+C stays editor
input and is ignored as upstream does; use Ctrl-Q to exit. Terminal EOF exits
cleanly if unmodified and reports lost edits otherwise. Ordinary input errors
follow upstream failure behavior; Ctrl-D remains an ordinary byte. The editor
owns an alternate screen, and normal exit restores raw state, the shell screen,
cursor and style.

Saves truncate and write the existing object; failures can leave partial
contents. There is no atomic replacement or autosave. `home://` persists on
installed systems and is RAM on live boots. Generic allocation, syntax, tab,
cursor, search and paging fixes are left to upstream. The guarded 1 MiB initial
userspace stack is supplied by the current SDK.
