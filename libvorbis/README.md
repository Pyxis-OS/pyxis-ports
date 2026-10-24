# libvorbis

Xiph libvorbis 1.3.7 from the checksum-pinned release archive in `metadata.lua`,
fetched only from the owner's raw-xiph cache.

```sh
lua build.lua libvorbis --sdk /path/to/build/sdk --ogg /path/to/libogg/dev
```

The recipe builds static Vorbis codec, encoder and FILE/callback decoder.
It omits shared libraries, examples and tests.
The development prefix carries the public header layout and `sdk://`
pkg-config metadata with explicit static dependencies. The native compiler
supplies libc, including math functions; no separate `-lm` is needed.

Source checksums and the selected profile are retained in `share/libvorbis`.
Upstream BSD notices and author credits are under `share/licenses/libvorbis`;
original recipe/configuration sources and MPL notices accompany them.
The reusable package includes no game source, private assets or audio device
authority. Callers supply buffers, callbacks or local file grants.
