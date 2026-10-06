# TCC: Pyxis compiler

The recipe builds a **guest** `bin/tcc.pxe`, a **host-running** x86-64 Pyxis
compiler and a **target** support archive. Both compilers preprocess, compile
ELF objects and statically link native P1F executables. Pyxis packages the guest
compiler and target SDK in its normal image; GCC remains the default compiler
for maintained userspace and the OS.

## Build and use

With the Pyxis cross-toolchain on PATH and an exported SDK:

```sh
lua build.lua tcc --sdk /path/to/pyxis/build/sdk
build/tcc/stage/host/bin/x86_64-pyxis-tcc -c /path/to/pyxis/userspace/mandelbrot/main.c -o mandelbrot.o
```

The recipe also needs host `cc` and GNU Make. Only the host compiler links the
host C runtime. The guest compiler, target support and application objects use
the Pyxis GCC/SDK, with the guest ELF retained at `build/tcc/build/guest/tcc.elf`
for GDB. The runner's usual `--work` and `--cross-prefix` options apply.

The compiler records the selected SDK sysroot and stage directory. Rebuild if
moving the SDK; `-B` can select a relocated compiler-header/support directory.
Its default includes are SDK `usr/include`, then `stage/lib/tcc/include` with
TCC's own `stddef.h`, `stdarg.h`, `stdbool.h` and `float.h`. Do not supply GCC's
private headers. Host CPATH/C_INCLUDE_PATH/LIBRARY_PATH are intentionally ignored;
use explicit `-I`/`-L` options. `-print-search-dirs` shows the configured paths.

The exported Pyxis SDK includes target libgcc, so host-running TCC can link
against the same runtime without extra library paths:

```sh
build/tcc/stage/host/bin/x86_64-pyxis-tcc \
  /path/to/pyxis/userspace/mandelbrot/main.c -o mandelbrot.pxe
```

This writes P1F directly. There is no ELF executable/converter step. ELF objects
remain interoperable with the SDK's GNU linker when an ELF with debug symbols
is needed; P1F itself contains only loadable segments and the entry point.

## Target and runtime contract

- LP64/System V x86-64, `__pyxis__`, SSE2 float/double and 80-bit x87 long double.
  No Linux/Unix target predefines, host include paths or Linux startup objects.
  The target is freestanding and uses the SDK's integer types, size and pointer
  types. Explicit machine options are limited to `-m64` and `-msse`.
- ELF relocatable objects have a non-executable `.note.GNU-stack`. FP register
  transfers reserve temporary stack storage rather than using the red zone;
  LEA adjusts RSP without destroying pending integer condition flags.
- `libtcc1.a` is built with the Pyxis GCC/SDK from `libtcc1.c`, `va_list.c` and
  `builtin.c`. It supplies unsigned-integer-to-FP conversions, `__fixxfdi`,
  `__va_arg` and TCC's bit-operation builtins. It does not import startup,
  dynamic-loader, backtrace, coverage, bounds-checking or atomic runtime code.
- libgcc owns `__fixunssfdi`, `__fixunsdfdi` and `__fixunsxfdi`. The Pyxis patch
  excludes their TCC copies and the unused signed float/double helpers;
  `__fixxfdi` uses the libgcc long-double helper. The two archives have no
  overlapping defined symbols with the current Pyxis GCC 16.2.0 toolchain.
- Default linking adds `crt0.o` before application inputs, then rescans libc,
  libterm, libpyxis, libtcc1 and libgcc until no further archive members are
  extracted. This resolves dependencies back into earlier runtime libraries.
  Explicit archives and `-l` inputs retain ordinary command-line order; place
  them after their users. `-nostdlib` omits both startup and default libraries.

First-party implementation sources remain GNU C23. This is not a promise that
TCC accepts all of them: for inspection, libc format/printf compile with
`-include stdbool.h`; the line editor's `[[fallthrough]]` is unsupported. No
first-party source is rewritten to make this port's validation compile.

## Native executable output

Executable output defaults to static P1F at a fixed base of `0x400000`, with
`_start` as the entry point and `a.pxe` as the default filename. The linker
retains its existing section sorting, symbol resolution, GOT handling and
relocations. Permission transitions are page-aligned before relocation; the
writer emits the SDK's unchanged P1F header/segments, zeroes interior gaps and
omits zero-filled tails. It rejects writable/executable mappings and checks
layout arithmetic against the linker's signed-int offset limit and P1F's user
address bounds. Sections with stronger alignment retain it.

Unresolved strong symbols fail the link; undefined weak symbols retain ELF's
zero value. TLS, indirect functions, dynamic relocations and constructor/
destructor arrays are unsupported and diagnosed. Shared objects, linker scripts,
PIE, JIT/run and arbitrary `-Wl` controls are not accepted. `-static` explicitly
selects the already-default policy. `-L`/`-l` select static archives through the
existing directory grants. `-c` still writes ELF relocatable objects; `-g` can
retain their debug information, but executables carry no debug sections.

Output uses create/truncate streams and checks writes and close. An I/O failure
may leave a partial file; no filesystem replacement/removal facility is invented
for this port.

## Guest SDK and use

The normal Pyxis image installs `bin/tcc.pxe` at `bin://tcc.pxe`. The SDK lives
under read-only `boot://sdk`: shared headers in `usr/include`, `crt0.o` and the
libc/libterm/libpyxis/libgcc archives in `usr/lib`, and this recipe's `lib/tcc`
with libtcc1 and its four compiler-private headers. GCC builtin headers and host
compiler/converter executables are not guest inputs.

The recipe stages TCC's source pin and ordered patch copies under `share/tcc`,
and its license and runtime source notices under `share/licenses/tcc`. Pyxis
packages those with the SDK's library/toolchain provenance and a manifest naming
the Pyxis, userland and ports revisions. The recipe never modifies the SDK.

`GUEST_SYSROOT` and `GUEST_TCCDIR` select the default prefixes at build time;
use a fresh build directory when changing them. `-I`, `-isystem`, `-nostdinc`
and `-B` can select explicitly granted headers; `-L` selects library directories.
The paths above are the normal-image contract. Keep source and output in
writable `home://`.

From the shell, for example:

```text
tcc boot://src/cat/main.c -o home://cat.pxe
./cat.pxe boot://share/hello.txt
tcc -c boot://src/cat/main.c -o home://cat.o
tcc -E -P boot://src/shell/main.c -o home://shell.i
tcc -g -c boot://src/mandelbrot/main.c -o home://mandelbrot.o
```

The guest driver accepts:

- Default P1F linking, `-L path`, `-lname`, `-static`, `-nostdlib`.
- `-E`, `-c`, `-o path`, source/object/archive paths and `@response-file`.
  Multiple `-c` inputs produce separate objects and cannot share one explicit `-o`. Preprocessing
  defaults to stdout; `-o -` also selects stdout for `-E`.
- `-I`, `-isystem`, `-include`, `-D`, `-U`, `-nostdinc`, `-B`, `-P`, `-dD`, `-dM`.
- `-g`, `-g0`, `-g1`, `-g2`, `-gdwarf`; the guest defaults to DWARF 5 when debug
  output is requested. Its compilation-directory metadata uses the optional
  startup description, never as authority; overlong descriptions are diagnosed.
- `-std=c99`, `-std=gnu99`, `-std=c11`, `-std=gnu11`; `-x c`, `assembler`,
  `assembler-with-cpp` or `none`. These retain TCC's dialect and extensions,
  not strict conformance checking or full C23 support.
- `-w`, the warnings/flags listed by `-hh`, `-m64`, `-msse`, `-v`, `-vv`,
  `--version`, `-h`, `-hh`, `-print-search-dirs`, `-dumpmachine`, `-dumpversion`.

Other options report an error. This includes relocatable merging (`-r`),
arbitrary linker options (`-Wl`), dependency generation, archive creation, runtime/JIT
execution, coverage, backtraces, bounds checking and compiler subprocess dispatch.
`-bench` reports compilation time and throughput using the monotonic clock.
Unsupported options cannot be silently overridden by a later `-c` or `-E`.

The process needs an `output` console with write rights and `memory` with manage
rights; `-bench` and calendar macros also need `clock` with read rights.
Stdin compilation also needs an `input` console with read rights. Sources
and headers need readable file/directory grants, and output needs a writable
directory with lookup/create rights and writable files. Relative paths require
the inherited directory chain; rooted paths use the named startup grants. The
existing shell supplies these. TCC needs no launcher or process-management grant.

The process stack stays 64 KiB. GCC stack-usage output reports a largest fixed
compiler frame of 2,720 bytes for this build; recursive parsing is not bounded
by that single-frame figure. Existing cat, shell preprocessing and Mandelbrot
work in the guest. Compiling the existing line editor demonstrates an ordinary
unsupported-C23 diagnostic and stream/process cleanup, not a new test source.

## Native stream and path adaptation

The selected source, response-file and ELF object/archive readers now use
`FILE *` streams. Source streams belong to the existing `BufferedFile` chain,
which closes nested includes during normal completion and compile-error
unwinding. Memory inputs have no stream; stdin is borrowed. Reads distinguish
errors from EOF, binary seeks check their offsets, and object/archive read
failures release temporary data. Output uses create/truncate streams and checks
writes and close; a failed output can remain partial because Pyxis has no
replacement/removal operation yet. Shared objects and linker scripts are
rejected by the Pyxis target.

When the compiler itself is built for Pyxis:

- Relative names use the inherited working directory through libc streams.
  A leading `scheme://` selects a startup root. Components are not normalized
  before traversal; `missing/..` must still fail at `missing`.
- Include/library path APIs take one path per call; colons are preserved.
  Repeat `-I`/`-L` for multiple paths.
  Configured include defaults are separate `{R}/usr/include`
  and `{B}/include` entries; library defaults are `{R}/usr/lib` and `{B}`.
  The guest recipe supplies the packaged SDK prefixes.
- Quoted includes first search beside the source file. Rooted includes are
  opened directly, without falling back to search directories. `#include_next`
  requires a relative name. Paths exceeding TCC's existing filename buffers
  produce an error rather than being truncated.
- `#pragma once` reports an error: native file handles have no identity query,
  and aliases cannot be compared reliably using path strings. Use ordinary
  include guards until that filesystem operation exists.
- `__DATE__` and `__TIME__` use UTC, with C's month-name date and 24-hour
  time spelling. A missing clock or date outside years 0000–9999 is an error;
  no date is invented. Explicit user macro definitions still work normally.

Host-running TCC retains host pathname, clock and `#pragma once` behavior.
The guest compiler builds against the real Pyxis SDK. It excludes the upstream
host tools and uses the existing native startup metadata for
debug information. No fd compatibility layer or new libc/kernel API is added.

## Source and local changes

Pinned upstream: [TinyCC 3dc99dbc82f8e07308c5d398136803e62f9676df](https://github.com/TinyCC/tinycc/tree/3dc99dbc82f8e07308c5d398136803e62f9676df)
(`0.9.28rc`). Metadata lists seven ordered patches: the Pyxis object target,
runtime symbol ownership, FP scratch allocation, native streams/paths, the
guest driver, native P1F linking, and guest clock integration. The P1F writer consumes the SDK format
header; the host build uses a quoted include path so it does not import target
libc headers. The clock patch keeps host clock behavior and upstream floating-point
benchmark formatting; the guest uses libpyxis monotonic reads and libc UTC
calendar conversion. Calendar years are padded to four digits. Its existing unsigned millisecond counter wraps after
roughly 49 days, so benchmark intervals must be shorter than that. The stream patch uses existing bounded formatting for internal
names and removes an unnecessary `inttypes.h` dependency. Upstream algorithms
and formatting are otherwise retained; the target remains x86-64 Pyxis, with
ELF objects and P1F executables.

Upstream `COPYING` contains LGPL 2.1. `lib/libtcc1.c` separately states GPL 2 or
later with its explicit unlimited-linking exception; retain that notice as well.
The stage includes `COPYING` and the three selected runtime source files under
`share/licenses/tcc`, preserving their individual notices. Compiler and runtime
licensing must not be described as one blanket license.
