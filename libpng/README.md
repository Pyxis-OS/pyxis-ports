# libpng

Static PNG encoding/decoding from libpng 1.6.59, under the libpng license.
The release archive is pinned by SHA-256 in `metadata.lua` and fetched only
through the owner's shared `raw-sourceforge` mirror. `share/libpng/source.txt`
records that pin and the profile; the upstream license and port notice are
staged under `share/licenses/libpng/`.

```sh
lua build.lua libpng --sdk /path/to/pyxis/build/sdk \
  --zlib /path/to/zlib/stage/dev
```

Build zlib against the same SDK first. Its development prefix is an explicit
input; the runner does not find or build dependencies. Libpng requires libc's
`modf` in addition to existing allocation, memory/string, stdio, setjmp, time
and math functions. Math functions live in libc, with no separate `-lm`.

The recipe compiles unmodified conventional read/write sources using baseline
x86-64 SDK flags. It preserves stdio, setjmp error recovery, floating-point,
ancillary chunks and normal transforms. It omits the simplified read/write API,
architecture acceleration, shared libraries and upstream programs/tests.
`pyxis.dfa` selects only the simplified-API omissions. Upstream
`scripts/pnglibconf.mak` generates `pnglibconf.h` with awk and the target
preprocessor; no target program or host C configuration probe runs.

Development outputs are `stage/dev/lib/libpng.a` and `png.h`, `pngconf.h` and
the matching generated `pnglibconf.h` under `stage/dev/include`. Pyxis exports
them at `build/ports-dev/libpng`, outside the base and guest SDK. Consumers use
both the libpng and zlib include paths, then link their objects, `libpng.a`,
`libz.a` and SDK runtime libraries in that order. The generated header is part
of this library's configuration and must travel with its archive.

Callers own streams, error recovery, image/input limits, storage and I/O
authority. This library grants no filesystem, network or screenshot access.
Only provenance and license notices enter the boot payload. Archive builds and
symbol/configuration inspection precede PNG runtime qualification through the
screenshot consumer.
