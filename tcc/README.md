# TCC: Pyxis object compiler

The recipe builds a **guest** `bin/tcc.pxe`, a **host-running** x86-64 Pyxis
compiler and a **target** support archive. Guest TCC supports preprocessing and
ELF object compilation, not executable linking yet. It is not installed in the
normal boot image at this stage; GCC remains the default compiler.

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

Use the existing SDK link path for objects. For example, with `PYXIS_SDK` and
`TCC_STAGE` set to the absolute SDK and `build/tcc/stage` paths:

```sh
x86_64-unknown-pyxis-gcc --sysroot="$PYXIS_SDK/sysroot" \
  -nostdlib -static -no-pie \
  -Wl,-T,"$PYXIS_SDK/sysroot/usr/lib/pyxis.ld" \
  -Wl,--build-id=none -Wl,-z,max-page-size=0x1000 \
  -o mandelbrot.elf "$PYXIS_SDK/sysroot/usr/lib/crt0.o" mandelbrot.o \
  -Wl,--start-group \
  "$PYXIS_SDK/sysroot/usr/lib/libc.a" \
  "$PYXIS_SDK/sysroot/usr/lib/libterm.a" \
  "$PYXIS_SDK/sysroot/usr/lib/libpyxis.a" \
  "$TCC_STAGE/lib/tcc/libtcc1.a" -lgcc -Wl,--end-group
"$PYXIS_SDK/bin/elf2pxe" --format p1f -o mandelbrot.pxe mandelbrot.elf
```

Keep the ELF for GDB. `-gdwarf` selects DWARF debug information; upstream `-g`
defaults to STABS, which current GDB deprecates. This GNU linker/converter path
is an intermediate development path, not the planned guest compiler workflow.

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
- Link `crt0.o` and application objects before the archive group shown above.
  Group rescanning resolves libc/libpyxis cycles and support dependencies on
  libc and libgcc. Future native TCC linking must preserve that behavior; merely
  appending one pass over the libraries is insufficient.

First-party implementation sources remain GNU C23. This is not a promise that
TCC accepts all of them: for inspection, libc format/printf compile with
`-include stdbool.h`; the line editor's `[[fallthrough]]` is unsupported. No
first-party source is rewritten to make this port's validation compile.

P1F output and permanent guest SDK packaging remain later tasks.
Executable/shared/in-memory output is rejected at this stage;
there is no Linux ELF executable fallback. Only preprocessing/object compilation
and the explicit GNU link path above are supported here, not every upstream
command-line option or language extension.

## Guest use and staging

For manual image staging, add `stage/bin/tcc.pxe` to `app://tcc.pxe`, the SDK's
`sysroot/usr/include` beneath `app://sdk/usr/include`, and `stage/lib/tcc/include`
beneath `app://sdk/lib/tcc/include`. Add existing application sources separately.
These are temporary staging paths, not a new normal-image packaging rule.
The recipe's `GUEST_SYSROOT` and `GUEST_TCCDIR` settings select the prefixes at
build time; use a fresh build directory when changing them. `-I`, `-isystem`,
`-nostdinc` and `-B` can select explicitly supplied headers at runtime.

From the shell, for example:

```text
tcc -c app://src/cat/main.c -o home://cat.o
tcc -E -P app://src/shell/main.c -o home://shell.i
tcc -g -c app://src/mandelbrot/main.c -o home://mandelbrot.o
```

The guest driver accepts:

- `-E`, `-c`, `-o path`, source paths and `@response-file`. Multiple `-c` inputs
  produce separate objects and cannot share one explicit `-o`. Preprocessing
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

Other options report an error. This includes linking and library selection
(`-r`, `-L`, `-l`, `-Wl`), dependency generation, archive creation, runtime/JIT
execution, coverage, backtraces, bounds checking and compiler subprocess dispatch.
`-bench` specifically reports the missing elapsed-time clock. Unsupported
options cannot be silently overridden by a later `-c` or `-E`.

The process needs an `output` console with write rights and `memory` with manage
rights; stdin compilation also needs an `input` console with read rights. Sources
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
  Repeat `-I` for multiple paths; the guest driver defers `-L` until linking.
  Configured include defaults are separate `{R}/usr/include`
  and `{B}/include` entries; library defaults are `{R}/usr/lib` and `{B}`.
  The guest recipe supplies those prefixes; permanent guest SDK packaging
  remains a later task.
- Quoted includes first search beside the source file. Rooted includes are
  opened directly, without falling back to search directories. `#include_next`
  requires a relative name. Paths exceeding TCC's existing filename buffers
  produce an error rather than being truncated.
- `#pragma once` reports an error: native file handles have no identity query,
  and aliases cannot be compared reliably using path strings. Use ordinary
  include guards until that filesystem operation exists.
- Expanding `__DATE__` or `__TIME__` reports the missing wall-clock facility.
  No date is invented. Explicit user macro definitions still work normally.

Host-running TCC retains host pathname, clock and `#pragma once` behavior.
The guest compiler builds against the real Pyxis SDK. It excludes the upstream
host tools and clock calls and uses the existing native startup metadata for
debug information. No fd compatibility layer or new libc/kernel API is added.

## Source and local changes

Pinned upstream: [TinyCC 3dc99dbc82f8e07308c5d398136803e62f9676df](https://github.com/TinyCC/tinycc/tree/3dc99dbc82f8e07308c5d398136803e62f9676df)
(`0.9.28rc`). Metadata lists five ordered patches: the Pyxis object target,
runtime symbol ownership, FP scratch allocation, native streams/paths, and the
guest driver. The stream patch uses existing bounded formatting for internal
names and removes an unnecessary `inttypes.h` dependency. Upstream algorithms
and formatting are otherwise retained; the supported port target remains x86-64 Pyxis/ELF.

Upstream `COPYING` contains LGPL 2.1. `lib/libtcc1.c` separately states GPL 2 or
later with its explicit unlimited-linking exception; retain that notice as well.
The stage includes `COPYING` and the three selected runtime source files under
`share/licenses/tcc`, preserving their individual notices. Compiler and runtime
licensing must not be described as one blanket license.
