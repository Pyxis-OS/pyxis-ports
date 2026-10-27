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
startup grants, plus a clock with precise READ authority.

Ctrl-S saves, Ctrl-Q quits with repeated confirmation for unsaved edits, and
Ctrl-F searches. Arrow keys and Page Up/Down use upstream navigation. Home/End
and Delete keep upstream bindings. The port preserves upstream byte insertion,
syntax highlighting, line reading and descriptor saving. Creation requests use
native mode 0666; authority comes from the supplied filesystem grants.

The single patch supplies native libterm input/output, viewport size, typed
resize events, Ctrl+C passthrough and alternate-screen ownership. It preserves
the logical cursor and search prompt across resize. A viewport below two columns
or three rows is refused. Decoder state survives resize; native special keys do
not enter byte insertion or ctype calls. The margin adjustment avoids erasing
the last printed cell under Pyxis terminal semantics.

Ordinary status messages expire after five monotonic seconds, including while
idle, using libterm's deadline wait. Search prompts remain visible. Ctrl+C stays
editor input and is ignored as upstream does; use Ctrl-Q to exit. Terminal EOF
exits cleanly if unmodified and reports lost edits otherwise. Ctrl-D remains an
ordinary byte. Exit restores the shell screen, cursor and style.

Saves truncate and write the existing object; failures can leave partial
contents. There is no atomic replacement or autosave. `home://` persists on
installed systems and is RAM on live boots. Generic allocation, syntax, tab,
cursor, search and paging fixes are left to upstream. The guarded 1 MiB initial
userspace stack is supplied by the current SDK.
