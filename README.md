# Pyxis ports

Pinned third-party programs built against the exported Pyxis SDK.

Use Lua 5.4, Git, a POSIX shell, GNU coreutils, GNU Make and the
`x86_64-unknown-pyxis-` toolchain on `PATH`. Sources come only from the
internal mirrors, so building needs access to `git.internal` and
`repo.internal`. Export the SDK with `make sdk` in Pyxis first.

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

`source.url` and `source.commit` are upstream provenance. The runner fetches
that commit from `source.mirror`, a Git mirror under
`https://git.internal/mirrors/` that allows fetching a commit by its ID. A new
port or a moved pin needs its mirror created or synced first.

A recipe may use `source.archive = { url = "https://...", mirror =
"https://...", sha256 = "..." }` instead of a Git checkout. `url` is the
upstream file and `mirror` its cached copy under `https://repo.internal/`,
which the runner downloads. The checksum is mandatory and verified before tar
extraction, which removes the release archive's single top-level directory.
`source.url` and the exact `source.commit` remain upstream provenance; the
archive checksum identifies the actual build input. Archive recipes list curl,
sha256sum, tar and the relevant decompressor as host dependencies.

A standalone data recipe uses `source.file = { url = "https://...", mirror =
"https://...", sha256 = "...", name = "filename" }`. The runner downloads and
verifies the mirror copy without extraction. Its checksum replaces the Git
commit requirement; standalone file recipes do not apply patches. They list
curl and sha256sum as host dependencies and retain upstream provenance beside
the recipe.

Boot-archive inclusion is handled by Pyxis; this repository only stages files.

`install.lua` selects the guest layout from the per-port stage trees for Pyxis's
manifest runner. It includes executables, notices and TCC target support, while
excluding the host compiler and intermediate build files.

## License

Original Pyxis material is licensed under [MPL-2.0](LICENSE). See
[LICENSING.md](LICENSING.md) for scope and third-party exceptions.
