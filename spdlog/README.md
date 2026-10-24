# spdlog

spdlog 1.16.0, pinned to `486b55554f11c9cccc913e11a87085b2a91f706f`,
builds as a reusable static library with its bundled fmt 12.0.0. The recipe
downloads only the owner-cached commit archive and verifies its SHA-256.
It uses the native SDK's libc++, threads, TLS and exceptions. There is no
dependency on the separately packaged fmt 12.2.0.

```sh
lua build.lua spdlog --sdk /path/to/pyxis/build/sdk
```

The development prefix contains `lib/libspdlog.a`, all upstream spdlog headers
including bundled fmt, the `spdlog::spdlog` CMake target, and `spdlog.pc` with
`sdk://` paths. CMake consumers use `find_package(spdlog CONFIG REQUIRED)` and
link `spdlog::spdlog`; direct consumers need the prefix's include directory,
`-DSPDLOG_COMPILED_LIB`, and the archive before the SDK C++ runtime libraries.
Keep matching headers and archive together. Do not mix bundled fmt headers or
symbols with another fmt version in one program.

The native patch rejects `%P` when a pattern is configured, with a clear
`spdlog_ex`; it exports no process-ID accessor. Clock reads use libc's real
realtime clock and throw on failure. Calendar conversions check libc's result
and throw on invalid TZ, unavailable zone files or an unrepresentable calendar.
Convenience calendar calls use the checked clock, preserving valid pre-epoch
timestamps. There is no invented PID, epoch or fallback date. The native
default error handler writes a timestamp-free stderr message, without querying
the failed clock or calendar again. The ordinary templated logging path handles
these errors through spdlog's error handler; direct `details::os` calls and
non-format string-view logging overloads can propagate `spdlog_ex` to callers.
Set a custom error handler when the application needs explicit error accounting.

Recursive log-directory creation preserves complete native `name://` roots,
including a filename directly under a root. FILE access, stat, mkdir, rename,
remove and sync use libc. Async, stdout, ANSI color, basic-file and rotating-file
sinks remain enabled. The application supplies directory/stream, clock and
thread authority; missing rights produce real errors. Retry sleeps require
native clock sleep authority, as specified by the SDK C++ runtime.

Upstream delivery limits remain: stdout/color sinks ignore write/flush results;
the default error handler is best-effort stderr output; file close errors are
not reported; `flush` drains FILE output without storage sync. File append is
not atomic between independent writers. Rotation removes/renames files in order
and can truncate the active log after repeated rename failure; it is not a
crash-safe transaction. No persistence guarantee follows from a successful log
call or shutdown. Examples, benchmarks, shared libraries and upstream tests are
not built.

`share/licenses/spdlog` retains spdlog/fmt MIT notices and the Pyxis MPL notice;
`share/spdlog/source.txt` records provenance. The corresponding recipe and
patch sources are in `share/sources/spdlog`. Preserve them with the development
package. The manual consumer under `manual/` supports an independent installed
prefix build and later public-data QEMU checks; building it is not runtime
qualification.
