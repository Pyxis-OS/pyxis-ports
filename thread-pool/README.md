# thread-pool

BS thread-pool 4.1.0, pinned to commit
`097aa718f25d44315cadb80b407144ad455ee4f9`, exports `BS_thread_pool.hpp` and
its optional `BS_thread_pool_utils.hpp` companion. The recipe downloads only
the checksum-pinned archive from the owner's raw-github proxy.

Use C++17 or newer and `pkg-config --cflags --libs thread-pool`. The metadata
names the merged `sdk://` view and depends on the real Pyxis C++ runtime.
This header package supplies no implementation archive. Workers execute on
real C++ threads; queueing is not replaced by synchronous execution.

Select optional upstream feature macros before the first include and keep
them consistent throughout each consumer. This package imposes none.
Libultraship selects priority and pause; both affect class definitions.
Higher priorities run first, with no FIFO promise for ties. Pause retains
queued work; a paused wait drains running work only. Futures can deadlock
when every worker waits for more work queued to the same pool.

The recorded local patch keeps the completion predicate under the existing
mutex, correcting upstream concurrent state reads. Chronium accepted it on
2026-10-24. Remove it when the selected upstream source contains the fix.
Notices, provenance and complete recipe/patch source travel under
`dev/share/licenses/thread-pool` and `dev/share/thread-pool`.
