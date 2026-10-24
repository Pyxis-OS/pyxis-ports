# Manual consumer

Build against an installed spdlog development prefix, with no source-tree
headers or archive. From the ports root:

```sh
make -j4 -f spdlog/manual/Makefile SDK=/path/to/sdk \
  SPDLOG=/path/to/spdlog/stage/dev RECIPE="$PWD/spdlog" \
  BUILD="$PWD/build/spdlog-manual"
```

In a later interactive Pyxis run, grant stdout/stderr, writable public scratch
directory access, clocks and threads, then run the P1F consumer with a native
directory argument such as `tmp://spdlog-public/missing/nested`. It shows `%P`
rejection, checked clock/calendar calls, URI root-parent spelling, bundled fmt
number formatting, stdout plain/color, basic file, actual rotation, and a real
async queue followed by shutdown. Inspect basic.log, rotate.log and numbered
rotation files, and async.log. Repeat with a directory root such as `tmp://`.
File and console calls retain the upstream reporting limits documented in the
recipe README; the final output does not claim that all output was delivered.

Try an unavailable native root and denied creation/rename/removal. A malformed
TZ or denied clock must report a real failure rather than a fabricated date.
The direct clock/calendar calls propagate exceptions to the program's stderr
handler. This source uses public strings only. Building it establishes link
closure; no runtime behavior is claimed until an actual run is recorded.

For the denied-clock fixture, run `spdlog-manual --clock-errors` without clock
authority. It requires two error-handler calls, one each for raw and formatted
messages, and checks that the raw message's source location reaches the handler.
A direct clock-helper call must still throw. The explicitly authored timestamp
overload must add no current-clock request. This mode uses an empty logger with
no sinks; it requires no filesystem or new worker thread. Its result is only
meaningful when the launch actually denies clock authority.
