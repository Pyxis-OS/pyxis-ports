# Lua interpreter

Lua 5.5.1 from the official mirror, pinned to
`7579fc9d7ed90240487251dfb69168f8e64e9294`. The upstream MIT notice is retained
in `lua.h` and staged at `share/licenses/lua/lua.h`.

Build against an exported Pyxis SDK containing `sha256.h` and `libsha256.a`:

```sh
lua build.lua lua --sdk /path/to/pyxis/build/sdk
```

The recipe also exports `stage/dev/lib/liblua.a` and public headers under
`stage/dev/include`. The archive contains the pinned core, auxiliary library and
all upstream standard libraries and their initializer, without
the CLI's `main` or its native `pyxis` bridge.
Embeddings call `lua_pyxis_io_shutdown(state, normal)` immediately before
`lua_close(state)` to choose normal completion or error abandonment. No caller
pointer is borrowed. Without the hook, finalizers abandon pending transactions;
the hook is harmless when the embedding has not opened `io`.
Consumers choose which libraries to open. `lua.h` retains the MIT notice.
Pyxis stages these build inputs separately at `build/ports-dev/lua`; they are
included in the ports bundle but excluded from the boot archive. Link with the
same SDK used to build the archive. It provides upstream `luaL_openlibs`, debug and math. The standalone interpreter additionally links
the SDK's shared SHA-256 library for its native helper; liblua itself does not
link crypto.

The guest command is `bin/lua.pxb`, with its executable inside `app/bin/`.
The declaration preserves Lua's ordinary resources and requests optional
`job_work`, LAUNCH and PIPE context. Normal `lua` command lookup selects the
bundle. Pure Lua still starts when the optional job context is absent.

Bundle children can explicitly declare `service/create` when this runtime
already holds the endpoint constructor. Preparation selects it from the owned
caller capture and uses the SDK resolver's protocol/rights checks; missing
required authority refuses. The resulting constructor is a private runtime
role. Plain children keep their existing profile, and neither raw launch nor a
bundle omitting the request gains it. This needs the SDK's declared private
service-grant support; no compiler rebuild is required.

An explicit bundle `jit/manage` request can select the runtime's held `jit`
source. It remains private in the child. Automatic capability inheritance omits
JIT, including aliases. This requires the SDK's JIT ABI and bundle resolver
support. The shipped Lua bundle requests JIT optionally; a context without a
JIT source remains without one.

The standalone CLI initializes a source-only SDK runtime before startup chunks
or user code. `pyxis.run` is foreground; exact held shell/Lua interpreter
continuation receives the available sources, while other native programs retain
their ordinary profile and bundles select their declarations. Code identity is a
held native READ FILE, never a name or manifest ID. Embeddings do not initialize
this context just by linking liblua.

Handed/shebang scripts own a foreground subgroup; explicit filename, `-e` and
stdin scripts keep child launches under their positional owner. The REPL has no
script-wide group. Normal return, unhandled error and `os.exit` use the same
routing and cleanup as the corresponding shell mode. Enclosing owners can still
stop children outside a handed script's foreground subgroup. Captured jobs below
remain leaf jobs, without new script expansion or implicit provisioners.

From the guest shell:

```text
lua -e 'print("Hello from Lua", 2 ^ 0.5)'
lua -e 'local t = {3, 1, 2}; table.sort(t); print(table.concat(t, ","))'
```

`lua -e 'code'` executes a text chunk; repeated `-e`/`-l` options run in order.
`-l [global=]module` requires a module, `-v` prints the version, `-i` enters the
upstream REPL after execution, `-W` enables warnings and `-E` ignores Lua startup
environment variables. `lua file.lua [args...]` loads a
script, and `lua -- file.lua [args...]` permits filenames beginning with `-`.
Success returns zero; usage, file, allocation, compilation and execution errors
return one. Runtime errors include a Lua traceback. Script and `-e` chunk return
values are discarded; use `print` for output.

With no script or expression, real terminal stdin selects upstream's plain
stdio REPL; redirected stdin is executed as a script. The pinned upstream
argument parser, expression/continuation handling, `_PROMPT`/`_PROMPT2`, stdin
reader and error flow are used. There is no readline/libterm editor or custom
line limit. Native console bytes follow current libc; editable terminal input,
signal-driven cancellation and history are not promised. Upstream's signal()
calls reach libc's honest SIG_ERR/ENOTSUP implementation, which installs nothing.

Script paths use the shared libc working directory or an explicit Pyxis URI;
the interpreter retains its native directory grants. Module search is separate from script loading.
For example, `lua home://scripts/hello.lua Alice` runs that file while retaining
the caller's working directory. `arg[0]` is the supplied script name, positive
indices contain its arguments, and negative indices contain preceding interpreter
arguments. The script also receives its arguments through `...`. With `-e`,
`arg[0]` names the interpreter and positive indices contain `-e` and the chunk.

`loadfile([filename[, mode[, environment]]])` and `dofile([filename])` use the
same path rules. Omitting the filename or passing nil reads stdin. `loadfile`
retains its function-or-nil/error result; `dofile` executes and returns results.
Text/binary modes, UTF-8 BOM and initial `#` line skipping are retained. The CLI
`-` selects stdin; `lua -- -` opens a file literally named `-`. Files opened by
the loader are closed after loading, including syntax/read failures.

A direct shebang launch consumes the actual handed `script` file object through
a native descriptor/FILE owner. It uses the existing upstream BOM/shebang and
byte-reader logic and closes that owned FILE after loading; the diagnostic
filename is not reopened. Normal loadfile calls and LUA_INIT do not consume the
handed main-script object.

Upstream's standard initializer opens the libraries, including debug and math.
A small interpreter-only hook registers the existing native `pyxis` module.
`require` keeps upstream preload, Lua and C searchers;
C loading selects the upstream explicit unsupported implementation. Native
`package.cpath` defaults to empty. `LUA_PATH_5_5` takes precedence over `LUA_PATH`;
`;;` inserts the native default `?.lua;?/init.lua;boot://share/lua/?.lua;boot://share/lua/?/init.lua`.
Relative templates use inherited cwd. The driver does not replace the selected
path with the script's directory. `LUA_INIT_5_5`/`LUA_INIT` run startup file or
text chunks; `-E` suppresses them and package environment selection. Dots in
module names become `/`. Dynamic modules remain unavailable.

The global/preloaded `pyxis` table provides `run` (dense argv list, launch/wait,
actual signed exit status), `dir` (array of name/kind entries) and `sha256`
(streaming lowercase hex digest through the SDK's shared hash library). Paths
and grants use only the consumed SDK; missing authority and native failures raise Lua errors. The
module is executable-only, leaving configuration embeddings restricted.

## HTTP files

`io.open(uri, "r")` uses typed OPEN for an HTTP GET and returns a completed, re-readable FILE.
Use `file:read(count)` for bounded reads; `file:read("a")` allocates the entire
remaining body. The provider spools the complete response into private backing
before OPEN succeeds. Lua's file
and script loaders check read errors, so a failed response cannot be loaded as
complete code. The selected provider owns redirects and final HTTP errors.

`io.open(uri, "w")` and `"w+"` open a staged transaction through libc.
The captured provider selects its method and headers; stock HTTP roots use POST.
Writing only stages bytes; reads before commit fail with EBUSY. Flushing that
exact file commits once. With `"w+"`, the same file then reads the reply, and
later writes fail with EBADF. Use ordinary file operations:

```lua
local request = assert(io.open("https://example.com/submit", "w+"))
assert(request:write("hello"))
assert(request:flush())
while true do
  local bytes, message, code = request:read(4096)
  if not bytes then
    assert(not message, message)
    break
  end
  assert(io.write(bytes))
end
assert(request:close())
```

A successful final owning close also commits an unsubmitted clean transaction;
for a write-only request, check the close result:

```lua
local request = assert(io.open("https://example.com/submit", "w"))
assert(request:write("hello"))
assert(request:close())
```

Explicit `file:close()` and `file:flush()` use normal libc close/commit.
A successful lexical-scope close of a Lua 5.5 to-be-closed file also uses normal
close; a real close failure raises a Lua error. During error unwinding,
`__close` abandons a pending transaction without submission and preserves the
original Lua error. Ordinary garbage collection also abandons pending requests.
Successful intentional abandonment is not an error. Automatic cleanup preserves
the original error without adding diagnostics or aggregating failures into the
Lua exit status. Explicit flush/close results remain available to callers.

Before destroying its Lua state, the standalone interpreter selects normal
close after successful execution and abandonment after an unhandled Lua error.
`os.exit(status, true)` selects normal close, including a nonzero requested
status; without the close flag it follows upstream C process exit. An already
committed request is never submitted again by either path.
A FILE error still blocks normal commit. Lua exceptions and requested nonzero
statuses alone do not mark a FILE as failed.

Read, flush and explicit close failures retain Lua's ordinary
`nil, message, errno` results. Write failures also return the number of bytes
accepted by that call. Libc never retries a lost submission. Implicit buffer
drains and C `fflush(NULL)` only stage; Lua's `io.flush()` flushes its selected
output file and can commit that file. Normal process exit stages buffers and
commits clean transactions; finalization failure makes an otherwise successful
exit fail.

The interpreter has no HTTP-specific preparation, commit, status, abort or
constant wrappers. `http prepare` creates configured capabilities before launch;
Lua uses the caller's existing declared grants through ordinary libc I/O.
Generic descriptions carry outcomes, without HTTP observation companions.
Missing provider/private staging authority refuses the operation. No helper
provisions authority or expands the interpreter bundle's requests. The native
`pyxis.sha256` helper uses positioned FILE reads, including completed HTTP
snapshots opened through the caller's declared provider root. The hash primitive
requires no clock, random or allocator authority; file access keeps its existing
grants and owned-handle cleanup. Input beyond SHA-256's 64-bit bit count is
refused.

## Owned native jobs

`pyxis.job(argv, options)` captures one native command;
`pyxis.pipeline({argv1, argv2, ...}, options)` uses the same core for several
stages. Both return `job, status`, including an owned job with a nonzero native
start status when cleanup remains unresolved. They need the declared private
RAM `job_work`, clock, pipe and launcher grants. Missing context returns
`nil, CALL_UNAVAILABLE` before admission. There is no shared temporary-directory
fallback. Bundle selections retain their full manifest, stack and resource
policy; shebang expansion is not supported by these entry points.

```lua
local job, status = pyxis.job({"echo", "captured"}, {timeout_ms = 1000})
if job then
  local result, wait_status = job:wait()
  if result.complete then
    local bytes, read_status = job:read("stdout", 0, pyxis.READ_MAX_BYTES)
    local close_status = job:close()
  end
end
```

`timeout_ms` is required and positive. It becomes one absolute monotonic deadline
before argument/context preparation; progress does not extend it. Retirement
after stop may take longer than this deadline. Other options are binary `input`
(nil omits stdin; `""` supplies EOF), `stdout_limit`, `stderr_limit`, exact
`resources`, `roots`, `cwd`, `environment` and boolean `providers`. Zero/omitted
output limits select the core's 1 MiB caps and polled stopping triggers; these
are not hard backing quotas.

Omitted resources select the present `memory`, `threads`, `clock` and `random`
roles for raw stages. Raw TCP stages receive CONNECT only. A bundle may
additionally declare private `service/create` and TCP LISTEN from held authority
when resources is omitted; an explicit `tcp` selector permits that held source
for the bundle's declaration, while raw stages still receive CONNECT only.
An explicit dense list bounds these bundle role requests as well as raw stages;
supported names also include
`tcp`, `udp`, `dns_udp`, `net_registry`, `pipe`, `system_info` and `service`.
`service` explicitly selects CREATE as a private role. Omitting it from an
explicit list, including `resources={}`, leaves a required bundle service request
unsatisfied even when the parent holds that constructor. Missing explicit
selections fail. Bundle-only `jit/manage` declarations select the held private
source separately from these raw resource selectors.
Omitted roots/cwd/environment take owned current snapshots.
An explicit root list uses `{name="home", access="read"}` or `"read-write"`;
rights intersect held authority, and read access strips mutations. The cwd
chain is rebuilt under these roots before admission. An incompatible or
unspelled cwd fails; `cwd=false` deliberately supplies no cwd. An explicit
environment is an exact string map, including an empty table for no variables.
Providers default to absent; `providers=true` explicitly stages exported provider
capabilities from an owned capture. Private controllers and configured bundle
sources are excluded.
Capability and text selections share exact names; collisions refuse admission.
Text rejects NUL; finite stdin and captured output remain binary.

Image lookup uses the caller's captured `path` list, independently of child
roots/cwd. Missing and empty lists refuse bare-name lookup. Explicit paths resolve
from the caller's held path context; use `./name.pxe` for a relative filename.
Thus `roots={}` and `cwd=false` can select a held image without passing its
content roots to the child. Bundle resource requests use selected roles and roots,
selected providers, configured manifest sources and explicitly declared JIT
authority; required requests that cannot be satisfied refuse admission.
Selected role masks remain bounds on
bundle requests. Captured stages never inherit terminal/display, launcher,
`path` or `job_work`.
The caller must keep private-tree, provider and credential reachability
consistent with the core's capture audit for the job's lifetime.

`job:step()` and `job:wait()` return `result, status`; `cancel()` returns a
status without waiting. Result field names mirror `pyxis_pipeline_result`,
including `stage_valid`, `stages[i].kind/exit_status`, `stdout_output` and
`stderr_output`. Nonzero child exits are data. Native kind constants are
`PROCESS_EXITED`, `PROCESS_FAULTED`, `PROCESS_TERMINATED`, `PROCESS_START_FAILED`.
For `PROCESS_START_FAILED`, `exit_status` preserves the native packed startup
step and call status. `LAUNCH_NO_STAGE` is exposed as `-1`; refused `failed_stage`
indices remain native and zero-based.
Reads require confirmed cleanup and accept explicit offset/count with at most
`READ_MAX_BYTES` per call. Completed snapshots are
repeatable. Close consumes once even on a release error; BUSY preserves ownership.

A strong registry retains jobs through lost variables, GC and protected Lua
errors. `pyxis.jobs()` returns those userdata records for explicit recovery;
there is no implicit GC wait, background pump or refreshed deadline. Explicit
cancel/wait/close owns retirement. Process/session teardown remains the fallback
when the caller exits; there is no per-job caller-death stop guarantee. Lua's
ordinary `pyxis.run` never forwards the private leaf or its provisioning ancestor.

The staged pure Lua module `require "pyxis.commands"` provides:

```lua
result, status, job = commands.run(argv, options)
stdout, stderr, result, status, job = commands.capture(argv, options)
stdout, stderr, result, status, job = commands.pipeline(stages, options)
```

These wait, read capped binary streams and close after confirmed cleanup.
The optional final job remains owned only when cleanup is unresolved. `run`
replays to current `io.stdout`/`io.stderr`, or open Lua FILEs selected with
`options.stdout`/`options.stderr`, then flushes. Output errors retain the actual
native results and add `operation_error`; `delivery` records accepted bytes,
flush confirmation and uncertainty. A failed write reports unknown acceptance,
and failed writes/flushes are never replayed. `capture`/`pipeline` return partial
known bytes on read failure, without trimming. Explicit `LUA_PATH` still controls
file module search, including this module.

`pyxis.run`, `pyxis.job` and `pyxis.pipeline` use libpyxis's shared command
selection. Bare names walk the captured ordered `path` capability list;
missing or empty lists provide no search. Only an absent command permits the
next entry. A denied, malformed, incomplete or conflicting plain/bundle target
reports its error. There is no implicit bin, boot, home, cwd or development
catalog search. Explicit URI and relative paths keep their held path authority;
a `.pxb` path selects the bundle's default command. These native APIs do not
interpret textual `PATH`; child environment text does not alter parent lookup.

`pyxis.run` captures the current libc cwd and environment for child launch.
It forwards the exported `path` list used for lookup. Explicitly exported
named capabilities, including native FILEs and controllers, retain their actual
rights and transport in plain children. Fixed runtime roles keep their protocol
checks, narrowed masks and conditional input selection; stdio and bundle-owned
entries follow their separate selected handoffs. Bundles receive their complete
manifest's requested resources from Lua's held grants and read-only app/resource
roots. Package selections retain the captured set for image, stack and resources;
other package grants still require an independently held matching source.
Plain children omit an inherited `app` root. Bundled scripts and ZIP bundles
are not supported.

`pyxis.run` inherits live C stdin/stdout/stderr, omitting closed streams.
`io.input`/`io.output` rebinding remains Lua-local. Standard files retain upstream Lua's protection against
closing them through `io.close`; the native accessor also handles streams closed
at the C runtime level. FILE append policy is preserved through the selected
stream's TEXT companion after a zero-length native APPEND support/authority
probe. Append companions on stdin or non-FILE streams refuse before launch.
File cursors, pushback and read-ahead remain private;
child file streams start at zero. FILE and forward-only STREAM input carry
their own generic description; neither needs an HTTP observation companion.
Append uses libc's native FILE APPEND, atomic per operation of at most 4,080
bytes. Larger writes and formatted lines may interleave; success does not imply
storage sync. A successful launch probe does not reserve later write success.
Launch authority is LAUNCH-only and explicitly opt-in per space. Shell Ctrl+C
terminates Lua itself; a child can outlive it, subject to any enclosing remote
execution group's lifetime.

`os.tmpname` reserves an empty exclusive `tmp://lua-XXXXXX` file; the caller
removes it. `io.tmpfile` creates and immediately unlinks an exclusive read/write
file. Both need native clock/random authority; anonymous files also need REMOVE.
Abrupt death between creation/unlink, or a failed unlink, can leave a named file.
No stale names are automatically deleted. `os.time()` is wall time.
`os.time(table)` converts with libc's `mktime` in the zone `TZ` selects and
normalizes the table's fields, as upstream does. A wall time skipped by a DST
gap, or repeated in a fold without an `isdst` field that settles it, makes
`mktime` fail, and `os.time` raises upstream's "time result cannot be
represented" error. An `isdst` that disagrees with an unambiguous wall time
reads it in the requested kind of time, as in standard C. `os.date` uses real
C-locale strftime, UTC/TZif names and numeric offsets. String comparison uses
libc's `strcoll`, which compares in byte order in the only, C, locale.
`os.rename` uses libc's replacement policy where the filesystem and caller's
authority permit it. Replacement requires matching kinds and an empty directory
victim; directory moves require RAM or writable npfs.

Absent functions are `os.execute` and `os.clock`. `os.setlocale` and numeric
locale handling use the real fixed-C libc API; other locale names return nil. `file:setvbuf` uses current libc. `io.popen` and dynamic C loading retain
upstream explicit unsupported behavior instead of disappearing from their tables.
The base library's in-memory `load`, string `dump` and normal integer/double
arithmetic remain available.

## Adaptations

- `0002` omits unavailable OS operations and adapts handed script ownership. File
  loading uses upstream stdin and BUFSIZ sizing, opening named files in binary
  mode before parsing, without the unavailable `freopen` cycle. Module paths,
  environment selection and unsupported loader behavior follow upstream.
  Calendar conversion and date/time error policy follow the pinned source.
- `0003` separates ordinary FILE close from native abandonment, preserves
  foreign callbacks and standard-file protection, and exposes the two-argument
  shutdown hook. It adds no automatic cleanup diagnostics or exit aggregation.
- `config.h` selects Lua's existing hooks for
  actual stdin terminal detection, reentrant calendar conversion
  and libc mkstemp-backed reserved names. There is no fake locale or
  signal implementation, and Lua's numeric types/recursion limits are unchanged.
- The pinned upstream `lua.c` is compiled as the interpreter, with only native
  script-object, module registration and final IO-ownership hooks. Its parser,
  checks, signal calls, handlers and stdio REPL are unchanged. The SDK supplies
  allocation, stdio and math. No handwritten interpreter driver is built.

The process has the general 1 MiB eager native stack and an unmapped guard page;
Lua's value stacks live on the heap. Native parser/callback and pattern recursion
retain upstream's limits. These limits are not a proof against every possible
combination exhausting the fixed native stack; automatic growth is deferred.
Lua bytecode calls ordinarily use the VM loop rather than one C frame per Lua
call. There is no signal-driven cooperative interruption: the parent shell can
terminate Lua while a chunk runs. Session configuration is evaluated by the separate userland launcher, using the exported Lua library.

`pyxis.monotonic_ns()` reads the declared CLOCK READ grant and returns an exact
Lua integer for absolute monotonic deadlines. Missing authority or an unrepresentable
reading raises an error; it never substitutes a clock value.

Native child launches preserve the held `dns_udp` resolver factory and read-only
`net_registry` grant when declared. Packet grants retain their immutable
interface scope; registry access does not grant packet or configuration rights.

The shipped Lua bundle requests the optional `dns_udp` path alongside its existing
UDP grant. It does not request registry observation; custom declared grants can
supply that separately. Its manifest remains within the native 20-request limit.
