# Chocolate Quake

[Chocolate Quake](https://github.com/Henrique194/chocolate-quake) 2.1.0, pinned
to the `chocolate-quake-2.1.0` release archive (commit
`8ee22175e3a5613e94ad2f9776aadc428b948cf3`), built through the SDK's CMake
toolchain file against the [SDL2 port](../sdl2/README.md). The recipe stages
`bin/chocolate-quake.pxe`, upstream's licence, SDL2's licence and the port
notice. Game data is not part of the recipe; the image stages one set of Quake
data for both this and the native [Quake](../quake/README.md).

```sh
lua build.lua chocolate-quake --sdk /path/to/build/sdk --sdl2 /path/to/sdl2/dev
```

Only the `chocolate-quake` target is built. Upstream requires SDL2_net and the
Vorbis, MP3 and FLAC libraries with no options to turn them off, so the first
patch removes them from the build. There is no network play or music. Sound
initialization fails as SDL2 has no audio here, and the game runs silent, as
with `-nosound`.

## Patches

1. **No network or music codecs.**
   - **Network:** SDL2_net and the UDP driver (`net_dgrm.c`, `net_udp.c`) are
     left out. The loopback driver, which single player uses, stays. Two
     network structures lose the SDL_net address and socket fields that only
     the UDP driver used.
   - **Music:** the Vorbis, MP3 and FLAC codecs are left out. The WAV codec
     stays.
2. **Platform.**
   - **Timer:** upstream stops when `SDL_Init(SDL_INIT_TIMER)` fails. SDL's
     timer subsystem needs threads, which the SDL2 port doesn't have, and
     Quake only reads the performance counter, so Pyxis skips that call.
   - **Data:** the default base directory is `boot://share/quake`, where the
     image stages the data. Upstream uses `SDL_GetPrefPath`, which would keep
     data and generated files together under `home://`.
   - **Generated files:** `-writedir PATH`, default `home://chocolate-quake`,
     sends config, saves, screenshots, demos and the `-condebug` log to
     `PATH/<game directory>` and searches it before the data, as the native
     port's option does. Files are written in place, as upstream does.
   - **Directories:** `Sys_mkdir`, empty upstream, calls libc `mkdir`.
3. **Frame sleep.** Upstream checks the clock in a tight loop between its
   72 Hz frames, keeping one CPU busy. On Pyxis the frame filter sleeps through
   the rest of the frame with `SDL_Delay`, in whole milliseconds rounded down,
   so the next frame is never late. Timedemos don't sleep.

## Memory

The heap is upstream's 256 MiB, taken with one allocation at startup. Pyxis
backs private memory eagerly, so it is committed at startup.
- **Measured in QEMU:** the system's allocated memory rose by about 288 MiB
  at startup and stayed there through demo1. A local variant with a 32 MiB
  heap, never shipped, rose by about 52 MiB; native Quake by about 48 MiB.
- **Small configurations:** a QEMU guest with 512 MiB needs room for the
  whole 288 MiB while the game runs.

Upstream compiler warnings remain visible.
