# Pyxis ports

Pinned third-party programs built against the exported Pyxis SDK.

Use Lua 5.4, Git, a POSIX shell, GNU coreutils, GNU Make and the
`x86_64-unknown-pyxis-` toolchain on `PATH`. Fetching sources needs network
access. Export the SDK with `make sdk` in Pyxis first.

```sh
lua build.lua kilo --sdk /path/to/pyxis/build/sdk
```

The runner fetches the exact commit, applies patches in order, builds with the
SDK and stages the executable and upstream license under `build/kilo/stage`.
It leaves source and intermediate files available for inspection. The work
directory must not already exist: remove it deliberately or select another
with `--work PATH`. `--cross-prefix PREFIX` overrides `CROSS_COMPILE`.
Build paths must use letters, digits, `_`, `.`, `/`, `+` or `-`, matching the
SDK's Make path restrictions. The SDK is consumed without modification.

`ports.lua` lists recipes. Each recipe records source, license, host and Pyxis
dependencies, ordered patches and staged outputs in `metadata.lua`.
Its `build.lua` receives the SDK, sysroot, compiler prefix, recipe, source,
build and stage paths, metadata and an argument-array command runner.
Recipes are trusted Lua code; dependencies are prerequisites, not a package
resolver. See each recipe's README for adaptation notes and limitations.

Boot-archive inclusion is handled by Pyxis; this repository only stages files.

`install.lua` selects the guest layout from the per-port stage trees for Pyxis's
manifest runner. It includes executables, notices and TCC target support, while
excluding the host compiler and intermediate build files.
