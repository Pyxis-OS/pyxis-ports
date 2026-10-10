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
The build creates native host PUC Lua 5.1 and `nlua0` with Neovim's mpack/bit
sources and LPeg. Upstream source, Vim syntax and help-tag generators run on
the host; the build never executes the target editor. Lua bytecode, LuaJIT,
gettext, unibilium and Wasmtime are disabled. No compiler container rebuild is
needed.

The staged bundle runs through the shell's explicit bundle route:

```text
boot://share/neovim/nvim.pxb file.c
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
passthrough reference; resize uses native geometry generations. Capability
traversal supplies scheme paths and `:cd`.

Loaded file buffers retain a file reference until unload. Ordinary
`:w` compares the held reference with the actual writable target, including
identity and modification-time validity, before truncation. A replaced target,
changed timestamp or unavailable comparison metadata requires explicit `:w!`.
Force still respects native write authority. References do not freeze names or
contents, and equal timestamps do not prove equal bytes.

External jobs, `system()`, `:terminal`, PTYs, socket listeners, Unix signals,
numeric PID operations, workers, asynchronous filesystem calls and watches are
outside this profile. Swap/backup recovery and patchmode are unavailable.
Dynamic Lua modules, dynamic grammar loading, LSP and true-colour output
are excluded. Seven upstream-bundled grammars are statically linked: `c`, `lua`,
`vim`, `vimdoc`, `query`, `markdown` and `markdown_inline`. Language registration
checks their actual Tree-sitter ABI and never uses `dlopen`. Lua, Markdown, help
and query use upstream Tree-sitter defaults; C and Vim retain legacy syntax,
with parsers available to explicit `vim.treesitter.start()`. A start request for
a language without a built-in parser quietly leaves/restores legacy syntax;
`language.add()` still returns no parser and its reason. Explicit dynamic paths
remain unsupported. Arbitrary
configuration can invoke unsupported operations and receive errors: file
watches (`vim._watch`, LSP file-change registration) and LSP over TCP call luv
functions that report ENOSYS, `vim.uv.available_parallelism()`, `os_getpid()` and
`os_homedir()` without `home` return nil where the runtime handles it, and
`vim.uv.os_get_passwd()` always returns nil. The client forwards its SYSTEM_INFO
grant to the embedded server, where Lua runs. `vim.fs.normalize('~')` yields
`home:/`, and `:checkhealth` stops at E5009 on the runtime scheme path before
reaching any uv call. QEMU
qualification covers local editing, highlighting, save and `:cd` with the
16-colour profile; it does not establish physical-host qualification.

The seven ordered patches adapt platform/process APIs, native paths and file
comparison, the console TUI, core/runtime assumptions, persistent undo, static parser registration and the swapfile prompt, which no
longer indexes the passwd record Pyxis refuses to supply. Undo
files are created with libc's 0666 creation mode instead of the edited
file's permission bits. Exact pins,
checksums and patch order are in `metadata.lua` and staged
`share/neovim/source.txt`. See [PORT-NOTICE.md](PORT-NOTICE.md) for licences
and local-change attribution.
