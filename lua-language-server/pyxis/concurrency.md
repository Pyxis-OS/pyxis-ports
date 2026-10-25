# Private LuaLS concurrency

`register_concurrency` supplies the LuaLS-used subset of bee.thread/channel/
select/epoll, plus private bee.stdio. These are program adapters, not new SDK
or kernel interfaces. Each worker uses the same embedded Lua ABI through
`new_state`; the initial thread requests stop and joins every worker before
closing its Lua state and removing the instance directory.

Channels copy only admitted nil/boolean/number/string/raw-table graphs through
lua-seri. Cycles/shared tables are retained; arbitrary `__pairs`, userdata and
closures are refused. Each private task/reply channel has one producer in the
LuaLS worker protocol. Its `fd()` returns a typed readiness object; selectors
only support channel READ readiness, with actual queue predicates and a shared
condition variable. Publication, capacity release and stop use the same mutex.
Capacity exhaustion waits for the consumer or cancellation; an oversized or
unserializable message fails the service rather than disappearing. The selected
bee patch checks C++ serializer allocations and releases scratch blocks on both
allocation exceptions and Lua's C++ error transfer. Allocation failure returns
through the protected pack call, releasing the admission before service stop.

Provisional implementation limits, pending public-fixture qualification:

- 8 workers, 32 named channels, 256 items/channel and 4096 admitted items total;
- 16 MiB conservative encoded message bound, 64 MiB/channel and 256 MiB total
  byte admission, including in-flight serialization;
- serialization reserves three times its conservative encoded size before
  copying, covering lua-seri blocks and flattening; publication reduces the
  charge to the actual owned buffer size, and pop releases it after unpack;
- at most 65,536 distinct tables, 1,048,576 visited values and depth 128 per
  preflight graph; worker bootstrap source at most 64 KiB, with no arguments;
- 8 KiB LSP header, 4 MiB body/output content; pending Lua tasks, queued LSP
  methods and active method coroutines each have a 1024-item limit.

Stdio requires native PIPE descriptors for stdin/stdout; unsupported bindings
fail before workers start. Data moves through the public descriptor try APIs;
borrowed native handles are used only for `wait_many`. An owned stop pipe wakes
blocked input/output readiness waits even when the editor keeps stdin open.
The timer module supplies the actual next timer deadline for condition waiting;
there is no busy loop, empty network selector or permanent polling timer job.

Shutdown still acknowledges and keeps input live. Exit requests stop and raises
a private sentinel through protected Lua calls. Owned xpcall/coroutine boundaries
preserve that identity. VM hooks interrupt Lua work and disable themselves when
they raise the sentinel, preventing repeated exceptions in error handlers.
`is_stop_error` lets main distinguish expected cancellation from an actual error
that races with exit. The first nonzero exit status is retained. Malformed or
truncated framing, unexpected EOF and I/O failures request failure; after reading
an exit notification, the reader retires that task without an extra EOF read.

Forced stop may cancel queued work. `stop_and_join` waits for every worker before
releasing channel buffers and wake handles. Worker states close through the
shared `close_state` helper; main owns final state and instance cleanup. No PID,
lock-file claim, socketpair, select, epoll, signal or CPU-time emulation is used.

Task 1 qualification includes the full static build, pinned Lua 5.5 syntax
checks and 4-CPU QEMU native-pipe initialize/documentSymbol/shutdown/exit, with
stdin left open and instance cleanup observed. Missing required grants refuse
launch. The parent task record carries exact artifacts and configuration.
Peak memory, large projects, decoded semantic tokens, API-aware completion and
native qualification remain later checks; no allocation-failure injection was
performed.
