# Lua interpreter

Lua 5.5.1 from the official mirror, pinned to
`7579fc9d7ed90240487251dfb69168f8e64e9294`. The upstream MIT notice is retained
in `lua.h` and staged at `share/licenses/lua/lua.h`.

Build against an exported Pyxis SDK:

```sh
lua build.lua lua --sdk /path/to/pyxis/build/sdk
```

The recipe also exports `stage/dev/lib/liblua.a` and public headers under
`stage/dev/include`. The archive contains the pinned core, auxiliary library and
selected base/coroutine/table/string/UTF-8 libraries, without the CLI's `main`.
Consumers choose which libraries to open. `lua.h` retains the MIT notice.
Pyxis stages these build inputs separately at `build/ports-dev/lua`; they are
included in the ports bundle but excluded from the boot archive. Link with the
same SDK used to build the archive. This does not provide `luaL_openlibs`, io/os,
package, debug or the full math library.

The guest executable is `bin/lua.pxe`; Pyxis installs it at `boot://lua.pxe`.
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
there is no executable/module search or implicit change to the script's directory.
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

The driver registers base, coroutine, table, string and UTF-8 libraries.
The base library's in-memory `load` and string library's `dump` remain available.
There is no package/require, debug library, io/os library or full math library.
Normal Lua integer/double arithmetic remains available. No environment startup
chunks are evaluated, and no native modules are loaded.

## Adaptations

- `0001` carries the production changes from upstream GC fix
  `0b29f408433e92953cc72b1d3e06c7ac8139e439`, avoiding a negative left shift when
  evaluating the major-to-minor collection threshold. Upstream tests are not
  imported.
- `0002` removes locale header dependencies, requires explicit filenames and
  uses a port-owned 512-byte file read buffer. Files open in binary mode from
  the start; Pyxis's identical text/binary stream behavior makes the upstream
  `freopen` cycle unnecessary. No stdio buffering API is introduced.
- `config.h` selects Lua's existing hooks for an integer hook flag (no signals),
  decimal point `.` and bytewise string collation. There is no fake locale or
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
call. There is no signal-driven interruption: Ctrl+C cannot stop a running
chunk. REPL cancellation applies only while reading input. Session configuration
is evaluated by the separate userland launcher, using the exported Lua library.
