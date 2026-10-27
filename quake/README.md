# Quake

Pinned erysdren/quakegeneric (the GPL WinQuake software renderer behind a small
platform interface), built with the Pyxis SDK and Clang. The recipe produces
`bin/quake.pxb` (`manifest.json` and `app/bin/quake.pxe`) and preserves
upstream's GPL license. Game data is not downloaded or included by the recipe.
The bundle ships in the `quake` game package with the data and reads that
package, read-only, as `pkg://quake`;
a missing package refuses launch on both live and installed systems.

The adapter replaces upstream's `sys_null.c` and uses unchanged `quakegeneric.c`:

- `quake_pyxis.c` (C23, Pyxis headers) owns the display, keyboard, optional
  pointer and clock sessions, input translation, elapsed time and the 320x240
  frame blit at the largest integer scale, with palette conversion to the
  display's channel shifts.
- `sys_pyxis.c` (gnu99, because Quake's headers define their own false/true
  enum) is the Quake system layer and main loop. File I/O follows `sys_null.c`
  through libc streams; `Sys_mkdir` uses libc `mkdir`; `Sys_Quit` runs
  `Host_Shutdown` so `config.cfg` is written. Upstream `QG_Create` owns the
  unchanged 8 MiB heap and initialization; `QG_Tick` dispatches frames.

The loop passes elapsed monotonic time to unchanged upstream `QG_Tick`; the
engine's frame filter owns pacing without an added sleep. It keeps running
without input focus, including
while its graphics layer is hidden or another space is selected. Explicit game
pause remains available. Keyboard focus changes and resets release every held
input; pointer focus, reset and lock changes release held mouse buttons and
discard pending mouse input while preserving keyboard play. Fresh presses are
required. Geometry changes preserve locked buttons and relative motion, while
clearing fractional scroll residue.
The optional pointer session is an ordinary surface subscription. Quake requests
lock once after its first successful presentation and applies device-relative
motion, buttons and wheel only while locked. A refused or revoked lock leaves
keyboard play and rendering available. Super+Esc unlocks without opening the
game menu; a fresh left click on the game surface is consumed by the kernel and
allows Quake to request lock again. Focus gain alone never requests lock.
If the initial request was refused while the space was inactive, a fresh ordinary
left-button press also requests lock and is not applied as a shot. A held button
or motion cannot repeat that request.
Vertical scroll keeps signed fractional residue until a complete 120-unit detent
can produce a wheel key press/release pair; positive viewport movement produces
MWHEELDOWN, negative produces MWHEELUP. Opposite partial increments cancel.
Focus, reset, lock and geometry changes clear residue. Translation retains its
limit of eight wheel pairs per pointer event and discards excess complete steps.
Only the first three buttons have engine bindings; extra buttons and horizontal
scroll have no action.
Mouse preferences and bindings follow upstream and the saved configuration.
Defaults are `-basedir pkg://quake/` and `-writedir home://quake`, inserted
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
Configuration, saves and screenshots use upstream's direct file-writing and
error-handling behavior. There is no added atomic replacement, previous-file
preservation or crash-durability promise.

No sound, networking, CD audio, joystick or video-mode switching. Upstream
compiler warnings remain visible; no broad cleanup or warning blanket is applied
to the vendored engine.
