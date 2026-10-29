# nudoku on Pyxis

nudoku 8.0.1 over static ncursesw. Build with an exported SDK and the staged
ncursesw development prefix:

```sh
lua build.lua nudoku --sdk /path/to/sdk --ncursesw /path/to/ncursesw/stage/dev
```

Install `nudoku`, then run `nudoku` with `TERM=pyxis` in a terminal at least
75 columns by 28 rows. Arrows or `h`/`j`/`k`/`l` move, `1` through `9` enter a
number, `x` deletes it, `u` undoes, `c` checks, `H` hints, `N` starts another
puzzle, `G` saves, and `Q` or Escape quits. `nudoku -r` resumes a save;
`nudoku -d hard` selects difficulty. `-h` lists options. Native KEY_RESIZE
uses upstream redraw handling. The terminfo ACS mapping supplies ASCII
`+`, `-` and `|` for the unchanged upstream box drawing.

Upstream saves to `$XDG_STATE_HOME/nudoku/state.save`, or when that variable
is absent, `$HOME/.local/state/nudoku/state.save`. It creates only the final
`nudoku` directory. The native session supplies a `home` directory capability,
not the text variable `HOME`. Before saving, set `export HOME=home://` and run
`mkdir home://.local` then `mkdir home://.local/state` (or set XDG_STATE_HOME
to an existing writable directory). Loading with `-r` opens
and removes the save before reading it; save failures and consumption semantics
remain upstream behavior. Directory grants need read, write, create and remove
for save/resume. On live systems `home://` is RAM-backed; installed systems use
the persistent home volume.

The selected release tag archive has no generated configure script. A small
Makefile and config.h select the upstream `--disable-nls --disable-cairo`
profile directly: English strings, no PDF/PNG output. Upstream C is unchanged.
The bundle requests memory and precise clock read/sleep; streams, cwd and
filesystem roots follow ordinary bundle launch grants.

The owner-cache 8.0.1 archive is checksum-pinned; its upstream revision is
`96638addc1b5ec45293ed122ffb6b68248851115`. Sources are GPL-3.0-or-later, including
the bundled gettext header. The bundle retains upstream source, licence and
ncursesw notices; packaging also retains the recipe and configuration. The
target build passed with the current SDK. Interactive Pyxis qualification is
separate.
