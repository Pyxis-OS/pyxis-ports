# EDuke32

[EDuke32](https://voidpoint.io/terminx/eduke32) pinned to master commit
`ec5824db81817866f70da326d3811bb0f52b3517` (2026-08-07; upstream has no
release tags), built with its own GNU Make build against the
[SDL2 port](../sdl2/README.md). The recipe stages the program bundle
`bin/eduke32.pxb` (`manifest.json` and `app/bin/eduke32.pxe`), the GPL and
Build licences, SDL2's licence and the port notice. It is a personal-use
build: Pyxis builds it only for the owner's personal `duke3d` game package
(`make game-packages GAMES=duke3d` with a private recipe), and never installs it
with the ordinary ports or in an image. Game data is not part of the recipe.

```sh
lua build.lua eduke32 --sdk /path/to/build/sdk --sdl2 /path/to/sdl2/dev
```

## Bundle and grants

`eduke32` selects the bundle, as `lua` selects `lua.pxb`. Its manifest
declares the grants the game uses; nothing is obtained at runtime:

- required: `memory/manage`, `clock` read and sleep, `display/draw` and
  `keyboard/input` and the read-only `duke3d` package as `pkg://duke3d`;
- optional: `pointer/input` for the mouse, and for sound `audio/create`,
  `threads/manage` (SDL's audio callback thread) and `pipe/create` (SDL's
  cross-thread event wakeup), and `clipboard_local` and `clipboard_shared`
  (`publish`, `paste`) for the console's paste.

Sound effects and music use upstream's SDL driver (`driver_sdl.cpp`) through
the SDL2 port's audio backend at 48 kHz; music is upstream's default AdLib
OPL3 emulation. When a supplier cannot provide the optional sound grants, or
the executable is launched directly by its `app/bin` path, sound startup fails
with "SDL Audio: error in OpenAudio" and the game runs silent. Upstream's
`-ns` only turns sound effects and music off; the device stays open.

## Build

Only the `eduke32` game is built, with the classic software renderer. The
Makefile passes `PLATFORM=PYXIS` and turns off OpenGL, Polymer, libvpx,
Vorbis, FLAC, XMP, network play, mimalloc, the startup window, GTK and LTO.
`Common.mak` treats the unknown platform generically; the recipe supplies the
SDK's flags and replaces the host libraries with SDL2 and the SDK's. The SDK
headers are system directories, searched after EDuke32's own, because
EDuke32's `log.h`, `keyboard.h` and others share names with Pyxis headers.

## Patches

1. **Platform.**
   - **Build:** the ImGui OpenGL backend is left out without OpenGL, and ENet
     on Pyxis. `mmulti.cpp` keeps only the player globals the game reads.
     ENet's header gains a declaration-only Pyxis branch for the types other
     headers use.
   - **Headers:** Pyxis byte order from the compiler, no `<malloc.h>`,
     `<sys/ioctl.h>` or ImGui shell functions, the memory-mapping `mio.hpp`
     only with OpenGL, and three standard includes upstream gets transitively
     elsewhere.
   - **Files:** `BS_IREAD` and `BS_IWRITE` together make the 0666 libc's
     `open` accepts for creation. The data checksum cache matches on size
     alone, and output redirection to `stdout.txt` is off.
   - **Whole reads:** `Bread` loops until it has the count, end of file or an
     error, and the data checksum uses it (see the libc gaps).
   - **System:** the page size comes from `MEMORY_PAGE_SIZE`; there are no
     signal handlers or stack traces; the log uses libc's working-directory
     description.
   - **Refresh rate:** Pyxis displays report none, which SDL gives as 0.
     Upstream's default frame limit divides by it and hangs in its catch-up
     loop, so Pyxis uses 60 Hz.
   - **Sound:** after sound startup fails, playing a sound returns at once.
     Upstream tries each sound and logs every failure.
2. **Thread limits.** SDL's audio callback thread mixes sound beside the
   game's thread, so upstream's thread-local state, loguru's mutex and the
   audio library's lock stay as upstream has them. The patch leaves out only
   what Pyxis lacks or the build does not use:
   - loguru builds without `<regex>`, signal handlers, stack traces or pthread
     thread names;
   - the audio library's async tasks, used only by the Vorbis and XMP
     decoders, are left out.
3. **Paths.**
   - **Locations:** the data is `pkg://duke3d` and generated files go to
     `home://eduke32/`, created on first run and searched first. A missing
     `duke3d` package refuses launch on live and installed systems. Explicit
     data and working-directory options keep their upstream meaning. Upstream
     uses the executable's directory and the XDG or Steam paths.
   - **Working directory:** `buildvfs_chdir` and `buildvfs_getcwd` use libc's
     retained directory capabilities and descriptive scheme path. File operations
     pass their original paths to libc; no private textual cwd or pre-lookup
     component collapsing substitutes for ordered native traversal.
   - **Pyxis roots:** `Bcorrectfilename` keeps `boot://` and similar roots
     whole instead of collapsing the double slash.
   - **Home:** `Bgethomedir` returns `home://`.
4. **Frame sleep.** Upstream checks the clock in a loop until the next frame
   is due, keeping one CPU busy. On Pyxis the frame limiter sleeps through the
   rest of the frame with `nanosleep`. Frames are paced as before; demo
   profiling renders its extra frames inside a paced iteration and is
   unaffected.

## Libc gaps

The port works around these instead of adding them to libc, as accepted for
this task; each needs a design decision first:
- **Whole reads:** libc `read` returns one native transfer, under 4 KiB from
  a file, so it can stop short of the count before the end. EDuke32 expects
  POSIX's whole reads from regular files.
- **File metadata:** `stat` reports no modification time.
- **Absent:** `getuid` and `getpwuid`, `sysconf`, signals, `ioctl`,
  `freopen` and `setvbuf`. Upstream's `fopenfrompath`, which only the
  Mapster32 editor uses, builds with libc's `fdopen`.

## Memory

Upstream's 96 MiB data cache and smmalloc's 16 MiB of buckets are allocated at
startup, and Pyxis backs private memory eagerly. In QEMU the system's
allocated memory rose by about 158 MiB while the game ran.

Upstream compiler warnings remain visible. One in loguru, an unused thread
name, follows from patch 2.
