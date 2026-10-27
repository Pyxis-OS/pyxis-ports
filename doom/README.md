# Doom

Pinned ozkl/doomgeneric, built with the Pyxis SDK and Clang. The recipe produces
`bin/doom.pxb` (`manifest.json` and `app/bin/doom.pxe`) and preserves
upstream's GPL license. Game data is not downloaded or included by the recipe.
The bundle ships in the `doom` game package with the data and reads that
package, read-only, as `pkg://doom`;
a missing package refuses launch on both live and installed systems.

The Pyxis adapter uses a display DRAW grant, keyboard INPUT grant and clock
READ/SLEEP grant, plus the normal file/directory resources. It draws a 320x200
frame at the largest integer scale that fits, centred with black borders, using
the display's pitch and channel shifts. No GPU or calendar time is needed.

At each frame boundary the adapter queries the geometry generation. A new
size replaces the owned mapping and recomputes integer scale and letterboxing,
keeping the engine's 320x200 frame and game state. If the destination becomes
too small for scale one, it keeps the old mapping and runs with clipped output
until a later size fits. A failed replacement likewise keeps the old frame;
that generation is observed once, so allocation is retried only after another
resize. Starting Doom still requires a display large enough for scale one.

Use arrows to move/turn, Ctrl to fire, Space to use, Shift to run, Alt to strafe,
and comma/period to strafe left/right. Escape opens the menu; F10 then Y quits.
Super+Left/Right switches spaces. Doom starts and keeps rendering and advancing
game time without input focus, including while its graphics layer is hidden or
another space is selected. Explicit game pause remains available. Input is polled
continuously; focus changes and input resets release all held Doom keys, including
modifiers shared by left/right keys. Returning to graphics requires fresh presses.

The default IWAD path is `pkg://doom/DOOM.WAD`; `-iwad` overrides it.
`-playdemo path` plays a demo and returns to the shell when it finishes.

The first runtime patch keeps native quit, focus-reset release draining, native
save-directory selection and `home://`/`tmp://` routing. Engine headers,
parsers, load/save menus and normal error handling remain upstream.

`-timedemo demo1` plays a demo as fast as possible and exits with upstream's
report, `timed N gametics in M realtics (F fps)`, on stderr. Realtics are
1/35 s, so a short demo's figure is coarse.

Audio, networking, mouse input, configuration persistence, demo
recording and alternate render formats/scaling are outside this first port.
`-record`, `-gfxmode` and `-scaling` are rejected explicitly. Each frame is
drawn into a held display slot and submitted whole.
A second patch gives the deferred demo name static storage: the generic entry
point returns while playback still borrows that name. It also restores immediate
exit on a recursive error rather than recursing through shutdown callbacks.

F2/F3 save/load;
F6 selects a quicksave slot on first use, then F6/F9 confirm quicksave/quickload.
`-loadgame N` loads slot N (0–5). Saves live in
`home://doom/saves/<iwad-name>/`, using upstream's canonical mission name.
Shareware and registered Doom now both use `doom.wad/`; existing saves under
`doom1.wad/` need a manual move if present. No migration is performed. Directories are
created at startup, independently of disabled configuration persistence.
Installed systems keep saves across reboots; on live boots `home://` is RAM and
they last until reboot.

Saving uses upstream's `temp.dsg` path within the selected directory and its
ordinary write, close and remove/rename behavior. No port-owned reservation API,
per-attempt filename, checked-write/close or previous-slot preservation policy
is added. The native home base is unchanged. The format retains upstream's
trusted-file assumptions and error handling.

Upstream compiler warnings remain visible; no broad cleanup or warning blanket
is applied to the vendored engine.
