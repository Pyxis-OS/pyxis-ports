# tree-sitter

tree-sitter 0.26.13's C library for Neovim. The release archive is
checksum-pinned in `metadata.lua` and fetched only from the owner's
`raw-github` mirror.

```sh
lua build.lua tree-sitter --sdk /path/to/build/sdk
```

The recipe compiles upstream's `lib/src/lib.c` amalgamation against the SDK,
without `TREE_SITTER_FEATURE_WASM`. It stages `dev/lib/libtree-sitter.a` and
`dev/include/tree_sitter/api.h`. Link the archive before the SDK runtime
libraries. The graph-output APIs use libc's real `dup` and `fdopen`; there
are no replacement declarations or compatibility stubs.

`0001-pyxis-endian.patch` adds a Pyxis branch to the upstream portable endian
header, using the compiler's byte-order definitions and byte-swap builtins.
The patched header retains its upstream public-domain dedication and license
alternatives. `share/tree-sitter/source.txt` records the pin and profile;
`share/licenses/tree-sitter/` preserves the MIT library license, bundled
Unicode/ICU notices and port notice.

No parser module, parser generator, CLI, Wasmtime runtime, shared library or
upstream test program is built. Linking the library does not make tree-sitter
parsers available in Neovim: Pyxis has no parser-module loader in this slice.
