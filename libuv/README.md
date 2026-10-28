# Native libuv

Build the pinned libuv 1.52.1 archive against a current Pyxis SDK:

```sh
lua build.lua libuv --sdk /path/to/build/sdk
```

The recipe downloads only the owner mirror and verifies SHA-256 before applying
its platform patch. It builds a static library, development headers, licenses
and the unpacked `share/libuv/uv-relay.pxb` example bundle. No compiler container
rebuild is needed. Link the archive before the SDK runtime libraries.

The example relays a child's pipe output to a console stream while a timer and
console input remain active. Run it through the shell's explicit development
bundle route:

```text
boot://share/libuv/uv-relay.pxb
```

It requests memory, clock, launcher and pipe/create authority in its manifest.
The shell supplies only grants it holds. It is not added to the default command
catalog. Its child runs the same image's `--child` mode; plain children receive
ordinary memory/clock/launcher grants, explicit streams and inherited directory
roots excluding the parent's program-specific app root. Pipe creation is not
forwarded to plain children.

Bundle children can explicitly declare `service/create` when this runtime
already holds the endpoint constructor. Preparation selects it from the owned
caller capture and uses the SDK resolver's protocol/rights checks; missing
required authority refuses. An explicit extra-resource `service` selection
replaces the held role with exactly its selected rights and transport, including
a zero-right selection that refuses a required CREATE. The resulting constructor
is a private runtime role. Plain children keep their existing profile, and neither raw launch nor a
bundle omitting the request gains it. This needs the SDK's declared private
service-grant support; no compiler rebuild is required.

Bundle children may explicitly request `jit/manage` from the held private `jit`
source. An explicit extra-resource `jit` replaces that source with its selected
rights and transport. The resolver keeps the declared role private. Automatic
capability inheritance omits JIT, including aliases; explicit native extra-resource
delegation remains available for plain children. This needs the SDK's JIT ABI
and bundle resolver support. The shipped example bundle does not request JIT.

The native backend supports one loop owner thread, timers, prepare/check/idle
callbacks, coalesced same-thread async wake, native pipe and console streams,
explicit child launch and immutable process completion. Loop initialization
requires clock READ and pipe CREATE. Sleep requires clock SLEEP. No wall time
is substituted for monotonic time, and clock failures in value-only clock APIs
abort rather than returning invented timestamps.

Each loop reserves one of the native 32 wait interests for its wake pipe. An
opened stream or observed child reserves one of the remaining 31 interests,
including inactive streams; timers consume none. Capacity failure is UV_ENOSPC
before stream ownership transfer or child publication. A wait beyond 30 seconds
is capped and recomputed. A native wait error returns its negative UV error
from uv_run, in addition to the ordinary zero/alive return values.

Stream open transfers responsibility for its libc descriptor. It must not be
closed, reused or independently consumed until the libuv handle closes. Data
I/O through a duplicated descriptor also consumes the shared cursor and
read-ahead, so leave its aliases unused for I/O while the stream owns it. Data
I/O uses libc's descriptor-aware try operations, preserving read-ahead. Submitted
writes copy buffer descriptors and borrow their bytes until one callback. Close
cancels pending writes before the close callback; successful queued completions
keep their result. Pipe shutdown closes its write descriptor after queued writes.
TTY RAW mode uses libc's native terminal mode owner for the supplied descriptor,
making Ctrl+C ordinary input without a separate libuv passthrough grant. The
first RAW call saves the preceding owner state; repeated RAW calls keep that
snapshot. NORMAL mode, `uv_tty_reset_mode` and handle close restore and free it,
including a raw state established before libuv entered RAW. Reset covers handles
in all loops. Descriptor aliases share this owner; independent mode changes
follow libc's snapshot semantics, with the last restoration taking effect.
Console byte input already has no echo or line editing. RAW requires a readable
TTY; IO mode and blocking stream writes remain unsupported. Restoration failures
return their negative UV error. Close aborts on EIO, which includes uncertain
native mode operations, without replaying the operation. Other restoration errors
discard the snapshot and let ordinary descriptor close retire the owner. EBADF
for an already closed or replaced descriptor also leaves a replacement open.
Libc latches failed native releases without replaying them.

`uv/pyxis-native.h` exposes native TTY resize callbacks through
`uv_pyxis_tty_resize_start` and `uv_pyxis_tty_resize_stop`. Start records current
geometry generation and makes the handle active; subsequent generation changes
invoke the callback with new dimensions. Resize shares the TTY's existing wait
interest with byte I/O and consumes no extra slot. Notifications coalesce;
query failures stop observation and report their negative UV error with zero
dimensions. Close withdraws observation. There is no SIGWINCH emulation.

Child launch supports up to three explicit stdin/stdout/stderr slots, directional
CREATE_PIPE, inherited native PIPE/CONSOLE or native/callable-exported FILE
descriptors, and callable-exported STREAM input. Child FILE cursors start at zero;
selected FILE append policy is preserved after a zero-length native APPEND
support/authority probe. Append companions on stdin or non-FILE streams refuse
before launch. Append uses libc's native FILE APPEND, atomic per operation of at
most 4,080 bytes; larger writes may interleave and success does not imply storage
sync. The probe does not reserve later write success. Input outcomes use generic
descriptions; no HTTP observation companion is delegated. Duplex/IPC pipes, extra
slots, buffered inherited input and detached, setuid or setgid flags reject
before launch; Windows-only presentation flags are ignored, as on other
non-Windows platforms.

`options->file` uses shared libpyxis lookup over one captured held `path`
capability list. Bare names have no fallback roots or development catalog;
explicit paths keep their meaning. A supplied `path` resource selects that same list for
lookup and child inheritance, replacing even a malformed inherited `path`
entry. The supplied list is still validated; explicit TEXT `path` refuses.
Absent `PATH` uses the list; a supplied or inherited
`PATH` must equal its colon-joined diagnostic labels for a bare-name launch.
Unsupported textual search refuses instead of being ignored. A bundle child
retains its selected command and full manifest policy, prepared with only the authority this process
holds: memory, clock, threads, random, launcher, pipe, system information, TCP,
UDP, echo, profile and configured bundle sources. The console and input devices
are not offered; the child's streams are the selected stdio. A required grant
this process lacks refuses the spawn. Closing a
process handle releases observation and never terminates the child. EXITED keeps
its signed status; START_FAILED keeps the native packed startup step and call
status. FAULTED/TERMINATED report -1; term_signal is zero for every native result.
The public `exit_reason` is the native PROCESS_* result. Numeric pid fields/accessors return
UV_ENOSYS, already negative; no PID or Unix signal is invented.
Spawn initializes its inactive process handle before fallible preparation, so a
caller must close it after a failed spawn as well as after successful observation.

Inherited TCP factories are attenuated to CONNECT for ordinary children. A
private held TCP factory can satisfy an explicit bundle LISTEN request without
being added to an ordinary child's inventory. An explicit
`uv_pyxis_spawn` TCP resource replaces that declaration source with its selected
rights; a CONNECT-only replacement cannot satisfy LISTEN.

`uv_pyxis_spawn` accepts an array of `uv_pyxis_resource_t` records containing
name, source handle, rights and transport. Ordinary spawn forwards explicitly
exported named capabilities, including native FILEs and controllers, with their
actual masks and transport. The fixed memory/clock/launcher roles retain their
narrowed masks; stdio and bundle-owned context keep their selected handoffs.
A record replaces the inherited forward of the same name, so a caller can
narrow an exported capability, such as a create-only pipe service; other
records add to the selected grants. Empty or duplicate record
names and reserved names `memory`, `clock`, `launcher` and `script` reject before
pipe creation or child publication. Requested rights/transport must be held by
the source. Native capture and startup budgets apply; there is no separate
resource-count policy. CAP/TEXT name collisions refuse before launch; standard
stream choices replace or omit those names in child staging. The array, names and source handles are borrowed until
return and preserved on success and failure. `uv_pyxis_process_selection` borrows
that successful launch's exact selection until process close, including bundle
resources and fixed package provenance. Resource consumers must retain their own
read-only copies to outlive the process handle. Ordinary `uv_spawn` continues to delegate no pipe/create
resource. Neovim selects that resource only for its same-image internal `--embed`
server, allowing both event loops to create their own wake pipes.

`uv_pyxis_spawn_job(loop, process, launcher, command, exit_cb)` selects the exact
native inventory in `struct pyxis_job_options` from the SDK's `pyxis/job.h`.
It adds no inherited resources. The supplied image, argv, grants, selected CAP/TEXT
entries and cwd follow libc's direct-job preparation
and final grant audit, including rejection of known descendant launch authority.
The caller audits provider behavior for the child's lifetime.
The three live descriptor slots can select native FILE, PIPE or CONSOLE streams,
or `-1` for absence. Libc flushes selected buffered outputs before launch; FILE
children start with independent cursors at offset zero and no parent read-ahead.
Source grants and descriptors survive all outcomes. The caller prepares PIPE
endpoints and registers any parent endpoints with the loop separately.

This opt-in reserves one loop wait interest before preparation or publication.
Success transfers the WAIT/TERMINATE observer to the process handle; completion
uses the existing native `exit_reason` and exit callback. Failure leaves an
inactive process handle that still requires `uv_close`. Native uncertain
admission maps to `UV_EIO`; launch errors must not cause automatic replay.
Close releases observation without terminating or waiting for the child.
Ordinary `uv_spawn` and `uv_pyxis_spawn` retain their existing behavior.

`uv_pyxis_process_terminate` requests termination using the process handle's
held native observer and its TERMINATE authority. It does not wait for cleanup,
invent a numeric PID or translate Unix signals. Completion still arrives through
the exit callback and `exit_reason`; close releases observation only.

Filesystem calls run only with a null callback. A supplied callback rejects
before filesystem effects. The adapter exposes ordinary libc operations for
open/close, read/write and explicit offsets, metadata, rename/remove/create,
sync/resize/access, exclusive temporary files and directories, and directory
enumeration.
`uv_fs_realpath` returns libc's proved native scheme spelling in request-owned
storage released by `uv_fs_req_cleanup`. Providers, unavailable identity and
stale or unknown cwd ancestry fail; aliases are not collapsed. `uv_cwd` remains
a descriptive path and does not establish this proof.
Temporary-file creation requires native random and clock grants. Native type and size are always
known on a successful query. `uv_stat_t.stat_valid` carries UV_STAT_DEV_VALID,
UV_STAT_INO_VALID and UV_STAT_MTIME_VALID, aliases of libc's optional validity
bits. A valid zero value differs from unknown. Domain/object tokens retain the
native lifetime rules. Unix owners, permissions, link counts and other extra
metadata remain unknown even though legacy fields are zero-initialized.

## Operating-system queries

Four `uv_os_*` queries answer from native sources; each succeeds or returns a
negative error and never invents a value.

| Query | Result | Native source |
| --- | --- | --- |
| `uv_os_uname` | `sysname` `Pyxis`, `machine` the identity's architecture (`x86_64`), `release` the running kernel's source commit, `version` the kernel name (`Caelum`) | SYSTEM_INFO identity, READ |
| `uv_os_homedir` | `home://` | the caller's `home` root |
| `uv_os_tmpdir` | `tmp://` | the caller's `tmp` root |
| `uv_os_gethostname` | the boot's hostname | SYSTEM_INFO hostname, READ |

Without a SYSTEM_INFO grant `uv_os_uname` still succeeds, because callers index
its fields: `sysname` and `machine` keep their constants and `release` and
`version` are empty. A kernel without an embedded revision leaves `release`
empty too. `uv_os_gethostname` returns `UV_EACCES` without the grant. A program
that was not given the `home` or `tmp` root gets `UV_ENOENT` from the matching
query. The directories are scheme roots, not Unix paths, so joining a name with
a slash produces `home:///name`; callers use the root as a URI prefix.
`uv_os_get_passwd`, `uv_os_get_passwd2`, `uv_os_get_group` and the priority
queries have no native meaning (Pyxis has no users, groups or process
priorities) and return `UV_ENOSYS`.

Workers/pool submissions, thread creation/join, asynchronous filesystem calls,
sockets/address conversion, signals and dynamic module loading return
UV_ENOSYS. File watches (`uv_fs_event_*`, `uv_fs_poll_*`) are omitted. Unsupported fs flags remain distinct so open can reject them before
effects. `UV_FS_O_NOFOLLOW` is libc's `O_NOFOLLOW`, accepted because lookup
follows no links. Single-thread mutexes, recursive depth, once, keys and thread identity
are real native-process state; they do not claim cross-thread synchronization.
Cwd and environment use the shared mutable libc stores. `uv_chdir` performs
native capability traversal; `uv_cwd` returns its tracked scheme description,
which may become stale after external rename. Environment mutation copies strings,
and `uv_os_environ` returns a separately owned enumeration. Spawn captures the
current cwd/environment. A supplied cwd resolves in an independent child context,
and a supplied environment replaces inheritance, including an empty array.
Relative executable and script interpreter paths resolve in the child context;
textual PATH cannot add directory authority, and there is no Unix slash-root
namespace.
`uv_print_all_handles` walks actual loop handles and prints their live state;
`uv_freeaddrinfo` releases the upstream contiguous result allocation, although
address lookup itself remains unsupported.
Peripheral value-only memory/load/metrics introspection and Unicode conversion
symbols are omitted from this bounded library profile. Consumers must adapt
rather than assume those APIs exist or that every upstream libuv facility works.

The platform patch selects `uv/pyxis.h`, preserves negative errors with native
libc errno values, adds stat validity and omits unsupported socket address layout.
Upstream timer, queue/heap helpers, version, data accessors and string-copy code
are reused. `pyxis/watchers.c` retains the upstream loop-watcher implementation
and MIT notice; the other adapter files are original MPL-2.0 material.

Bundle-child launch can deliver held `dns_udp` and read-only `net_registry`
authority when the child's manifest declares them. Packet scope is retained;
this does not add network authority to applications that hold none.
