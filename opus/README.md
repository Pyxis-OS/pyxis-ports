# opus

Xiph opus 1.6.1 from the checksum-pinned release archive in `metadata.lua`,
fetched only from the owner's raw-xiph cache.

```sh
lua build.lua opus --sdk /path/to/build/sdk
```

The recipe builds static floating-point Opus encoder/decoder, baseline x86-64/SSE2.
It omits shared libraries, programs/tests, SSE4.1/AVX2, DRED/OSCE, native CPU flags.
The development prefix carries the public header layout and `sdk://`
pkg-config metadata with explicit static dependencies. The native compiler
supplies libc, including math functions; no separate `-lm` is needed.

Source checksums and the selected profile are retained in `share/opus`.
Upstream BSD notices and author credits are under `share/licenses/opus`;
original recipe/configuration sources and MPL notices accompany them.
The reusable package includes no game source, private assets or audio device
authority. Callers supply buffers, callbacks or local file grants.
