# Hexyl

Optional Hexyl v0.17.0, built with a supplied native Pyxis Rust std toolchain.

```sh
lua build.lua hexyl --sdk /absolute/path/to/pyxis/build/sdk \
  --rust /absolute/path/to/pyxis-rust-prefix --work /absolute/fresh/hexyl-work
```

The Rust prefix must include native std, an installation receipt with hashes,
its notices, and the canonical SDK linker adapter under `share/pyxis-rust`.
The recipe verifies its compiler, Cargo, target rlibs and adapter against that
receipt. It consumes the supplied SDK and prefix without rebuilding either.
Use a prefix whose std implements the admitted read-only native file operations;
the earlier stdio-only Gate 2 prefix is insufficient.

Sources use the approved Hexyl archive and Cargo proxy only. The upstream lock
is retained beside a narrow patch: remove Hexyl's direct libc dependency and
replace terminal_size 0.4.0 with the separately checksum-pinned patched source.
Every other package version and checksum remains pinned. Locked vendoring uses
a private Cargo home; the target build is frozen and offline. There is no
upstream fallback. Release LTO and embedded bitcode are disabled, and panic
aborts the process. Target linking uses the SDK's ordinary native C contract.
Line-level debug information is enabled; the canonical adapter also writes an
ELF and link map under the work directory's `build/debug-elf` for debugger use.
Those debugger artifacts remain outside the staged package.

The executable stages at `stage/bin/hexyl.pxe`; the parent selects optional
package publication and installation. Source provenance is in `share/hexyl`.
`share/licenses/hexyl` retains the upstream licenses, complete EPL-1.0 text, embedded table notices,
original and patched locks, ordered patches, complete vendored source/notices,
dependency inventory, Rust notices/receipt and SDK notices/manifest. The source
archive includes the locked closure, including build-only and unused platform
packages; its inventory is not a claim that every package is linked or supported.

Use the explicit qualified character profile:

```sh
hexyl --character-table=ascii --color=never home://example.bin
hexyl --character-table=ascii --color=auto home://example.bin
```

ASCII selects the byte character panel; ordinary borders still use box glyphs.
Hexyl keeps its upstream colour default (`always`), `NO_COLOR` handling and
BrokenPipe exit-success behavior. `--color=auto` additionally uses stdout console
recognition and supplied TEXT colour variables such as `TERM=pyxis`. Forced
colour and plain output do not require terminal detection. Terminal width queries
only supplied stdout, stderr, then stdin, falling back to Hexyl's upstream 80
columns when no valid size is available. No ambient terminal is opened.

Local patches use standard NotSeekable errors for forward stream skipping,
propagate input failures, flush final buffered output explicitly, and query
native supplied-console dimensions. File input uses granted paths; absent or
denied inputs must fail. Stdin supports forward skipping by reading/discarding;
backward/end-relative stdin seeks fail. Network, subprocesses, workers, default
font/glyph additions and general Rust crate support are outside this port.
Build and runtime qualification are recorded by the parent milestone; these
recipe notes do not assert a successful build or guest run.
