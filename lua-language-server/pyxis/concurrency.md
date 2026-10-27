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
unserializable message fails the service rather than disappearing. The
checksum-pinned bee serializer is used without generic allocation changes. Allocation-failure and Lua-error scratch cleanup remain unqualified.

Task 3 sized these program-private limits from three public LuaLS source
workloads (248 files, 1,665,517 bytes, plus the held Neovim library) and three
functional client instances. The source workload includes 147,425-byte
`parser/compile.lua`, decoded semantic tokens and a 64-request symbol burst.
A private measurement build counted admitted work under the existing mutex;
all six instances completed with no admission waits/rejections or counter
underflows. Measurement hooks are not shipped.

| Admission | Initial measured maximum | Selected bound / margin |
| --- | ---: | --- |
| Conservative serialized message | 423,872 B | 8 MiB, 19.8×; also fits a full-text 4 MiB LSP payload plus bounded metadata |
| Accounted bytes/channel | 1,271,633 B | 32 MiB; a full 3×8 MiB reservation must fit even when empty |
| Accounted bytes total | 2,302,689 B | 64 MiB; room for two maximum-size reservations and queued work |
| Accounted items/channel / total | 70 / 70 | 512 / 1024, 7.3× / 14.6× |
| Graph tables / visited values / depth | 94 / 694 / 8 | 8192 / 65,536 / 32; traversal guards, not workload-size limits |
| LSP input / output body | 152,808 / 720,906 B | Upstream framing; no added application byte cap |
| LSP header | 26 B | Upstream parsing; no added header cap |
| Pending Lua tasks per pool | 215 | Upstream Lua queue |
| Queued / active LSP methods | 70 / 79 | Upstream Lua queues |
| Workers / named channels | 4 / 8 | 8 / 32; ordinary profile still starts four workers |
| Worker bootstrap | 260 B | 4 KiB, 15.8×; no arguments |

Serialization reserves three times its conservative encoded size before
copying, covering lua-seri blocks and flattening. Publication reduces the
charge to the actual owned buffer; pop releases it after unpack. The maximum
whole reservation must fit both empty-channel and total budgets, checked at
compile time, so an otherwise admissible item cannot wait forever for space
which can never exist. The graph visited-value bound also bounds raw array
length; cyclic/shared graphs retain identity. Graph guards are deliberately
larger than a multiple of the modest fixture peaks: generated files and large
configuration arrays can have many small nodes while using little byte budget.
Visited values include keys and values, not just array entries. These limits
bound traversal/preflight scratch independently of copied-byte admission; a
sufficiently dense graph can still reach them below the byte ceiling.

These are tuning choices for the measured profile, not ABI contracts or
universal project limits. A well-framed JSON message still has to fit the
independent graph and byte limits; framing alone does not promise admission.
Global byte admission intentionally backpressures a third simultaneous maximum
reservation rather than provisioning four such copies. **Queue admission is not a whole-process memory
budget.** Four read-only debugger checkpoints on a separate corpus run observed
five native tasks and up to 1,143,627,776 backed bytes (about 1.07 GiB), including
Lua heaps, code and stacks. That is an observed high-water mark, not an exact
peak, allocator-failure check or leak proof. Large workspaces need corresponding
memory authority; overflow and genuine allocation failure remain visible.

Review qualification with the wider graph guards also passed a generated
10,000-entry Lua table (218,945 B), returning 10,001 symbols and 50,015 semantic
integers, plus a typed hover for the last field. A real client
`diagnostics.globals` array of 10,000 names applied in full through the reader
worker: at least 20,001 table/key/value visits before its envelope. This
exercises structural copying; raw source text alone is one string. The largest
held Neovim runtime Lua file, 423,854-byte `vimfn.gen.lua`, also opened and
returned symbols. A fresh ordinary feature/cleanup pass succeeded. These are
functional checks, not a new instrumented maximum or performance measurement.

Outgoing LSP result tables do not cross a worker channel. Semantic-token,
symbol and completion handlers in `script/provider/provider.lua` return their
results in the service VM. `script/proto/proto.lua` runs handlers through
`await.call` (a same-VM coroutine), then `response` → `send` →
`script/jsonrpc.lua:encode` produces the framed JSON string. Native
`stdio_write` in `pyxis/concurrency.cpp` writes that string directly to stdout;
there is no writer channel or added output byte cap. Thus the 50,015 semantic
integers above do not individually visit the channel graph guard.
Incoming editor messages, including replies to server requests, are decoded in
`script/brave/work.lua:loadProtoByStdio` and pushed as tables. Those and internal
worker task/reply graphs remain subject to the graph limits; the 10,000-name
configuration check above exercised that incoming path.

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
that races with exit. The first nonzero exit status is retained. Upstream
framing reports parse errors; unexpected EOF and native I/O failures request
failure. After reading an exit notification, the reader retires that task without an extra EOF read.

Forced stop may cancel queued work. `stop_and_join` waits for every worker before
releasing channel buffers and wake handles. Worker states close through the
shared `close_state` helper; main owns final state and instance cleanup. No PID,
lock-file claim, socketpair, select, epoll, signal or CPU-time emulation is used.

Task 1 qualification includes the full static build, pinned Lua 5.5 syntax
checks and 4-CPU QEMU native-pipe initialize/documentSymbol/shutdown/exit, with
stdin left open and instance cleanup observed. Missing required grants refuse
launch. The parent task record carries exact artifacts and configuration.
Task 3 also exercises decoded semantic tokens, typed API completion, diagnostics
and metadata navigation in QEMU. Chronium's attended vela HWP PXE editor check
passed hover, typed completion, semantic highlighting and diagnostics. Normal
editor exit removed its state leaf, verified separately over Remote. Native
allocation-failure/forced-stop cases and a heap-leak proof remain unqualified.

The measurements above describe the original Task 1/3 profile. The patch
cleanup restored upstream Lua queues, framing, configuration, plugins and cache
behavior; it removed the serializer allocation patch. Those earlier runs do not
qualify the changed profile. Native copied-buffer admission, cancellation and
join ownership are retained.
