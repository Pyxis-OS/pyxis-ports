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
  - **Sessions.** Creating the window acquires the keyboard. Creating its
    framebuffer acquires graphics and then the optional pointer subscription;
    destroying the window releases these sessions.
  - **Drawing.** SDL draws into its own surface; `SDL_UpdateWindowSurface`
    and `SDL_RenderPresent` copy the whole surface into a held display slot
    and submit it. Slots rotate and keep older frames, so dirty rectangles
    alone would leave stale areas. The presenter shows only complete frames.
    The first submission shows graphics.
  - **Resize.** When the display geometry changes, the slots are replaced and
    the last frame resubmitted, the display mode updated and the window gets
    SDL's resized event. A failed replacement keeps the old slots until the
    next change.
- **Input** (`SDL_pyxisevents.c`):
  - **Keyboard.** Pyxis key positions map to SDL scancodes; keycodes follow
    SDL's US defaults. Text input uses the SDK's shared US layout
    (`pxe/key_layout.h`), the same table the terminal uses. Control, Alt and
    Super suppress text.
  - **Pointer.** Ordinary motion uses native surface-local positions with matching
    destination and mapping identities, including anchored drags outside the
    window. SDL does not integrate device counts or scale ordinary motion.
    `SDL_WarpMouseInWindow` requests native bounded owner warp; refusal sets
    SDL's error and preserves position. Success arrives through native input.
    Relative mode requests native lock and returns an error when refused,
    including before first presentation or while inactive. Super+Esc and
    focus/device loss revoke it; the event pump clears SDL relative mode,
    pending motion and held buttons without warping. Native ordinary state and
    enter events restore the parked position without requiring device movement.
    A fresh consumed surface
    click permits a new explicit relative-mode request; the adapter does not
    automatically relock. Locked resize keeps held buttons. The wheel follows
    SDL's sign: toward the user is negative.
  - **Cursor.** SDL bitmap and color cursors copy straight-alpha BGRA pixels to
    the native surface cursor, with dimensions 1..64 and an in-image hotspot.
    The default is the native arrow. `SDL_ShowCursor` controls native saved
    visibility, so software-cursor applications can hide it. Lock independently
    hides the cursor and restores the saved preference on unlock.
  - **Focus.** Focus changes and input resets release every held key and
    button, so a key held across them must be pressed again. Keyboard focus
    becomes the window's focus events.
  - **Event pump.** Each pump polls display geometry, keyboard and the acquired
    pointer subscription in one `wait_many`. `SDL_WaitEvent` and positive-timeout
    `SDL_WaitEventTimeout` block on the same handles. Zero remains nonblocking;
    negative means infinite. Finite deadlines are retained across the native
    30-second wait bound, and infinite waits re-arm at that bound. Absent pointer
    grants are omitted; no new input session is acquired by waiting. Unsupported
    waits, ownership/backend errors, or missing window/input sessions fall back
    to upstream polling. SDL's joystick enumeration poll interval remains when
    that subsystem is initialized.
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

## Clipboard

All three clipboard hooks use native explicitly delegated `clipboard_local`
and `clipboard_shared` grants, including when either grant is absent. No SDL
private cache reports a publication or supplies text. A fresh physical local
Ctrl+C/V or Ctrl+Shift+C/V command supplies one operation-specific attempt;
shared Super+Shift+C/V arrives as one Ctrl+C/V event with a private native
identity and per-event Control snapshot. Global modifiers retain their real
state. Unsupported shared commands, including absent grants and refused native
identity, emit no SDL command. Native shared-layer classification also suppresses
refused commands when a focus/acquisition reset removes accepted modifiers; that
classification supplies no authority. Shared press, repeats and releases remain
consumed through physical release, even after modifiers change. Owner-scoped keyboard
refusal consumes a cancelled action without requiring that layer's clipboard
grant, so a refused shared action does not block a later local gesture.

The private event queue node retains the native identity through single-event
GET delivery; public SDL events, PEEK and synthetic pushes supply none. An
unused delivered command is refused before the next real event delivery. Batch
GET, overlapping commands, filtering, command discard, any failed event enqueue,
focus/input reset, display geometry change and ownership release refuse the
pending action. Native ownership, epoch, identity and five-second expiry checks
remain authoritative. Applications must make clipboard calls while handling
the original command; Has-then-Get is supported. Event filters/watchers cannot
use a delivered command's activation: Has refuses without consuming it, while
Set/Get refuse and consume the pending attempt. Nested callbacks keep the same
restriction until all callback scopes return.

Set accepts Unicode scalar UTF-8 through the first NUL, at most 64 KiB excluding
the terminator, preserving bytes and line endings. Invalid or over-limit text
consumes the attempt and preserves the store. Get returns an SDL-allocated
NUL-terminated copy; refusal returns allocated empty text with an SDL error
(NULL is possible on allocation failure). Has reveals only whether nonempty
text currently exists with a live delivered Paste action; it does not consume
or extend that action. No background/menu access, layer fallback, primary
selection, FILE/rich formats or host clipboard bridge is provided.

The [manual exercise](manual/README.md) is explicitly built and loaded by the
operator; ordinary recipes and images do not include it. DevilutionX's text
editor is the in-tree Set and Has-then-Get consumer.

## Patches

1. `0001` turns off the dynamic API: Pyxis links statically and has no `dlopen`.
2. `0002` registers the Pyxis video driver in SDL's bootstrap list.
3. `0003` skips the Steam virtual gamepad file, because Pyxis `stat` has no
   modification time.
4. `0004` makes native Pyxis position, warp and lock authoritative in SDL mouse
   core: no synthetic warp position or relative-mode fallback after lock refusal.
   Other video drivers retain upstream behavior.
5. `0005` permits the threadless Pyxis wait hook without `SendWakeupEvent`.
   There are no asynchronous SDL event producers in this configuration. A
   threaded port needs a real wakeup sender before using this path.

6. `0006` retains the Pyxis native clipboard action identity on the original
   private event queue node. Only original single-event removal delivers it;
   mutation, filtering, discard and ambiguous batches cannot rearm it. The
   Pyxis native keyboard helper translates a shared command's event modifiers
   without modifying global keyboard state. Other drivers keep upstream behavior.

## Not built

These facilities report themselves as unsupported, as upstream does:

- **Threads.** `SDL_CreateThread` fails, and so does `SDL_INIT_TIMER`, whose
  callback timers need a thread. Mutexes and semaphores succeed and do nothing,
  which is correct with one thread.
- **Audio devices.** `SDL_INIT_AUDIO` fails. Conversion and WAV loading work.
- **Haptics, sensors, HIDAPI, shared objects, power, OpenGL and Vulkan.**
- **Joysticks.** They use SDL's dummy driver: `SDL_INIT_JOYSTICK` succeeds
  with zero devices.

Named SDL system-cursor shapes and message boxes are unsupported.
`SDL_main` and `SDL_test` are not built.
