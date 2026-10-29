# Chocolate Doom

[Chocolate Doom](https://www.chocolate-doom.org/) 3.1.1, pinned to the
`chocolate-doom-3.1.1` release archive (commit
`410d96855b5df5410ff591a90efeafa889119224`), built through the SDK's CMake
toolchain file against the [SDL2 port](../sdl2/README.md) and
[SDL2_mixer](../sdl2-mixer/README.md). The recipe stages
`bin/chocolate-doom.pxb` (`manifest.json` and `app/bin/chocolate-doom.pxe`),
upstream's licence, SDL2/SDL2_mixer notices and the port notice. Game data is not
part of the recipe. This and the native [Doom](../doom/README.md) are packed into the
`doom` game package with the data, and read it as `pkg://doom`, read-only. A
missing package
refuses launch on both live and installed systems; explicit `-iwad` keeps its
upstream meaning.

```sh
lua build.lua chocolate-doom --sdk /path/to/build/sdk \
  --sdl2 /path/to/sdl2/stage/dev --sdl2-mixer /path/to/sdl2-mixer/stage/dev
```

Only the `chocolate-doom` target is built: not Heretic, Hexen, Strife,
`chocolate-setup` or the server. `ENABLE_SDL2_MIXER` enables upstream digital
sound effects and the game's own OPL music synthesizer. SDL2_mixer's MIDI
decoders are disabled; General MIDI and GUS modes cannot play music. The default
`snd_musicdevice 3` and `snd_sfxdevice 3` select Sound Blaster effects and OPL
music. The bundle requests optional native audio and threads; without them,
upstream reports an audio initialization failure. `-nosound` disables both,
`-nomusic` disables music, and `-nosfx` disables effects. The in-game Options
menu controls separate effects and music volumes. No sound fonts are needed.

`ENABLE_SDL2_NET` is off. libsamplerate, libpng and FluidSynth are not offered
to CMake, so screenshots are PCX. The upstream music-pack module remains
available with this mixer's WAV support; compressed music packs need codecs
outside this profile. No audio runtime check is implied by a successful build.

## Patches

1. **SDL2 target.** Upstream links `SDL2::SDL2main` and `SDL2::SDL2`; the Pyxis
   SDL2 package provides only the static library, `SDL2::SDL2-static`.
   SDL2_mixer targets likewise use `SDL2_mixer::SDL2_mixer-static`.
   The exported `SDL2MIXER_MIDI=OFF` derives `DISABLE_SDL2MIXER_MIDI` for Doom.
2. **Platform.**
   - **Music profile:** the derived MIDI guard excludes `i_sdlmusic.c`'s SDL
     MIDI/TiMidity backend, its module registration and its initialization.
     Effects, native PC speaker emulation, Doom's OPL synthesizer and the
     music-pack path retain upstream code. The Timidity config variable stays
     available for upstream config-file compatibility.
   - **Textscreen:** there is no external file picker, so file selection
     reports itself unavailable, and no URL opener, so a help URL is printed.
   - **IWAD search:** `pkg://doom` replaces the XDG and Steam
     directories. Temporary conversion files use `tmp://` when TMPDIR is unset.
   - **Video environment:** Pyxis has no borrowed-string `putenv`; a configured
     video driver uses SDL's native hint and X screensaver embedding is omitted.
3. **Software scaling.**
   - **Default:** upstream selects the renderer and its two-stage upscale path.
   - **Bug fix:** the upscale texture limit treats a renderer maximum of zero
     as no limit, as SDL documents. Before, the software renderer's zero
     shrank the texture to 0x0.

Upstream compiler warnings remain visible.
