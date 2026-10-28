# lua-language-server

LuaLS 3.19.2 on native Pyxis threads, files and byte-pipe stdio. The checksum-pinned
LuaLS, bee.lua and LPegLabel inputs use the existing raw-github cache. Lua 5.5.0
is embedded coherently with bee; no host Lua or target Ninja is needed. The pinned bee bootstrap main and LuaLS
make/bootstrap.lua handle -E, -e and explicit scripts; native hooks supply bundle
roots, modules and worker lifetime. No replacement C++ argument parser is built.

```sh
lua build.lua lua-language-server --sdk /path/to/build/sdk
```

The staged `bin/lua-language-server.pxb` supplies the public program package
`lua-language-server`. Its declared command is also `lua-language-server`.
Install the package and opt in with `vim.lsp.enable('lua_ls')` in Neovim; the
editor uses its held `nvim_runtime://lua` library, with project settings in
`.luarc.json` or `.luarc.jsonc`. Its
stdio mode requires native byte-pipe input/output, as Neovim supplies. `--help`
and `--version` report usage/version without starting a server.

Required grants are memory, threads, precise read/sleep clock, pipe creation and
randomness for exclusive state creation. Ordinary bundle inheritance also
supplies its existing filesystem roots, including potentially writable home.
An owned instance is created under `home://.cache/lua-language-server/` and
removed after orderly exit. Forced exit may leave that instance for manual
cleanup. Default metadata and lazy-cache paths use the owned instance; package
resources are read-only. Upstream configuration, log/meta path overrides and
plugins use the inherited filesystem grants. Neither a dot-prefix nor a random
name is isolation.

The profile uses initial workspace scanning and editor text events; external
changes need an explicit reload/restart. Upstream JSON/JSONC and executable Lua
configuration, config-write actions,
Lua plugins, custom annotation directories and `--lazy` are available. The lazy
cache uses per-instance state without a process ID. Formatter/spelling/name-style
features, filesystem watchers, TCP and subprocess CLI modes remain unavailable. LuaLS keeps upstream command parsing and protocol
framing; native copied worker messages still have the admission bounds below.

Native serialization has explicit admission limits documented in
`pyxis/concurrency.md`; overflow fails visibly rather than dropping work. LSP
shutdown keeps input alive for exit. At orderly exit the reader is interrupted
and workers are joined before Lua/C++ finalization. Lua core uses its supported
C++ exception mode with C API linkage so error unwinding releases binding
owners. Missing clocks, files and rights report actual failures.

Upstream Bee filesystem bindings use the SDK's native `std::filesystem` and
C++23 formatting; the Pyxis build selects the SDK's supplied `format_string` API.
Unsupported operations, including real-file copying, report the SDK error.
Builtin metadata generation writes its in-memory templates through upstream FILE
operations. Retained native handles still identify owned instance state during
cleanup. The runtime uses no procfs, fake PIDs or fake CPU clock.
Its URI codec matches Neovim's native `ROOT://rest` / `file:///ROOT/rest` mapping.
Actual linked size, startup and manual runtime qualification are recorded by
the parent task; no unrun behavior or native performance is claimed here.

Embedded Lua keeps upstream locale handling through current libc's fixed-C
setlocale/localeconv API; querying or selecting C succeeds, other locale names
return nil. No mutable locale implementation is added.
