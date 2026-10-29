# vitetris on Pyxis

vitetris 0.59.1 at the pinned upstream revision, built with its curses backend
and static ncursesw:

```sh
lua build.lua vitetris --sdk /path/to/sdk --ncursesw /path/to/ncursesw/stage/dev
```

Install `vitetris`, then run `vitetris` with `TERM=pyxis`. Arrows navigate menus,
Enter selects, Backspace or Escape goes back, and `q` exits. In a game, Left
and Right move, Up rotates, Down soft-drops, Space hard-drops, and `p` or Enter
pauses. Local two-player mode, menus, block styles and SIGWINCH resizing use
upstream code. The terminal size remains subject to upstream's 32–127 column
and 20–25 row bounds. Only the Pyxis ncursesw terminal profile is available.

The profile disables networking, tty sockets, joystick, Xlib and Allegro.
Upstream game logic, timers and input remain unchanged. `gettimeofday` and
`usleep` use libc; the bundle declares memory and precise clock read/sleep.
`-help` lists upstream options and save locations.

Configuration uses the supplied `$HOME/.vitetris`, falling back to the
relative `.vitetris` when HOME is absent or too long. The small Pyxis path
patch omits Unix passwd/UID lookup because Pyxis has no Unix user database.
Set `export HOME=home://` to use the supplied home capability for configuration;
the native session does not set the text variable automatically. Configuration
includes the player's high scores. The
upstream extra score-file path is mapped to `home://.vitetris-hiscores` through
a build definition. Original read/write, import and failure behavior remains.
Filesystem access needs ordinary supplied directory grants. Live `home://`
storage is RAM-backed; installed systems use the persistent home volume.

The owner-cache archive is checksum-pinned to upstream commit
`744b5090a6d1315cd7b818e5276ed90ba0f3df23`. Its BSD-2-Clause licence, patched
source and ncursesw notices are bundled; packaging retains the recipe, build
configuration and patch. The target build passed with the current SDK.
Interactive Pyxis qualification is separate. Upstream make still prints an
ignored missing-network-archive move when networking is disabled.
