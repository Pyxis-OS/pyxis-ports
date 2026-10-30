# freetype

FreeType 2.15.0, the release blurgtext already pins, builds its unchanged static
library against the exported Pyxis SDK. The recipe downloads only the
checksum-pinned archive from the owner's sourceforge proxy.

The development prefix provides `include/freetype2/`, `libfreetype.a`, upstream
CMake targets and `freetype2.pc`. Use `find_package(Freetype REQUIRED)` with
`Freetype::Freetype`, or `pkg-config --cflags --libs freetype2`.

The profile is plain FreeType with upstream's default modules. Compressed fonts
use FreeType's internal zlib. HarfBuzz, PNG (color bitmap glyphs), bzip2 and
Brotli (WOFF2) are not built; FreeType reports those features as unavailable.
Blurgtext keeps its own FreeType/HarfBuzz build until it is moved onto a package.

Notices and provenance are installed under `dev/share/licenses/freetype` and
`dev/share/freetype`. The latter includes the complete Pyxis recipe source.
Shared libraries and upstream demo/test programs are not built.
