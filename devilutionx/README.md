# DevilutionX

DevilutionX 1.5.5, the Diablo engine reimplementation, built against the
Pyxis SDK with the [SDL2 port](../sdl2/README.md). The source is pinned to the
`1.5.5` commit and fetched only from the owner's `mirrors/DevilutionX`. No game
data is included.

**Licensing.** DevilutionX is under the non-commercial Sustainable Use
License, and the executable links libmpq under the GPL. Nobody who distributes
the combined program can meet both licences, so an image containing it is for
the person who built it. `PORT-NOTICE` explains this and lists every licence
staged with it. Pyxis builds this recipe only when `DEVILUTIONX=1` asks for the program, without game data.

```sh
lua build.lua devilutionx --sdk /path/to/pyxis/build/sdk \
  --zlib ZLIB_DEV --libpng LIBPNG_DEV --fmt FMT_DEV --sdl2 SDL2_DEV
```

The four prefixes are the development outputs of the zlib, libpng, fmt and
SDL2 recipes (`stage/dev`). DevilutionX finds them with CMake: zlib and libpng
through CMake's own modules, fmt and SDL2 through their packages.

## Dependencies

DevilutionX builds six more dependencies from source, at the commits it pins
itself. They are this recipe's `source.extra` entries:

- bzip2 1.0.8;
- libmpq and libsmackerdec, DevilutionX's forks;
- SimpleIni;
- SDL_image 2.0.5, of which only the PNG loader is built, from its release
  archive;
- SDL_audiolib at commit `cc1bb6af8d4cf5e200259072bde1edd1c8c5137e`, the
  archive upstream pins by MD5, here by SHA-256 through the download cache,
  with only its WAV and MP3 decoders. `sdl_audiolib-0001-fmt12` includes
  `<fmt/format.h>`, where fmt 12 declares `fmt::format`.

The build uses CMake's FetchContent with downloads turned off
(`FETCHCONTENT_FULLY_DISCONNECTED`), pointed at those directories.

## Bundle and grants

`devilutionx` selects the program bundle `devilutionx.pxb`, as `lua` selects
`lua.pxb`. Its manifest declares the grants the game uses; nothing is obtained
at runtime:

- required: `memory/manage`, `clock` read and sleep, `display/draw` and
  `keyboard/input` and the read-only `diablo-data` package as `diablo_data://`;
- optional: `pointer/input`; for sound `audio/create`, `threads/manage`
  (SDL's audio callback thread) and `pipe/create` (SDL's cross-thread event
  wakeup); and `clipboard_local` and `clipboard_shared` (`publish`, `paste`)
  for the text editor's Copy and Paste.

Sound uses SDL_audiolib through the SDL2 port's audio backend. Without the
optional sound grants, or when the executable is launched directly by its
`app/bin` path, `Aulib::init` fails with "Couldn't create audio thread" and
the game runs silent. Without a clipboard grant, Copy and Paste refuse.

## Patches

1. `0001-pyxis-platform` adds `CMake/platforms/pyxis.cmake`, selected when
   `CMAKE_SYSTEM_NAME` is `Pyxis`:
   - single player only, no tests;
   - loose assets, because there is no `smpq` host tool;
   - no post-link strip, because `llvm-strip` cannot read P1F executables;
   - no link-time optimization;
   - upstream's threads stub, because threads are part of libc, with
     `DVL_NO_FILESYSTEM`.

   It also keeps `_POSIX_C_SOURCE` off, since Pyxis libc is not POSIX.
2. `0002-pyxis-file-checks` gives `file_util.cpp` Pyxis branches:
   - file and directory existence use `stat`, so read-only package data
     counts as present;
   - resizing a save uses `ftruncate`.
3. `0003-libcxx-fmt12` makes the build fixes for libc++ and fmt 12:
   `<new>` for `std::launder`, `<cstdlib>` for `std::getenv`, and
   `<fmt/format.h>` in `xpbar.cpp`.
4. `0004-pyxis-paths-and-defaults` sets Pyxis paths and defaults:
   - **Paths:** data in `diablo_data://` unless `--data-dir` says otherwise.
     A missing `diablo-data` package refuses launch on both live and installed
     systems. Program assets come from the bundle's `app/assets/` through its
     read-only `devilution_assets://` resource root.
   - **Preferences:** saves and `diablo.ini` always go to SDL's preference path,
     `home://devilution/`. A writable `diablo.ini` in the working directory no
     longer redirects them.
   - **Frame rate:** defaults to "Limit FPS", because Pyxis has no vertical sync.

5. `0005-pyxis-command-modifier-snapshot` makes the text editor use the
   delivered key command's modifiers on Pyxis. Shared Copy/Paste arrives as one
   Ctrl+C/V event; the adapter leaves global Control state unchanged. The editor
   retains upstream Set and Has-then-Get calls, including visible Set errors.
   Ctrl+X has no native Copy activation and therefore cannot delete selected
   text by claiming a successful clipboard publication.

DevilutionX's files use CRLF line endings. The patches add lines with LF, so
`git apply --whitespace=error-all` accepts them.

## Cursor options

The game's "Hardware Cursor" option uses its program-supplied SDL color cursor
as the native Pyxis surface image. Pyxis currently composes that image in
software; the VirtIO hardware cursor is a later pointer milestone task.
Disabling the option keeps DevilutionX's own software cursor and hides the native
cursor through SDL.

The SDL backend accepts cursor images from 1 to 64 pixels on each axis, after
DevilutionX's scaling. A larger image makes `SDL_CreateColorCursor` fail;
upstream logs the refusal, disables that cursor and draws its software fallback.
The game data and asset inputs stay unchanged.

## Staged files

| Path | Contents |
| --- | --- |
| `bin/devilutionx.pxb/manifest.json` | the bundle manifest |
| `bin/devilutionx.pxb/app/bin/devilutionx.pxe` | the game |
| `bin/devilutionx.pxb/app/assets/` | DevilutionX's own assets |
| `share/devilutionx/source.txt` | commit, patches and dependency pins |
| `share/licenses/devilutionx/` | the notice and every licence of the code linked in |

The `diablo-data` package supplies the game data, such as `spawn.mpq`;
this recipe never stages it.
