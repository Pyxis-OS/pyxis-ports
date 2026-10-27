# Chocolate Doom

[Chocolate Doom](https://www.chocolate-doom.org/) 3.1.1, pinned to the
`chocolate-doom-3.1.1` release archive (commit
`410d96855b5df5410ff591a90efeafa889119224`), built through the SDK's CMake
toolchain file against the [SDL2 port](../sdl2/README.md). The recipe stages
`bin/chocolate-doom.pxb` (`manifest.json` and `app/bin/chocolate-doom.pxe`),
upstream's licence, SDL2's licence and the port notice. Game data is not part
of the recipe. This and the native [Doom](../doom/README.md) are both packed into the
`doom` game package with the data, and read it as `pkg://doom`, read-only. A
missing package
refuses launch on both live and installed systems; explicit `-iwad` keeps its
upstream meaning.

```sh
lua build.lua chocolate-doom --sdk /path/to/build/sdk --sdl2 /path/to/sdl2/dev
```

Only the `chocolate-doom` target is built: not Heretic, Hexen, Strife,
`chocolate-setup` or the server. The CMake options `ENABLE_SDL2_MIXER` and
`ENABLE_SDL2_NET` are off, so there is no sound, music or network play.
libsamplerate, libpng and FluidSynth are not
offered to CMake, so screenshots are PCX.

## Patches

1. **SDL2 target.** Upstream links `SDL2::SDL2main` and `SDL2::SDL2`; the Pyxis
   SDL2 package provides only the static library, `SDL2::SDL2-static`.
2. **Platform.**
   - **Textscreen:** there is no external file picker, so file selection
     reports itself unavailable, and no URL opener, so a help URL is printed.
   - **Timidity:** upstream configures Timidity for SDL_mixer even when
     SDL_mixer is disabled; that setup now sits under upstream's own
     `DISABLE_SDL2MIXER` guard.
   - **IWAD search:** `pkg://doom` replaces the XDG and Steam
     directories.
   - **Libc adaptations:**
     - **`putenv` remains unavailable:** a configured video driver goes to SDL's
       `SDL_HINT_VIDEODRIVER` instead of the environment, which only SDL reads
       anyway. The X screensaver embedding, which also sets the environment,
       is left out.
3. **Software scaling.**
   - **Default:** upstream selects the renderer and its two-stage upscale path.
   - **Bug fix:** the upscale texture limit treats a renderer maximum of zero
     as no limit, as SDL documents. Before, the software renderer's zero
     shrank the texture to 0x0.

Upstream compiler warnings remain visible.
