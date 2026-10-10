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
TTY RAW mode holds a native console passthrough grant, making Ctrl+C ordinary
input, until NORMAL mode, `uv_tty_reset_mode` or handle close. Console byte input
already has no echo or line editing. RAW requires a readable TTY; IO mode and
blocking stream writes remain unsupported. Reset withdraws every RAW reference
owned by this adapter, including handles in other loops.

`uv/pyxis-native.h` exposes native TTY resize callbacks through
`uv_pyxis_tty_resize_start` and `uv_pyxis_tty_resize_stop`. Start records current
geometry generation and makes the handle active; subsequent generation changes
invoke the callback with new dimensions. Resize shares the TTY's existing wait
interest with byte I/O and consumes no extra slot. Notifications coalesce;
query failures stop observation and report their negative UV error with zero
dimensions. Close withdraws observation. There is no SIGWINCH emulation.

Child launch supports up to three explicit stdin/stdout/stderr slots, directional
CREATE_PIPE, and inherited native PIPE/CONSOLE descriptors. Duplex/IPC pipes,
extra slots, FILE cursor inheritance, buffered inherited pipe input, process
flags and child bundle paths reject before launch. Closing a
process handle releases observation and never terminates the child. EXITED keeps
its signed status; FAULTED/TERMINATED report -1 and term_signal zero. The public
`exit_reason` is the native PROCESS_* result. Numeric pid fields/accessors return
UV_ENOSYS, already negative; no PID or Unix signal is invented.
Spawn initializes its inactive process handle before fallible preparation, so a
caller must close it after a failed spawn as well as after successful observation.

`uv_pyxis_spawn` accepts an array of `uv_pyxis_resource_t` records containing
name, source handle, rights and transport. These explicitly selected resources
augment the ordinary attenuated memory/clock/launcher grants. Empty or duplicate
names and reserved names `memory`, `clock`, `launcher` and `script` reject before
pipe creation or child publication. Requested rights/transport must be held by
the source. Native capture and startup budgets apply; there is no separate
resource-count policy. The array, names and source handles are borrowed until
return and preserved on success and failure. No extra reference survives launch
capture in the parent. Ordinary `uv_spawn` continues to delegate no pipe/create
resource. Neovim selects that resource only for its same-image internal `--embed`
server, allowing both event loops to create their own wake pipes.

`uv_pyxis_process_terminate` requests termination using the process handle's
held native observer and its TERMINATE authority. It does not wait for cleanup,
invent a numeric PID or translate Unix signals. Completion still arrives through
the exit callback and `exit_reason`; close releases observation only.

Filesystem calls run only with a null callback. A supplied callback rejects
before filesystem effects. The adapter exposes ordinary libc operations for
open/close, read/write and explicit offsets, metadata, rename/remove/create,
sync/resize/access, exclusive temporary files and directory enumeration.
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

Workers/pool submissions, thread creation/join, asynchronous filesystem calls,
sockets/address conversion, signals and dynamic module loading return
UV_ENOSYS. File watches (`uv_fs_event_*`, `uv_fs_poll_*`) are omitted. Unsupported fs flags remain distinct so open can reject them before
effects. Single-thread mutexes, recursive depth, once, keys and thread identity
are real native-process state; they do not claim cross-thread synchronization.
Cwd and environment use the shared mutable libc stores. `uv_chdir` performs
native capability traversal; `uv_cwd` returns its tracked scheme description,
which may become stale after external rename. Environment mutation copies strings,
and `uv_os_environ` returns a separately owned enumeration. Spawn captures the
current cwd/environment. A supplied cwd resolves in an independent child context,
and a supplied environment replaces inheritance, including an empty array.
Relative executable and script interpreter paths resolve in the child context;
there is no colon-separated PATH search or Unix slash-root namespace.
Peripheral value-only memory/load/metrics introspection and Unicode conversion
symbols are omitted from this bounded library profile. Consumers must adapt
rather than assume those APIs exist or that every upstream libuv facility works.

The platform patch selects `uv/pyxis.h`, preserves negative errors with native
libc errno values, adds stat validity and omits unsupported socket address layout.
Upstream timer, queue/heap helpers, version, data accessors and string-copy code
are reused. `pyxis/watchers.c` retains the upstream loop-watcher implementation
and MIT notice; the other adapter files are original MPL-2.0 material.
