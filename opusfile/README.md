# opusfile

Xiph opusfile 0.12 from the checksum-pinned release archive in `metadata.lua`,
fetched only from the owner's raw-xiph cache.

```sh
lua build.lua opusfile --sdk /path/to/build/sdk --ogg /path/to/libogg/dev --opus /path/to/opus/dev
```

The recipe builds static FILE/memory/callback Ogg Opus decoder, OP_HAVE_LRINTF.
The Pyxis profile marks `op_freopen` compile-time unavailable and omits its
implementation because native libc has no `freopen`. This scope was accepted
by chronium on 2026-10-24. Every other upstream FILE, memory and callback API
is preserved. `op_fopen`, `op_fdopen` and `op_open_file` use real native libc.
The recipe omits opusurl, HTTP/TLS, shared libraries, examples and tests;
URL declarations in the public header belong to the unbuilt companion library.
The development prefix carries the public header layout and `sdk://`
pkg-config metadata with explicit static dependencies. The native compiler
supplies libc, including math functions; no separate `-lm` is needed.

Source checksums and the selected profile are retained in `share/opusfile`.
Upstream BSD notices and author credits are under `share/licenses/opusfile`;
original recipe/configuration sources and MPL notices accompany them.
The reusable package includes no game source, private assets or audio device
authority. Callers supply buffers, callbacks or local file grants.
