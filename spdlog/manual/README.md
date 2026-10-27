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
rejection, upstream clock/calendar calls, URI root-parent spelling, bundled fmt
number formatting, stdout plain/color, basic file, actual rotation, and a real
async queue followed by shutdown. Inspect basic.log, rotate.log and numbered
rotation files, and async.log. Repeat with a directory root such as `tmp://`.
File and console calls retain the upstream reporting limits documented in the
recipe README; the final output does not claim that all output was delivered.
