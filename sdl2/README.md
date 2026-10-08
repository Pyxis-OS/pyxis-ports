# SDL2

SDL 2.32.10, under the zlib license, built as a static library with a native
Pyxis backend. The source is pinned to the `release-2.32.10` commit and
fetched only from the owner's `mirrors/SDL` mirror. `dev/share/sdl2/source.txt`
records the commit, patches and profile; `dev/share/licenses/sdl2/` keeps the
upstream license and the port notice.

```sh
lua build.lua sdl2 --sdk /path/to/pyxis/build/sdk
```

## Using it

Compile with `-Ipath/to/sdl2/include/SDL2` and link `lib/libSDL2.a` before the
SDK libraries. CMake projects using the SDK's `share/pyxis.cmake` can add the
prefix to `CMAKE_FIND_ROOT_PATH` and call `find_package(SDL2)`: the package
provides upstream's static target `SDL2::SDL2-static` and the usual
`SDL2_INCLUDE_DIRS` and `SDL2_LIBRARIES` variables. There is no shared
`SDL2::SDL2` or `SDL2::SDL2main`.

The installed `SDL_config.h` replaces upstream's platform dispatcher, so
programs see the same configuration the library was built with.
A program needs the `display`, `keyboard` and `clock` grants; `pointer` is
optional, and without it or a mouse the program runs from the keyboard.

## The backend

The files in `pyxis/` are the platform layer:

- **Video** (`SDL_pyxisvideo.c`):
  - **Window.** One window, always the size of the display content area and
    marked fullscreen. A second window is refused.
  - **Sessions.** Creating the window acquires the keyboard and pointer;
    destroying it releases them and the display.
  - **Drawing.** SDL draws into its own surface; `SDL_UpdateWindowSurface`
    and `SDL_RenderPresent` copy the updated rectangles into the display
    mapping. The presenter therefore never shows a cleared or half-drawn
    frame, though rows can still tear. The first presentation shows graphics.
  - **Resize.** When the display geometry changes, the mapping is replaced,
    the display mode updated and the window gets SDL's resized event. A failed
    replacement keeps the old mapping until the next change.
- **Input** (`SDL_pyxisevents.c`):
  - **Keyboard.** Pyxis key positions map to SDL scancodes; keycodes follow
    SDL's US defaults. Text input uses the SDK's shared US layout
    (`pxe/key_layout.h`), the same table the terminal uses. Control, Alt and
    Super suppress text.
  - **Pointer.** Pyxis reports relative counts; one function turns them into
    SDL motion, and SDL keeps the position clamped to the window. Warping
    moves SDL's copy of it, and relative mode works from the same counts.
    There is no acceleration; `SDL_HINT_MOUSE_NORMAL_SPEED_SCALE` scales
    motion. The wheel follows SDL's sign: toward the user is negative.
  - **Focus.** Focus changes and input resets release every held key and
    button, so a key held across them must be pressed again. Keyboard focus
    becomes the window's focus events.
  - **Event pump.** Each pump polls display geometry and keyboard readiness in
    one `wait_many` and polls the pointer.
- **Timer** (`SDL_systimer.c`): ticks and the performance counter come from
  `clock_now` in nanoseconds, and `SDL_Delay` from `clock_sleep_for`. Sleeps
  use HPET deadlines and never finish before the requested deadline. Interrupt
  delivery and scheduling can delay resumption; nanosecond clock values do not
  guarantee wakeup precision.
- **Paths** (`SDL_sysfilesystem.c`):
  - `SDL_GetPrefPath(org, app)` creates and returns `home://APP/`; the
    organisation is not used.
  - `SDL_GetBasePath` is unsupported, because Pyxis has no reliable
    executable path.

## Patches

1. `0001` turns off the dynamic API: Pyxis links statically and has no `dlopen`.
2. `0002` registers the Pyxis video driver in SDL's bootstrap list.
3. `0003` skips the Steam virtual gamepad file, because Pyxis `stat` has no
   modification time.

## Not built

These facilities report themselves as unsupported, as upstream does:

- **Threads.** `SDL_CreateThread` fails, and so does `SDL_INIT_TIMER`, whose
  callback timers need a thread. Mutexes and semaphores succeed and do nothing,
  which is correct with one thread.
- **Audio devices.** `SDL_INIT_AUDIO` fails. Conversion and WAV loading work.
- **Haptics, sensors, HIDAPI, shared objects, power, OpenGL and Vulkan.**
- **Joysticks.** They use SDL's dummy driver: `SDL_INIT_JOYSTICK` succeeds
  with zero devices.

There is no system cursor, message box or clipboard. `SDL_WaitEvent` uses
upstream's polling loop. `SDL_main` and `SDL_test` are not built.
