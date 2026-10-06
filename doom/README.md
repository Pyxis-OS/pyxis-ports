# Doom

Pinned ozkl/doomgeneric, built with the Pyxis SDK and GCC. The recipe produces
`bin/doom.pxe` and preserves upstream's GPL license. Game data is not downloaded
or included by the recipe. The parent Pyxis build can stage a locally supplied
WAD and the two development demos separately.

The Pyxis adapter uses a display DRAW grant, keyboard INPUT grant and clock
READ/SLEEP grant, plus the normal file/directory resources. It draws a 320x200
frame at the largest integer scale that fits, centred with black borders, using
the display's pitch and channel shifts. No GPU or calendar time is needed.

Use arrows to move/turn, Ctrl to fire, Space to use, Shift to run, Alt to strafe,
and comma/period to strafe left/right. Escape opens the menu; F10 then Y quits.
Super+Left/Right switches spaces. An inactive session blocks on keyboard input
and excludes the inactive interval from game time. Focus changes and input resets
release all held Doom keys, including modifiers shared by left/right keys.

The default IWAD path is `boot://share/doom/DOOM.WAD`; `-iwad` overrides it.
`-playdemo path` plays a demo and returns to the shell when it finishes.

The ordered runtime patch removes unused Unix headers and desktop error-dialog
launching, uses fixed-width integer headers and existing integer/float parsers,
and makes normal quit actually exit. It drains key releases to support focus
resets. Upstream's generic configuration persistence remains disabled; this
save/load adaptation uses native directory creation and libc atomic rename.

Audio, networking, mouse input, configuration persistence, demo
recording, timedemo reporting and alternate render formats/scaling are outside
this first port. `-record`, `-timedemo`, `-gfxmode` and `-scaling`
are rejected explicitly. The renderer remains single-buffered and can tear.
A second patch gives the deferred demo name static storage: the generic entry
point returns while playback still borrows that name. It also restores immediate
exit on a recursive error rather than recursing through shutdown callbacks.

A third patch restores save/load menus and engine dispatch. F2/F3 save/load;
F6 selects a quicksave slot on first use, then F6/F9 confirm quicksave/quickload.
`-loadgame N` loads slot N (0–5). Saves live in
`home://doom/saves/<iwad-name>/`: `doom1.wad` for shareware, `doom.wad` for
registered/Ultimate Doom, and upstream's canonical mission name otherwise.
This uses the detected game mode, not the input filename. Directories are
created on the first save, independently of disabled configuration persistence.
Installed systems keep saves across reboots; on live boots `home://` is RAM and
they last until reboot.

Each save exclusively reserves a temporary name, checks writes and close, then
atomically replaces the slot. Failure reports an error and retains the previous
save; simultaneous successful saves to one slot use last-replacement-wins.
An interrupted save can leave a `temp-*.dsg` file; later saves skip it. Load
errors stop on short reads, and menu descriptions are bounded. The upstream
save format still assumes trusted, matching game data; this is not a hardened
save-file parser or isolation between different PWADs on the same IWAD.

Upstream compiler warnings remain visible; no broad cleanup or warning blanket
is applied to the vendored engine.
