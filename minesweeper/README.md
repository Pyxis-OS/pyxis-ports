# Minesweeper

Pinned Andrew Hamel raylib Minesweeper, MIT. The cache URL, commit and SHA-256
are in metadata.lua. The static build consumes explicit SDK, raylib and SDL2
prefixes; it downloads only the pinned internal mirror archive.

```sh
lua ports/build.lua minesweeper --sdk build/sdk --work build/ports/minesweeper \
  --raylib build/ports/raylib/stage/dev --sdl2 build/ports/sdl2/stage/dev
```

The 600x600 logical board has 15x15 cells and 22 mines. Left click reveals,
right click toggles a flag, R resets and Escape exits. The first click can lose,
as upstream. No sound, network, persistence or launcher authority is needed.
The bundle requires pointer/keyboard input, display, memory and a precise sleeping
clock; its unchanged flag image comes through read-only app://resources/flag.png.
The adaptation caps drawing at 60 FPS and reports startup/texture failures.

The public package must carry the staged MIT license and provenance plus raylib
and SDL2 notices for the statically linked closure. Unused WAV files are omitted.
