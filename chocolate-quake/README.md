# Chocolate Quake

[Chocolate Quake](https://github.com/Henrique194/chocolate-quake) 2.1.0, pinned
to the `chocolate-quake-2.1.0` release archive (commit
`8ee22175e3a5613e94ad2f9776aadc428b948cf3`), built through the SDK's CMake
toolchain file against the [SDL2 port](../sdl2/README.md). The recipe stages
the program bundle `bin/chocolate-quake.pxb` (`manifest.json` and
`app/bin/chocolate-quake.pxe`), upstream's licence, SDL2's licence and the port
notice. Game data is not part of the recipe. This and the native
[Quake](../quake/README.md) are both packed into the `quake` game package with
the data, and read it as `pkg://quake`, read-only. A missing package refuses launch on both live and
installed systems; explicit `-basedir` keeps its upstream meaning.

```sh
lua build.lua chocolate-quake --sdk /path/to/build/sdk --sdl2 /path/to/sdl2/dev
```

Only the `chocolate-quake` target is built. Upstream requires SDL2_net and the
Vorbis, MP3 and FLAC libraries with no options to turn them off, so the first
patch removes them from the build. There is no network play or music.

## Bundle and grants

`chocolate-quake` selects the bundle, as `lua` selects `lua.pxb`. Its manifest
declares the grants the game uses; nothing is obtained at runtime:

- required: `memory/manage`, `clock` read and sleep, `display/draw` and
  `keyboard/input` and the read-only `quake` package;
- optional: `pointer/input` for mouse look, and for sound `audio/create`,
  `threads/manage` (SDL's audio callback thread) and `pipe/create` (SDL's
  cross-thread event wakeup).

Sound uses upstream's SDL driver (`snd_sdl.c`) through the SDL2 port's audio
backend. When a supplier cannot provide the optional sound grants, or the
executable is launched directly by its `app/bin` path, sound initialization
fails with SDL's error and the game runs silent, as with `-nosound`.

## Patches

1. **No network or music codecs.**
   - **Network:** SDL2_net and the UDP driver (`net_dgrm.c`, `net_udp.c`) are
     left out. The loopback driver, which single player uses, stays. Two
     network structures lose the SDL_net address and socket fields that only
     the UDP driver used.
   - **Music:** the Vorbis, MP3 and FLAC codecs are left out. The WAV codec
     stays.
2. **Platform.**
   - **Timer:** upstream stops when `SDL_Init(SDL_INIT_TIMER)` fails. When this
     adaptation was written the SDL2 port had no threads, which made that call
     fail. It no longer does: `SDL_INIT_TIMER` starts no thread, and the first
     callback timer starts one lazily. Quake only reads the performance counter,
     so Pyxis still skips the call.
   - **Data:** the default base directory is `pkg://quake`, from the
     declared package. Upstream uses `SDL_GetPrefPath`, which would keep
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
