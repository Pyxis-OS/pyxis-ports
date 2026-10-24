# Monocypher

Monocypher 4.0.2 at the exact commit in `metadata.lua`, fetched as a
checksum-pinned archive from the owner's raw-github cache.

```sh
lua build.lua monocypher --sdk /path/to/build/sdk
```

The static archive contains the unmodified core and optional Ed25519/SHA-512
translation units, matching upstream's normal library. Both public headers
are installed at the include root. `monocypher.pc` uses `sdk://` and supplies
`-lmonocypher`; the compiler supplies the native C runtime. C++ callers use
the same C ABI. No shared library, documentation generator, external crypto
library or upstream test/vector generator is built.

Callers supply keys, nonces, entropy and Argon2 workspace. The package adds
no random source, storage or network authority. `share/monocypher/source.txt`
records the source revision, checksum and selected profile. Complete upstream
license and author notices and the four selected source/header notice blocks
are retained under `share/licenses/monocypher`. Original recipe files and MPL
notices are retained under `share/monocypher/pyxis` and the license directory.
