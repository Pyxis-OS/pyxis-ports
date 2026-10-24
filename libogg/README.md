# libogg

Xiph libogg 1.3.6 from the checksum-pinned release archive in `metadata.lua`,
fetched only from the owner's raw-xiph cache.

```sh
lua build.lua libogg --sdk /path/to/build/sdk
```

The recipe builds static Ogg framing and bit packing, native fixed-width types.
It omits shared libraries, programs and tests.
The development prefix carries the public header layout and `sdk://`
pkg-config metadata with explicit static dependencies. The native compiler
supplies libc, including math functions; no separate `-lm` is needed.

Source checksums and the selected profile are retained in `share/libogg`.
Upstream BSD notices and author credits are under `share/licenses/libogg`;
original recipe/configuration sources and MPL notices accompany them.
The reusable package includes no game source, private assets or audio device
authority. Callers supply buffers, callbacks or local file grants.
