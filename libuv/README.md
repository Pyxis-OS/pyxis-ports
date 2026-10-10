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
I/O uses libc's descriptor-aware try operations, preserving read-ahead. Submitted
writes copy buffer descriptors and borrow their bytes until one callback. Close
cancels pending writes before the close callback; successful queued completions
keep their result. Pipe shutdown closes its write descriptor after queued writes.
Console raw mode and blocking stream writes are unsupported.

Child launch supports up to three explicit stdin/stdout/stderr slots, directional
CREATE_PIPE, and inherited native PIPE/CONSOLE descriptors. Duplex/IPC pipes,
extra slots, FILE cursor inheritance, buffered inherited pipe input, process
flags, cwd selection and child bundle paths reject before launch. Closing a
process handle releases observation and never terminates the child. EXITED keeps
its signed status; FAULTED/TERMINATED report -1 and term_signal zero. The public
`exit_reason` is the native PROCESS_* result. Numeric pid fields/accessors return
UV_ENOSYS, already negative; no PID or Unix signal is invented.
Spawn initializes its inactive process handle before fallible preparation, so a
caller must close it after a failed spawn as well as after successful observation.

Filesystem calls run only with a null callback. A supplied callback rejects
before filesystem effects. The adapter exposes ordinary libc operations for
open/close, read/write and explicit offsets, metadata, rename/remove/create,
sync/resize/access and directory enumeration. Native type and size are always
known on a successful query. `uv_stat_t.stat_valid` carries UV_STAT_DEV_VALID,
UV_STAT_INO_VALID and UV_STAT_MTIME_VALID, aliases of libc's optional validity
bits. A valid zero value differs from unknown. Domain/object tokens retain the
native lifetime rules. Unix owners, permissions, link counts and other extra
metadata remain unknown even though legacy fields are zero-initialized.

Workers/pool submissions, thread creation/join, asynchronous filesystem calls,
sockets/address conversion, watches, signals and dynamic module loading return
UV_ENOSYS. Unsupported fs flags remain distinct so open can reject them before
effects. Single-thread mutexes, recursive depth, once, keys and thread identity
are real native-process state; they do not claim cross-thread synchronization.
Cwd/environment are immutable startup observations; mutation is unsupported.
Peripheral value-only memory/load/metrics introspection and Unicode conversion
symbols are omitted from this bounded library profile. Consumers must adapt
rather than assume those APIs exist or that every upstream libuv facility works.

The platform patch selects `uv/pyxis.h`, preserves negative errors with native
libc errno values, adds stat validity and omits unsupported socket address layout.
Upstream timer, queue/heap helpers, version, data accessors and string-copy code
are reused. `pyxis/watchers.c` retains the upstream loop-watcher implementation
and MIT notice; the other adapter files are original MPL-2.0 material.
