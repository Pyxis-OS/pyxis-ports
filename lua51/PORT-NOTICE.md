# Lua 5.1 port notice

PUC Lua 5.1.5, LPeg 1.1.0, luv 1.52.1-0 and lua-compat-5.3 0.13 are
pinned by archive SHA-256 in `metadata.lua`. The bundle records those upstream
URLs, hashes and ordered patch names in `app/metadata/source.txt`. This is
provenance, not a source-code distribution.

Lua, LPeg and lua-compat-5.3 retain MIT notices; luv retains Apache-2.0.
Their notices travel in `app/metadata/licenses/lua51/`. The statically linked
native libuv retains MIT, `LICENSE-extra` and its port notice under
`app/metadata/licenses/libuv/`. SDK libc's TLSF, musl and TRE notices and the
compiler runtime's LLVM notice are copied into the same metadata tree.
Development prefixes retain their own copies for static-library consumers.

The ordered patches adapt Lua's standard libraries to supported native libc
operations, omit interactive mode and dynamic modules, and preload LPeg/luv.
The luv patch uses native exit observations and rejects unsupported socket,
watch, signal, PID and worker operations. The compat-5.3 patch removes the
unsupported binary-mode `freopen` operation. Upstream-derived changes retain
the licences of their files; original recipe and documentation are MPL-2.0
under the packaged `licenses/MPL-2.0` notice. Native libuv adapter files also
retain their MPL-2.0 notices.
