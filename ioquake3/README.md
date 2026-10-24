# ioquake3

Pinned ioquake3 client with the static OpenGL 1 renderer, SDL2's OSMesa profile
and Mesa softpipe. Game modules run as interpreted QVMs from separately supplied
packages. The engine contains no game assets.

Build with the ordinary ports runner, supplying `--sdl2` (the SDL2 development
prefix), `--sdl2-osmesa` (its separate GL development prefix) and `--mesa` (the
Mesa development prefix). The SDK supplies the C++ runtime archives. The recipe
stages `ioquake3.pxb`, including licenses, the patched engine source and Pyxis
adapter sources. Its default logical canvas is 320 by 240; use the normal
`r_mode` cvar to change it.

There is no network transport, native game-module loader or QVM compiler in
this profile. OpenAL, HTTP downloads, VoIP, Mumble and Opus are disabled. SDL
provides audio; bundled Ogg/Vorbis provides OpenArena music. Bundled zlib and
IJG JPEG remain on the exact engine source pin.
