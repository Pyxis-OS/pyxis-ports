# Neovim

Neovim 0.12.5, pinned to `5885a30e1e1225349079e7a1c4a3848aa8e43e42`,
built as a native terminal editor against the Pyxis SDK. The recipe verifies
the archive checksum and fetches sources only from owner mirrors.

```sh
lua build.lua neovim --sdk /path/to/build/sdk \
  --lua51 /path/to/lua51/dev --libuv /path/to/libuv/dev \
  --utf8proc /path/to/utf8proc/dev --tree-sitter /path/to/tree-sitter/dev
```

These development prefixes provide real static Lua 5.1, LPeg, luv, libuv,
utf8proc and tree-sitter libraries. Encoding conversion uses SDK libc's iconv.
Their `share/` trees supply notices and source provenance for the bundle.
Lua51 defaults to abandoning pending FILE transactions during implicit Lua-state
teardown. Neovim keeps that default without selecting normal publication or
adding cleanup-failure diagnostics/exit policy. Its runtime uses Lua51's restored
`package.cpath` and explicit unsupported `package.loadlib` implementation.
The build creates native host PUC Lua 5.1 and `nlua0` with Neovim's mpack/bit
sources and LPeg. Upstream source, Vim syntax and help-tag generators run on
the host; the build never executes the target editor. Lua bytecode, LuaJIT,
gettext, unibilium and Wasmtime are disabled. No compiler container rebuild is
needed.

The staged `bin/nvim.pxb` bundle runs without a catalog on live media and
installed revisions:

```text
nvim file.c
bin://nvim.pxb file.c
```

Its manifest requests memory, clock READ/SLEEP, launcher and pipe CREATE, and
optionally SYSTEM_INFO READ, which backs `vim.uv.os_uname()` and the hostname.
The terminal client forwards it to the same-image server.
It delegates the read-only runtime as `nvim_runtime://`. The terminal client
launches only its same-image `--embed` server over directional stdio pipes;
that internal child explicitly receives pipe CREATE and retains the runtime
root. The parent's `app://` is excluded from child inheritance. Redirected
stdin requiring fd 3 rejects before launch.

`nvim_runtime://sysinit.vim` loads before optional user configuration at
`home://.config/nvim/init.lua` or `home://.config/nvim/init.vim`. It selects the
Pyxis 16-colour scheme, `notermguicolors` and
disabled swap/backup/writebackup. Startup sets `NVIM_NOTTYFAST`, clears
`COLORTERM` and selects the explicit runtime. Terminal input owns a native RAW
passthrough reference; queued SIGWINCH watchers re-query terminal geometry.
Capability traversal supplies scheme paths and `:cd`.

File selection proves the existing parent, then anchors the final component to
an absolute native scheme path; missing/unproved parents refuse selection.
The selected name stays fixed across `:cd`. Saving follows upstream open,
truncation, modification-time warning and `:w!` behavior through libc/libuv.
Existing files use ordinary `O_TRUNC`; new files use upstream `O_CREAT`.
Plain `:w >> missing` refuses; `:w! >> missing` creates and appends. Append uses
libc's native FILE APPEND, atomic per operation of at most 4,080 bytes. A buffer
write may span operations and interleave; it does not imply storage sync.
Unsupported append targets refuse before creation or truncation. Rename retains
upstream's unlink/copy fallback, with each operation checked by native grants.
Writes and fallback operations can leave partial output on failure.

Native stat validity stays explicit. Pathname FileID queries return unavailable
because their native identity tokens do not survive the released reference;
no persistent IDs are invented. File-save admission does not require retained
identity or modification-time metadata. Force still respects native authority.

External jobs and shell commands run as native children over pipes:
`'shell'` is the Pyxis `shell -c`; collection uses `>%s 2>&1` for stdout
and stderr. Executable checks use shared program lookup over the exported
`path` capability list. `jobstop` and Ctrl+C terminate at once, and temporary
files live in `tmp://`. A child startup failure keeps its native completion
reason and maps to job status 1. `:terminal`, PTY and detached jobs, socket listeners,
numeric PID operations, workers, asynchronous filesystem calls and watches are
outside this profile. SIGINT and SIGWINCH use persistent watchers on the
initial-thread main loop; headless startup omits the WINCH watch when no standard
descriptor is a terminal. Actual SIGINT follows upstream deadly-signal cleanup
and preserve-exit behavior. Raw Ctrl+C stays input-byte edit interruption.
Upstream Unix signal watchers retain their normal lifecycle; signals without a
native source remain dormant. Only INT and WINCH have native event delivery.
Swap/backup recovery and patchmode are unavailable.
`vim.uri` maps a native path to a file URI with its root name as the first
segment, `home://test/a.c` to `file:///home/test/a.c` and back; a buffer name
is a native path only when the editor holds that root.
Language servers run as attached children. The port ships an opt-in clangd
configuration (`vim.lsp.enable('clangd')`) using the bare `clangd` command
exported by the installed `clang-hosted` package. It resolves through the held
`path` list's live package view, so the next server launch sees a package switch
without restarting the space. A missing command refuses without an archive or
home-directory fallback. The editor declares `"sdk": "optional"` and passes its
`sdk://` to clangd, which therefore sees the package set captured when the
editor started; without `sdk://` the editor still starts, and clangd has no
system headers. Resource views such as `clang_resources://` come from the exact
retained selection libuv launched, without reopening its manifest or resolving
the command again. The editor's own roots take precedence. Header buffers pin
read-only directory copies through package switches and server exit; a new read
with conflicting active server roots refuses as ambiguous. Server exit removes
only its active bindings; wiping a buffer releases its retained view.

Lua editing has a separate opt-in configuration. Install the public
`lua-language-server` package, then add `vim.lsp.enable('lua_ls')` to
`home://.config/nvim/init.lua`, or run it with `:lua` in the session.
It launches the bare package command through the same native stdio mechanism
as clangd and attaches only to Lua buffers. The nearest `.luarc.json`,
`.luarc.jsonc` or `.git` selects the root; a file without those markers uses
its directory. API-library buffers reuse an existing LuaLS session.

The default analysis language is `Lua 5.1`, matching this editor's PUC Lua
runtime. LuaLS reads the complete `nvim_runtime://lua` library, including the
real `vim/_meta` signatures, from the editor's held read-only runtime.
The library is parsed as source data; its meta files are not executed.
No allowed-global substitute, LuaJIT setting, package lookup or download is
used. Normal LSP completion, diagnostics and semantic tokens remain available;
the configuration requests no watched-file registration or formatter.

Project `.luarc.json`/`.luarc.jsonc` settings override these client defaults.
For example, a project running Pyxis Lua 5.5 can select its actual language
and omit the Neovim API library:

```json
{
  "runtime.version": "Lua 5.5",
  "workspace.library": []
}
```

These analysis settings are independent of LuaLS's embedded Lua 5.5 interpreter.
Executable LuaLS project configuration and plugins remain unavailable. Restart
LuaLS after changing project JSON/JSONC settings; closed files and settings
files are not watched. Ordinary child launch retains the editor's
exported filesystem roots and cwd with their existing rights; this is not
workspace confinement. The `nvim_runtime` resource itself stays read-only.

Dynamic Lua modules, dynamic grammar loading and true-colour output
are excluded. Seven upstream-bundled grammars are statically linked: `c`, `lua`,
`vim`, `vimdoc`, `query`, `markdown` and `markdown_inline`. Language registration
checks their actual Tree-sitter ABI and never uses `dlopen`. Lua, Markdown, help
and query use upstream Tree-sitter defaults. `sysinit.vim` also starts it for
`c` and `vim` buffers (`runtime/lua/vim/_pyxis_treesitter.lua`, one `FileType`
autocommand in the `pyxis_treesitter` group, which `init.lua` can clear) and sets
`g:c_syntax_for_h`, so `.h` files are `c`. A start request for
a language without a built-in parser follows upstream parser-error behavior;
`language.add()` returns no parser and its reason. Explicit dynamic paths
remain unsupported. Arbitrary
configuration can invoke unsupported operations and receive errors: file
watches (`vim._watch`, LSP file-change registration) and LSP over TCP call luv
functions that report ENOSYS. `vim.uv.available_parallelism()` reports the
startup usable CPU count. `os_getpid()` and `os_homedir()` without `home` return
nil where the runtime handles it, and
`vim.uv.os_get_passwd()` always returns nil. The client forwards its SYSTEM_INFO
grant to the embedded server, where Lua runs. `vim.fs.normalize('~')` yields
`home:/`, and `:checkhealth` stops at E5009 on the runtime scheme path before
reaching any uv call. QEMU
qualification covers local editing, highlighting, save and `:cd` with the
16-colour profile; it does not establish physical-host qualification.

The nine ordered patches adapt platform/process APIs, native paths and file
comparison, the console TUI, core/runtime assumptions, persistent undo, static
parser registration, anchored file selection, external jobs, file URIs and
language-server resource bindings. Final native process, path, terminal underline
and command-selection changes live in the original backend patches. Generic
error handling and missing-parser behavior follow upstream. Undo
files are created with libc's 0666 creation mode instead of the edited
file's permission bits. Exact pins,
checksums and patch order are in `metadata.lua` and the bundle's
`app/metadata/source.txt`. Notices for Neovim, its linked dependencies and SDK
runtime travel under `app/metadata/licenses/`; the port note is beside them.
See [PORT-NOTICE.md](PORT-NOTICE.md) for licences
and local-change attribution.

The optional `fat32_data` directory grant selects the launch source `fat32_write`
and binds it as `edit://`. It is supplied only by explicit trusted configuration
or a shell-owned writable FAT mount named `fat32://`; it grants no disk or mount
management authority. Without that source, ordinary startup is unchanged. Open
`edit://FILE` explicitly to edit the selected scratch volume; inherited FAT views
remain read-only. Device identifiers belong only in local configuration.

