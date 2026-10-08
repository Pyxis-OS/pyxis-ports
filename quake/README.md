# Quake

Pinned erysdren/quakegeneric (the GPL WinQuake software renderer behind a small
platform interface), built with the Pyxis SDK and Clang. The recipe produces
`bin/quake.pxe` and preserves upstream's GPL license. Game data is not
downloaded or included by the recipe; the parent Pyxis build stages the pinned
shareware pak or a locally supplied `QUAKE_DATA` directory.

The adapter replaces upstream's `sys_null.c` and `quakegeneric.c`:

- `quake_pyxis.c` (C23, Pyxis headers) owns the display, keyboard, optional
  pointer and clock sessions, input translation, elapsed time and the 320x240
  frame blit at the largest integer scale, with palette conversion to the
  display's channel shifts.
- `sys_pyxis.c` (gnu99, because Quake's headers define their own false/true
  enum) is the Quake system layer and main loop. File I/O follows `sys_null.c`
  through libc streams; `Sys_mkdir` uses libc `mkdir`; `Sys_Quit` runs
  `Host_Shutdown` so `config.cfg` is written. The heap is 32 MiB.

The loop measures elapsed monotonic time and sleeps to Quake's 72 Hz cap except
during `timedemo`. It starts and keeps running without input focus, including
while its graphics layer is hidden or another space is selected. Explicit game
pause remains available. Focus changes and input resets release every key and
mouse button Quake holds and discard pending motion; fresh presses are required.
Keyboard and pointer notifications control only their own input eligibility.
`+mlook` is queued at every start, and a first run without `config.cfg` binds
the middle button to `impulse 10`.
Defaults are `-basedir boot://share/quake` and `-writedir home://quake`, inserted
before the caller's arguments so trailing `+commands` stay intact.

Ordered patches:

1. `0001-64-bit-quakec-strings.patch`: QuakeC stores strings as 32-bit offsets
   from `pr_strings`. On 64 bits, engine strings in static data can lie more
   than 2 GiB away, which crashed map loading. Reads go through `PR_GetString`;
   writes go through `PR_SetEngineString`, which keeps non-negative offsets
   that fit and gives other strings negative indices into a 512-entry table,
   as Quakespasm does.
2. `0002-writable-game-directory.patch`: `-writedir PATH` sends generated files
   to `PATH/<game directory>` and searches it before the game data.

No sound, networking, CD audio, joystick or video-mode switching. Upstream
compiler warnings remain visible; no broad cleanup or warning blanket is applied
to the vendored engine.
