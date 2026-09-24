# Lua interpreter

Lua 5.5.1 from the official mirror, pinned to
`7579fc9d7ed90240487251dfb69168f8e64e9294`. The upstream MIT notice is retained
in `lua.h` and staged at `share/licenses/lua/lua.h`.

Build against an exported Pyxis SDK:

```sh
lua build.lua lua --sdk /path/to/pyxis/build/sdk
```

The guest executable is `bin/lua.pxe`; Pyxis installs it at `app://lua.pxe`.
From the guest shell:

```text
lua -e 'print("Hello from Lua", 2 ^ 0.5)'
lua -e 'local t = {3, 1, 2}; table.sort(t); print(table.concat(t, ","))'
```

`lua -e 'code'` executes one text chunk. `lua file.lua [args...]` loads a
script, and `lua -- file.lua [args...]` permits filenames beginning with `-`.
Success returns zero; usage, file, allocation, compilation and execution errors
return one. Runtime errors include a Lua traceback. No arguments prints usage;
this is not yet an interactive REPL. Chunk return values are discarded; use
`print` for output.

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
  the upstream CLI and all standard libraries. Its message handler follows
  upstream `lua.c` under the same MIT notice. Library initialization, compilation
  and execution are protected against Lua errors, including argument-table
  construction and file loading; the state is closed before process exit. The
  SDK supplies allocation, stdio and the core math subset.

The process has the general 1 MiB eager native stack and an unmapped guard page;
Lua's value stacks live on the heap. Native parser/callback and pattern recursion
retain upstream's limits. These limits are not a proof against every possible
combination exhausting the fixed native stack; automatic growth is deferred.
Lua bytecode calls ordinarily use the VM loop rather than one C frame per Lua
call. There is no signal-driven interruption: Ctrl+C cannot stop a running
expression. REPL input/cancellation and configuration evaluation are later work.
