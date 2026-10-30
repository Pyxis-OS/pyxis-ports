# SDL3

SDL 3.2.28, under the zlib license, built as a static library with a native
Pyxis backend. The source is pinned to the `release-3.2.28` commit and
fetched only from the owner's `mirrors/SDL` mirror. `dev/share/sdl3/source.txt`
records the commit, patches and profile; `dev/share/licenses/sdl3/` keeps the
upstream license and the port notice.

```sh
lua build.lua mesa --sdk /path/to/pyxis/build/sdk
lua build.lua sdl3 --sdk /path/to/pyxis/build/sdk --mesa build/mesa/stage/dev
```

The recipe builds two archives. `stage/dev` contains the ordinary `libSDL3.a`,
upstream's public headers and covered Pyxis backend source. `stage/gl-dev`
contains `libSDL3-osmesa.a` and its metadata and notices. Select one of them,
never both.

## Using it

Compile with `-Ipath/to/sdl3/include` and `#include <SDL3/SDL.h>`, then link
`lib/libSDL3.a` before the SDK libraries. `pkg-config sdl3` gives the same
flags. CMake projects using the SDK's `share/pyxis.cmake` add the prefix to
`CMAKE_FIND_ROOT_PATH` and call `find_package(SDL3)`; the package provides
`SDL3::SDL3-static` and `SDL3::SDL3`, which links it. There is no shared SDL3.

For desktop OpenGL, link `libSDL3-osmesa.a` instead, with Mesa and the C++
runtime: `pkg-config --static sdl3-osmesa`, or `find_package(SDL3OSMesa)` and
`SDL3::SDL3-osmesa-static`.

`SDL_build_config.h` is the library's build configuration and is not
installed; SDL3's public headers need none. It enables only facilities the
Pyxis libc provides. SDL uses its own `iconv` and `nl_langinfo`-free date
format, and keeps its generic semaphore over pthread mutexes and conditions.

A program needs the `display`, `keyboard` and `clock` grants; `pointer` is
optional. `SDL_CreateThread` and callback timers need a bundle declaring
`threads/manage`; without it thread creation fails and timed condition waits
return at once. A threaded program that blocks in `SDL_WaitEvent` also needs
`pipe/create` so other threads can end the wait; without the pipe it falls
back to SDL's polling wait. Audio needs the `audio` grant and, for SDL's
device thread, `threads/manage`.

The OSMesa presentation path copies a matching-size canvas row by row into the
held native slot, zeroing the unused pixel bits. It clears the slot only when
the viewport leaves borders. Different-size canvases retain the same nearest-
neighbour aspect-fit scaling and RGB conversion; submission and slot ownership
are unchanged.

## The backend

The files in `pyxis/` are the platform layer, adapted from the SDL2 port's:

- **Video** (`SDL_pyxisvideo.c`). One window covers the output. A software
  window takes the output size and follows its changes as
  `SDL_EVENT_WINDOW_RESIZED`; `SDL_SetWindowSize` reports the output size
  back. SDL draws into its own surface; presenting copies the whole surface
  into a held display slot and submits it. Fullscreen changes succeed without
  effect, because the window already covers the output. A second window is
  refused.
- **Input** (`SDL_pyxisevents.c`). The keyboard is acquired with the window,
  the pointer after graphics. Keys map to SDL scancodes; printable text comes
  from the SDK's US layout while text input is active. Pointer positions,
  buttons up to 32 and wheel detents come from native events; focus changes
  and input resets release held keys and buttons. `SDL_WaitEvent` waits
  natively on the display, keyboard, pointer and wakeup pipe.
- **Mouse** (`SDL_pyxismouse.c`). Colour cursors up to 64×64, native default
  arrow, warp through the native pointer, and relative mode as an explicit
  native lock. A refused lock is reported; SDL never falls back to warping.
- **OpenGL** (`SDL_pyxisopengl.c`, OSMesa archive only). Contexts render into
  a top-down BGRA canvas of the window's size, which `SDL_SetWindowSize`
  changes. Swapping scales the canvas into the largest centred
  aspect-preserving viewport of the output; pointer coordinates map back to
  the canvas. Only swap interval 0 is supported.
- **Audio** (`SDL_pyxisaudio.c`). One default playback device. Opening it
  acquires the space's native PCM session, held until the last logical
  device closes. The device runs at the native 48 kHz S16LE stereo with
  1,024-frame buffers, one maximum native write; SDL's streams convert
  everything else. SDL's device thread waits until the session is writable
  and writes one buffer, keeping the 80 ms native queue near full. A late
  buffer becomes native silence; engine failure disconnects the device.
  There is no recording.
- **Threads** (`SDL_systhread.c`). Pthreads with at least a 1 MiB stack;
  only normal priority, and real detach.
- **Paths** (`SDL_sysfilesystem.c`). `SDL_GetPrefPath` creates `home://APP/`;
  `SDL_GetUserFolder` knows only `SDL_FOLDER_HOME`; there is no base path.

SDL3 already starts its timer thread at the first `SDL_AddTimer` and stops it
in `SDL_Quit`, so the SDL2 port's timer patch has no counterpart.

## Patches

1. `0001` makes pthread condition timeouts use `CLOCK_MONOTONIC`.
2. `0002` takes file modification times from valid Pyxis `stat` fields; there
   are no creation or access times.
3. `0003` reads the Steam virtual gamepad file's time the same way.
4. `0004` fills SDL's environment from the native environment table; Pyxis has
   no `environ`.
5. `0005` registers the Pyxis video driver.
6. `0006` makes native Pyxis pointer positions authoritative in SDL mouse core:
   no focus decision from the position and no clamping, so drags past the
   window edge keep native coordinates.
7. `0007` registers the Pyxis audio driver.

## Not built

Audio recording is unsupported. Clipboard, joysticks, haptics, sensors,
HIDAPI, camera, GPU, process creation, dialogs, trays, shared objects and
Vulkan use SDL's dummy drivers or are disabled, and report themselves
unsupported. `SDL_test` is not built.
