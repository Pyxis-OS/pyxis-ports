# pkgconf for Pyxis

Build the pinned 3.0.10 CLI against a current SDK:

```sh
lua ports/build.lua pkgconf --sdk /path/to/pyxis/build/sdk
```

The `pkgconf` package publishes both `pkgconf` and `pkg-config`; the live archive
also includes both names. There is no subprocess, network fetch or implicit
filesystem authority. A plain command inherits the space's selected roots.

Default search is `sdk://lib/pkgconfig`, with static output (including
`Libs.private`). Redundant `-Isdk://include` and `-Lsdk://lib` are filtered;
SDL2's `-Isdk://include/SDL2` remains. `Requires` carries the static library
order: `pkgconf --cflags --libs raylib` emits SDL2's Cflags and
`-lraylib -lSDL2`. The compiler supplies its usual startup and C runtime.

`PKG_CONFIG_PATH` is searched first, before `PKG_CONFIG_LIBDIR` or the built-in
path. Lists use colons, preserving each `://` scheme marker:

```text
export PKG_CONFIG_PATH=home://lib/pkgconfig:pkg://extra-dev/lib/pkgconfig
pkgconf --cflags --libs extra
```

These paths still require actual grants. Setting a path never creates one.
An empty `PKG_CONFIG_LIBDIR` removes the built-in search path. The usual
`--variable`, `--modversion`, `--exists`, explicit `.pc` path and diagnostic
options remain available. `--shared` requests upstream's non-private metadata;
it does not add shared libraries to Pyxis.

The adaptation preserves URI slashes, excludes the symlink walk (native
filesystems have no symlinks) and uses the existing NULL diagnostic callback
for `--silence-errors` instead of `/dev/null`. The only new libc dependency is
`nl_langinfo(CODESET)`, which reports libc's existing fixed UTF-8 encoding.
Only the CLI and private library closure are built; libpkgconf's API, companion
tools, manuals and tests are not installed. Search-directory deduplication is
textual, using upstream's default configuration rather than invented inode data.

COPYING/AUTHORS carry pkgconf's ISC terms. The bundled CLI option parser has
additional Miller/NetBSD notices, retained in the staged getopt_long sources.
This product includes software developed by the NetBSD Foundation, Inc.
and its contributors.
