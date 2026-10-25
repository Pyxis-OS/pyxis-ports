# lua-language-server

LuaLS 3.19.2 on native Pyxis threads, files and byte-pipe stdio. The checksum-pinned
LuaLS, bee.lua and LPegLabel inputs use the existing raw-github cache. Lua 5.5.0
is embedded coherently with bee; no host Lua or target Ninja is needed.

```sh
lua build.lua lua-language-server --sdk /path/to/build/sdk
```

The staged `bin/lua-language-server.pxb` supplies the public program package
`lua-language-server`. Its declared command is also `lua-language-server`.
Install the package and opt in with `vim.lsp.enable('lua_ls')` in Neovim; the
editor uses its held `nvim_runtime://lua` library, with project settings in
`.luarc.json` or `.luarc.jsonc`. Its
stdio mode requires native byte-pipe input/output, as Neovim supplies. `--help`
and `--version` report the native profile without starting a server.

Required grants are memory, threads, precise read/sleep clock, pipe creation and
randomness for exclusive state creation. Ordinary bundle inheritance also
supplies its existing filesystem roots, including potentially writable home.
An owned instance is created under `home://.cache/lua-language-server/` and
removed after orderly exit. Forced exit may leave that instance for manual
cleanup. Generated metadata paths stay under the owned instance; package
resources are read-only. Neither a dot-prefix nor a random name is isolation.

The profile uses initial workspace scanning and editor text events; external
changes need an explicit reload/restart. Configuration is JSON/JSONC only. Executable config,
plugins, downloads, config-write actions, persistent analysis cache/`--lazy`,
formatter/spelling/name-style features, watchers, TCP and subprocess modes are
unavailable, with their entry points/capabilities gated explicitly.

Native serialization has explicit admission limits documented in
`pyxis/concurrency.md`; overflow fails visibly rather than dropping work. LSP
shutdown keeps input alive for exit. At orderly exit the reader is interrupted
and workers are joined before Lua/C++ finalization. Lua core uses its supported
C++ exception mode with C API linkage so error unwinding releases binding
owners. Missing clocks, files and rights report actual failures.

The runtime does not use std::filesystem, procfs, fake PIDs or a fake CPU clock.
Its URI codec matches Neovim's native `ROOT://rest` / `file:///ROOT/rest` mapping.
Actual linked size, startup and manual runtime qualification are recorded by
the parent task; no unrun behavior or native performance is claimed here.
