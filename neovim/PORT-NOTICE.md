# Neovim port notice

Neovim 0.12.5 is pinned to commit
`5885a30e1e1225349079e7a1c4a3848aa8e43e42`. Its release archive has SHA-256
`314bb8d8695cc2c1c9b69e6c93df8c75108ca66588dfffb81c42369d2f85c90a`.
The source URLs, owner mirrors and archive hashes are recorded in
`metadata.lua` and `share/neovim/source.txt`.

Neovim retains Apache-2.0 and Vim terms in upstream `LICENSE.txt`. That notice
also identifies independently licensed vendored components: xdiff source
headers retain LGPL-2.1-or-later and `src/xdiff/COPYING` contains LGPL-2.1;
mpack, lua-bitop, lua-cjson, klib, libtermkey and libvterm retain MIT notices.
Individual runtime files retain their own notices. These terms are not
replaced by the Pyxis licence.

The separately pinned target dependencies retain their recipe notices: Lua
5.1.5, LPeg 1.1.0 and lua-compat-5.3 0.13 are MIT; luv 1.52.1-0 is Apache-2.0;
libuv 1.52.1 is MIT with its `LICENSE-extra`; utf8proc 2.11.3 retains MIT and
Unicode data terms; tree-sitter 0.26.13 retains MIT and Unicode-DFS-2016 terms.
Native libuv adapter files retain their MPL-2.0 notices. Host Lua and LPeg
build inputs retain MIT and are separate from target archives.

The statically linked grammars use Neovim 0.12.5's exact dependency pins:
tree-sitter-c 0.24.1, tree-sitter-lua 0.5.0, tree-sitter-vim 0.8.1 and
tree-sitter-markdown 0.5.3 retain MIT terms; tree-sitter-vimdoc 4.1.0 and
tree-sitter-query 0.8.0 retain Apache-2.0 terms. The Markdown archive supplies
both `markdown` and `markdown_inline`. Each archive's original licence is
staged separately under `share/licenses/neovim`; its URL, mirror, version,
licence and SHA-256 are recorded in `share/neovim/source.txt`. Generated C
parsers and scanners are compiled directly, without fetching or regenerating
grammars during the target build.

Local changes are recorded as six ordered upstream-derived patches, retaining
the licences of the files they modify:

1. `0001-pyxis-platform.patch`: native process observation/termination, explicit
   internal child grants, one-thread event loops, and rejected Unix process,
   socket, signal and PTY paths.
2. `0002-pyxis-files.patch`: native scheme paths and proved realpath, validity-aware
   metadata, retained file references and checked overwrite; unavailable
   swap/backup operations reject explicitly.
3. `0003-pyxis-tui.patch`: native console streams, RAW passthrough lifetime,
   generation-based resize, the 16-colour terminal profile and suppressed
   terminal probes/query waits.
4. `0004-pyxis-core.patch`: Pyxis build/runtime selection, static Lua module
   loading, native home/runtime paths, restricted internal channels and omitted
   automatic listeners and PID metadata.
5. `0005-pyxis-undo-mode.patch`: persistent undo files are created with mode
   0666 instead of the edited file's permission bits.

6. `0006-pyxis-static-parsers.patch`: built-in language registration, static grammar
   linking, and quiet legacy-syntax fallback when a parser is unavailable.

Original recipe, host wrapper, configuration, colourscheme and documentation
are MPL-2.0 under the ports repository `LICENSE`. The host help-tag wrapper
uses real GNU find directory enumeration with NUL-framed names/types, then runs
the unmodified upstream help-tag generator. Host generation adds no target
module-loading or Unix compatibility interface.
