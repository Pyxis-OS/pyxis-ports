# EDuke32

[EDuke32](https://voidpoint.io/terminx/eduke32) pinned to master commit
`ec5824db81817866f70da326d3811bb0f52b3517` (2026-08-07; upstream has no
release tags), built with its own GNU Make build against the
[SDL2 port](../sdl2/README.md). The recipe stages `bin/eduke32.pxe`, the GPL
and Build licences, SDL2's licence and the port notice. It is a personal-use
build: Pyxis builds it only when given the owner's `duke3d.grp`, and never
installs it with the ordinary ports. Game data is not part of the recipe.

```sh
lua build.lua eduke32 --sdk /path/to/build/sdk --sdl2 /path/to/sdl2/dev
```

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
     alone, and output redirection to `stdout.txt` is off. `fopenfrompath`,
     used only by the editor, is left out.
   - **Whole reads:** `Bread` loops until it has the count, end of file or an
     error, and the data checksum uses it (see the libc gaps).
   - **System:** the page size comes from `MEMORY_PAGE_SIZE`; there are no
     signal handlers or stack traces; the log's working-directory line is
     empty.
   - **Refresh rate:** Pyxis displays report none, which SDL gives as 0.
     Upstream's default frame limit divides by it and hangs in its catch-up
     loop, so Pyxis uses 60 Hz.
   - **Sound:** after sound startup fails, playing a sound returns at once.
     Upstream tries each sound and logs every failure.
2. **Single thread.** Pyxis processes have one thread:
   - loguru builds without threads, `<regex>` or signals; its mutex is a
     no-op, there is no flush thread, and its thread ID and error context are
     plain values;
   - smmalloc's per-thread cache and the audio library's lock flag are plain
     globals, and the library's async tasks, used only by the Vorbis and XMP
     decoders, are left out;
   - minicoro is built with `MCO_NO_MULTITHREAD`.
3. **Paths.**
   - **Locations:** the data is `boot://share/duke3d/` and generated files go
     to `home://eduke32/`, created on first run and searched first. Upstream
     uses the executable's directory and the XDG or Steam paths.
   - **Working directory:** the port keeps its own, set by `buildvfs_chdir`.
     The engine's open, `fopen`, `stat`, `mkdir`, `unlink` and directory
     listings resolve relative names against it and collapse `./` and
     repeated slashes.
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
- **Working directory:** libc has no `chdir` or `getcwd`; patch 3 keeps one.
- **Whole reads:** libc `read` returns one native transfer, under 4 KiB from
  a file, so it can stop short of the count before the end. EDuke32 expects
  POSIX's whole reads from regular files.
- **File metadata:** `stat` reports no modification time.
- **Absent:** `getuid` and `getpwuid`, `sysconf`, signals, `ioctl`, `fdopen`,
  `freopen` and `setvbuf`.

## Memory

Upstream's 96 MiB data cache and smmalloc's 16 MiB of buckets are allocated at
startup, and Pyxis backs private memory eagerly. In QEMU the system's
allocated memory rose by about 158 MiB while the game ran.

Upstream compiler warnings remain visible. Two in loguru, an unused thread
name and an unread flush flag, follow from patch 2.
