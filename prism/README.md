# Prism

The Prism text preprocessor at the exact commit in `metadata.lua`, fetched as
a checksum-pinned archive from the owner's raw-github cache.

```sh
lua build.lua prism --sdk /path/to/build/sdk --spdlog /path/to/spdlog/dev
```

The recipe builds a static C++20 library and the upstream C11 callback bridge,
using the SDK's libc++, exceptions and RTTI. The installed `include/prism/`
tree includes the processor, AST, lexer and utility headers. Consumers include
`<prism/processor.h>`. `prism.pc` uses `sdk://` and declares libcxx and the
static spdlog dependency; the selected spdlog profile uses compiled spdlog
with its bundled fmt. No source-tree include paths are exported.

The standalone CLI, examples, debug parse option, sanitizers and upstream
CMake network fetches are excluded. This pin has no bundled `lib/strhash64`
source despite its CMake glob. No GSL or bz2 dependency is used.

Callers provide text, context storage, native callbacks and any include-loader
callback. No default filesystem loader or new authority is introduced.
Callback arguments are temporary; the bridge deletes them and its returned
heap result. Exceptions must not escape through the C callback bridge. The
package does not make the interpreter an untrusted-input sandbox.

`share/prism/source.txt` records the pin and profile. Upstream MIT notices,
original recipe files and MPL notices are retained under `share/`. The
statically linked consumer package must also retain spdlog/fmt notices.
