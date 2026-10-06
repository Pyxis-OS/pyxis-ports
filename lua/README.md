# Lua interpreter

Lua 5.5.1 from the official mirror, pinned to
`7579fc9d7ed90240487251dfb69168f8e64e9294`. The upstream MIT notice is retained
in `lua.h` and staged at `share/licenses/lua/lua.h`.

Build against an exported Pyxis SDK and the configured Mbed TLS development
prefix from this ports checkout:

```sh
lua build.lua lua --sdk /path/to/pyxis/build/sdk \
  --mbedtls /path/to/pyxis/build/ports/mbedtls/stage/dev
```

The recipe also exports `stage/dev/lib/liblua.a` and public headers under
`stage/dev/include`. The archive contains the pinned core, auxiliary library and
selected base/coroutine/table/string/UTF-8/io/os/package libraries, without
the CLI's `main` or its native `pyxis` bridge.
Consumers choose which libraries to open. `lua.h` retains the MIT notice.
Pyxis stages these build inputs separately at `build/ports-dev/lua`; they are
included in the ports bundle but excluded from the boot archive. Link with the
same SDK used to build the archive. This does not provide `luaL_openlibs`,
debug or the full math library. The standalone interpreter additionally links
Mbed TLS PSA for its native SHA-256 helper; liblua itself does not link crypto.

The guest executable is `bin/lua.pxe`; Pyxis installs it at `bin://lua.pxe`.
From the guest shell:

```text
lua -e 'print("Hello from Lua", 2 ^ 0.5)'
lua -e 'local t = {3, 1, 2}; table.sort(t); print(table.concat(t, ","))'
```

`lua -e 'code'` executes one text chunk. `lua file.lua [args...]` loads a
script, and `lua -- file.lua [args...]` permits filenames beginning with `-`.
Success returns zero; usage, file, allocation, compilation and execution errors
return one. Runtime errors include a Lua traceback. Script and `-e` chunk return
values are discarded; use `print` for output.

With no arguments, `lua` opens a libterm REPL. Expressions print their results;
statements execute normally, and globals persist between chunks. As in Lua's
CLI, locals belong to a single chunk. Incomplete statements use a `>> ` prompt
until Lua can compile them. Syntax/runtime errors, including errors in the global
`print` function, are reported before returning to the primary `> ` prompt.

Ctrl+C discards the whole pending chunk. Ctrl+D on an empty line exits zero,
including at the continuation prompt; with text on the line it is ignored.
The editor supports insertion, deletion and cursor movement, without history.
Each line is limited to 1023 bytes and the visible terminal capacity. Rejected
input or input loss discards the pending chunk instead of executing partial
source. Multiline source grows on Lua's heap. Allocation and terminal failures
exit nonzero. Prompts are fixed; `_PROMPT`/`_PROMPT2` overrides are not supported.

Script paths use the inherited working directory or an explicit Pyxis URI;
the interpreter retains that cwd. Module search is separate from script loading.
For example, `lua home://scripts/hello.lua Alice` runs that file while retaining
the caller's working directory. `arg[0]` is the supplied script name, positive
indices contain its arguments, and negative indices contain preceding interpreter
arguments. The script also receives its arguments through `...`. With `-e`,
`arg[0]` names the interpreter and positive indices contain `-e` and the chunk.

`loadfile(filename[, mode[, environment]])` and `dofile(filename)` use the same
path rules and require a filename. `loadfile` retains its function-or-nil/error
result; `dofile` executes the chunk and returns its results. Lua's text/binary
modes, UTF-8 BOM handling and initial `#` line skipping are retained. File streams
are closed after loading, including syntax/read failures. Omitting a filename
or using nil is an argument error; stdin scripts are not supported. A bare `-`
is not a stdin selector (`lua -- -` can open a file literally named `-`).

Direct shebang launches remain deferred: the Pyxis launcher passes an open
`script` file capability, which must eventually be consumed as that exact object.
The driver currently rejects this handoff explicitly instead of reopening its
diagnostic filename. Run `lua file.lua` for this milestone.

The driver registers base, coroutine, table, string, UTF-8, io, bounded os and
pure-Lua package libraries. `require` searches preloads and Lua files only.
An exact `LUA_PATH` value overrides all defaults, even if empty; versioned
variables and `;;` expansion are not used. Otherwise `?.lua` and `?/init.lua`
are searched in the script's directory, then `boot://share/lua/`. The REPL and
`-e` use inherited cwd before boot modules. Dots in module names become `/`.
No environment startup chunks or dynamic modules are loaded.

The global/preloaded `pyxis` table provides `run` (dense argv list, launch/wait,
actual signed exit status), `dir` (array of name/kind entries) and `sha256`
(streaming lowercase hex digest through PSA). Paths and grants use only the
consumed SDK; missing authority and native failures raise Lua errors. The
module is executable-only, leaving configuration embeddings restricted.

`pyxis.run` inherits live C stdin/stdout/stderr, omitting closed streams.
`io.input`/`io.output` rebinding remains Lua-local. Standard files retain upstream Lua's protection against
closing them through `io.close`; the native accessor also handles streams closed
at the C runtime level. File cursors, append state,
pushback and read-ahead remain private; child file streams start at zero.
Launch authority is LAUNCH-only and explicitly opt-in per space. Shell Ctrl+C
terminates Lua itself; a child can outlive it, subject to any enclosing remote
execution group's lifetime.

`os.tmpname` reserves an empty exclusive `tmp://lua-XXXXXX` file; the caller
removes it. `io.tmpfile` creates and immediately unlinks an exclusive read/write
file. Both need native clock/random authority; anonymous files also need REMOVE.
Abrupt death between creation/unlink, or a failed unlink, can leave a named file.
No stale names are automatically deleted. `os.time()` is wall time; calendar
tables are rejected. `os.date` uses real C-locale strftime, UTC/TZif names and
numeric offsets. Native `os.rename` does not replace an existing target.

Absent functions are `file:setvbuf`, `io.popen`, `os.execute`, `os.clock`,
`os.setlocale`, `package.loadlib`, C-module searchers, debug and full math.
The base library's in-memory `load`, string `dump` and normal integer/double
arithmetic remain available.

## Adaptations

- `0001` carries the production changes from upstream GC fix
  `0b29f408433e92953cc72b1d3e06c7ac8139e439`, avoiding a negative left shift when
  evaluating the major-to-minor collection threshold. Upstream tests are not
  imported.
- `0002` removes locale header dependencies, requires explicit filenames and
  uses a port-owned 512-byte file read buffer. Files open in binary mode from
  the start; Pyxis's identical text/binary stream behavior makes the upstream
  `freopen` cycle unnecessary. The patch also removes unsupported io/os
  entries and dynamic loading paths, retaining pure-Lua package searchers.
  Failed clock/strftime calls report errors instead of formatting fake time.
- `config.h` selects Lua's existing hooks for an integer hook flag (no signals),
  decimal point `.`, bytewise string collation, reentrant calendar conversion
  and libc mkstemp-backed reserved names. There is no fake locale or
  signal implementation, and Lua's numeric types/recursion limits are unchanged.
- `main.c` embeds the upstream core through its public API instead of building
  the upstream CLI and all standard libraries. Its message handler and REPL
  compilation flow follow upstream `lua.c` under the same MIT notice.
  Library initialization, compilation
  and execution are protected against Lua errors, including argument-table
  construction and file loading; the state is closed before process exit. The
  SDK supplies allocation, stdio and the core math subset.

The process has the general 1 MiB eager native stack and an unmapped guard page;
Lua's value stacks live on the heap. Native parser/callback and pattern recursion
retain upstream's limits. These limits are not a proof against every possible
combination exhausting the fixed native stack; automatic growth is deferred.
Lua bytecode calls ordinarily use the VM loop rather than one C frame per Lua
call. There is no signal-driven cooperative interruption: the parent shell can
terminate Lua while a chunk runs, while REPL line cancellation applies only
while reading input. Session configuration
is evaluated by the separate userland launcher, using the exported Lua library.
