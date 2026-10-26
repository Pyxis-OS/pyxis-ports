# blurgtext

Upstream blurgtext 0.2.1, with its bundled FreeType 2.15.0 and HarfBuzz 15.0.0,
built as static libraries and the upstream SDL2/OpenGL demo. SDL2's OSMesa
profile uses native display/input and Mesa softpipe. The demo's text, layout,
styles, SVG emoji and CJK sample are unchanged.

```sh
lua build.lua blurgtext --sdk /path/to/sdk \
  --sdl2 /path/to/sdl2/stage/dev \
  --sdl2-osmesa /path/to/sdl2/stage/gl-dev \
  --mesa /path/to/mesa/stage/dev
```

The 0.2.1 tag identifies `bfbb3df5`, including upstream's font-query cache
lifetime fix for issue #7. Its direct and nested gitlinks match 0.2.0: Raqm
0.11.0 (canonical HOST-Oman/libraqm), SheenBidi 3.0.0, libunibreak 8.0,
plutosvg 0.0.8 and PlutoVG 1.3.3. FreeType/HarfBuzz archive pins are unchanged.
The tag and commit archives were compared through the cache; all three platform
patches apply unchanged.

The port keeps the supported FT/HB bundle. Metadata pins every direct and
nested submodule and both library release archives. The runner fetches them
through the cache; CMake receives verified local archives and cross find roots,
so host libraries and upstream fallback downloads are not used. No compiler
container rebuild is needed.

`stage/bin/blurgtext.pxb` runs the demo. It declares the `fonts` package and an
8 MiB stack, with memory, threads, clock, display and keyboard; pointer is
optional. Install the expanded `fonts` package, then run `blurgtext`. Escape
quits. The GL canvas stays 800x600; match the display content size for a 1:1
capture. Resize/aspect fitting can scale it and is not a font-quality check.

`stage/dev` contains the public header, static blurgtext/dependency archives and
pkg-config link metadata. Link with the SDK C++ runtime. No separate
freetype-dev/harfbuzz-dev or installed blurgtext-dev package is added here.

## Native font discovery

`sysfonts_pyxis.c` scans only `pkg://fonts` and its immediate family directories.
The bundle's package declaration is its read authority. FreeType opens single-face
TTF/OTF files, retaining metadata faces in a private FT library. Failures unwind;
missing package, invalid font or exceeded bounds fails enablement. This profile
allows 64 family directories, 512 font files, 32 MiB per font and 1,024-byte paths.
These are implementation bounds, not limits of the font formats. Collections
need an indexed public file-loading API and are refused.

Matching is ASCII-case-insensitive family, italic preference then nearest OS/2
weight, with stable path ordering for ties. Generic sans-serif/serif/monospace
map to Noto Sans/Noto Serif/Noto Sans Mono. Unknown families fall back to Noto
Sans; a required missing character searches all discovered faces for coverage.
Synthetic bold is flagged for a regular face when bold is requested; upright
faces are not falsely reported as italic. No system path, locale or fontconfig
scan is involved. Fonts are trusted package assets; upstream parser/OOM hardening
is not claimed.

## Patches

1. `0001`: optional static build, cached bundle arguments and cross find roots;
   platform-selected sysfonts source.
2. `0002`: package-backed Pyxis sysfonts backend and its internal declarations.
3. `0004`: SDL2/OSMesa selection, package font filenames and callback-only GLAD.
   Startup, presentation, pacing and cleanup follow the upstream demo unchanged.

The package backend uses upstream's existing internal byte loader and face
constructor, so it does not register callback-created faces in the canonical
family cache. Both public add-file/add-memory APIs register, which would change
subsequent style selection. Core font-manager/cache and public header changes
are not needed; the callback cannot resize the family table on this backend.
Upstream 0.2.1 fixes the stock-backend cache-lifetime issue reported as #7;
no port-local core workaround is needed. The existing isolated Fontconfig ASan
reproducer exits successfully on unpatched 0.2.1. That host check does not qualify
the native backend or general thread safety.

PlutoVG's optional mmap-based filesystem font-cache loader is disabled by its
existing build option; SVG rasterization stays enabled. Local changes are listed
separately for upstream discussion. Full licences, attribution, original
metadata and modified source are retained in the bundle. See PORT-NOTICE.
