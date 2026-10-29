# SDL2_mixer

SDL2_mixer 2.8.2, pinned to `release-2.8.2` at
`b208916aed9250fe434360e6c6a95f0697bb7b01`, built unchanged against the native
[SDL2 port](../sdl2/README.md). The release archive comes only from the internal
cache and is verified against its SHA-256 in `metadata.lua`.

```sh
lua build.lua sdl2-mixer --sdk /path/to/build/sdk --sdl2 /path/to/sdl2/stage/dev
```

The development prefix contains `libSDL2_mixer.a`, `SDL2/SDL_mixer.h`, upstream
CMake packages and `SDL2_mixer.pc`, with licenses, provenance and the Pyxis
recipe under `share/`. The CMake target is
`SDL2_mixer::SDL2_mixer-static`; consumers also link `SDL2::SDL2-static`.
The pkg-config dependency is `sdl2` and resolves that static link order.

WAV streaming, WAV/AIFF/VOC samples and channel mixing are enabled. All MIDI
backends, FLAC, GME, MOD, MP3, Opus, Vorbis, WavPack and external-player commands
are disabled. No optional codec library is needed. The library is zlib licensed;
the original Pyxis build recipe and metadata retain MPL-2.0.

Consumers need SDL2's native audio and thread grants. This profile supports
Chocolate Doom's digital sound and its own OPL music synthesizer through
`Mix_HookMusic`; OPL does not require SDL2_mixer's MIDI decoder. General MIDI
and GUS playback remain unavailable. No sound fonts or game data are included.
