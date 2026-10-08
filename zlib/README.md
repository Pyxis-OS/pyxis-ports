# zlib

Static compression/decompression and checksums from zlib 1.3.2, under the zlib
license. The release archive is pinned by SHA-256 in `metadata.lua` and fetched
only from the owner's `raw-zlib` mirror. `share/zlib/source.txt` records the
archive URL, checksum and profile; `share/licenses/zlib/` retains the complete
upstream license and port notice.

```sh
lua build.lua zlib --sdk /path/to/pyxis/build/sdk
```

The recipe compiles unmodified upstream core sources and `compress.c`/
`uncompr.c` using the SDK's baseline x86-64 flags. It retains the normal
deflate/inflate, raw/zlib/gzip stream formats, checksums, dictionaries,
compression levels and buffer convenience APIs, including the size_t variants.
It does not build the `gz*` file helpers, contrib, shared libraries or upstream
programs/tests. The unmodified public header still declares omitted file
helpers; using those functions fails at link time.

The upstream `zconf.h` selects the C23/LP64 target types without a configure
probe. Shipped CRC and fixed-Huffman tables avoid runtime table initialization;
`DYNAMIC_CRC_TABLE`, `BUILDFIXED` and `Z_SOLO` are unset. Default stream
allocators use the SDK's `malloc`/`free`; callers may supply upstream allocation
callbacks instead. Streams remain caller-owned. This library provides no
filesystem, network or screenshot authority and starts no threads.

Development outputs are `stage/dev/lib/libz.a`, `stage/dev/include/zlib.h` and
`stage/dev/include/zconf.h`. Pyxis exports them at `build/ports-dev/zlib`, outside
the base and guest SDK. Consumers compile with that prefix's `include` path
and link `libz.a` before the SDK runtime libraries. Only provenance and license
notices enter the boot payload. PNG runtime qualification belongs to the
screenshot consumer; an archive build alone does not establish it.
