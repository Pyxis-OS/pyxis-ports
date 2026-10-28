# Neovim port notice

Neovim 0.12.5 is pinned to commit
`5885a30e1e1225349079e7a1c4a3848aa8e43e42`. Its release archive has SHA-256
`314bb8d8695cc2c1c9b69e6c93df8c75108ca66588dfffb81c42369d2f85c90a`.
The source URLs, owner mirrors and archive hashes are recorded in
`metadata.lua` and the bundle's `app/metadata/source.txt`. The packaged source
text records provenance, not a source-code distribution.

Neovim retains Apache-2.0 and Vim terms in upstream `LICENSE.txt`. That notice
also identifies independently licensed vendored components: xdiff source
headers retain LGPL-2.1-or-later and `src/xdiff/COPYING` contains LGPL-2.1;
mpack, lua-bitop, lua-cjson, klib, libtermkey and libvterm retain MIT notices.
Individual runtime files retain their own notices. These terms are not
replaced by the Pyxis licence.

The separately pinned target dependencies carry notices and provenance under
`app/metadata/licenses/`: Lua
5.1.5, LPeg 1.1.0 and lua-compat-5.3 0.13 are MIT; luv 1.52.1-0 is Apache-2.0;
libuv 1.52.1 is MIT with its `LICENSE-extra`; utf8proc 2.11.3 retains MIT and
Unicode data terms; tree-sitter 0.26.13 retains MIT and Unicode-DFS-2016 terms.
Native libuv adapter files retain their MPL-2.0 notices. Host Lua and LPeg
build inputs retain MIT and are separate from target archives. SDK libc's
TLSF, musl and TRE notices and compiler-runtime LLVM terms travel there too;
the independently distributed SDK/library notices remain available.

The statically linked grammars use Neovim 0.12.5's exact dependency pins:
tree-sitter-c 0.24.1, tree-sitter-lua 0.5.0, tree-sitter-vim 0.8.1 and
tree-sitter-markdown 0.5.3 retain MIT terms; tree-sitter-vimdoc 4.1.0 and
tree-sitter-query 0.8.0 retain Apache-2.0 terms. The Markdown archive supplies
both `markdown` and `markdown_inline`. Each archive's original licence is
staged separately under `app/metadata/licenses/neovim`; its URL, mirror, version,
licence and SHA-256 are recorded in `app/metadata/source.txt`. Generated C
parsers and scanners are compiled directly, without fetching or regenerating
grammars during the target build.

Local changes are recorded as nine ordered upstream-derived patches, retaining
the licences of the files they modify:

1. `0001-pyxis-platform.patch`: native process observation and termination,
   explicit internal child grants (including optional SYSTEM_INFO READ), attached
   external jobs, one-thread event loops and rejected Unix process/signal paths.
2. `0002-pyxis-files.patch`: native scheme paths and proved realpath,
   validity-aware metadata, retained file references, checked overwrite,
   proved-parent selection and executable probes over the held `path` list.
   Unavailable swap/backup operations reject explicitly.
3. `0003-pyxis-tui.patch`: native console streams, RAW passthrough lifetime,
   generation-based resize, the 16-colour terminal profile, underline and
   suppressed unsupported terminal probes. Native writes retain borrowed request
   and buffer storage through completion.
4. `0004-pyxis-core.patch`: Pyxis build/runtime selection, static Lua module
   loading, native home/runtime paths, attached pipe jobs and omitted automatic
   listeners and PID metadata. Static grammars link in the native build.
5. `0005-pyxis-undo-mode.patch`: persistent undo files are created with mode
   0666 instead of the edited file's permission bits.
6. `0006-pyxis-static-parsers.patch`: built-in language registration and grammar
   lookup with actual ABI checks. Missing-parser behavior follows upstream.
7. `0008-pyxis-file-selection.patch`: native origin lifetime on read and save.
8. `0010-pyxis-file-uris.patch`: file URIs carry the native root name as their
   first segment (`home://a.c` is `file:///home/a.c`), including server root views.
9. `0011-pyxis-language-servers.patch`: language servers stay attached; resource
   views copy the actual admitted process selection and retain each buffer's
   read-only root.

Original recipe, host wrapper, configuration, runtime Lua (`lsp/clangd.lua`,
`lua/vim/_pyxis_roots.lua`), colourscheme and documentation
are MPL-2.0 under the packaged `app/metadata/licenses/MPL-2.0` notice. The host help-tag wrapper
uses real GNU find directory enumeration with NUL-framed names/types, then runs
the unmodified upstream help-tag generator. Host generation adds no target
module-loading or Unix compatibility interface.

The linked libuv closure also retains complete notice-bearing tree.h
(BSD-2-Clause), heap-inl.h and queue.h (ISC) under the bundle's libuv notices.
