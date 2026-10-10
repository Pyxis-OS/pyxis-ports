# utf8proc

utf8proc 2.11.3 as a static library for Neovim. The release archive is
checksum-pinned in `metadata.lua` and fetched only from the owner's
`raw-github` mirror.

```sh
lua build.lua utf8proc --sdk /path/to/build/sdk
```

The recipe compiles unmodified upstream `utf8proc.c` with the shipped Unicode
tables and the SDK's target flags. It stages `dev/lib/libutf8proc.a` and
`dev/include/utf8proc.h`; consumers define `UTF8PROC_STATIC` and link the
archive before the SDK runtime libraries. No shared library, host tool, data
regeneration or upstream test program is built.

`share/utf8proc/source.txt` records the pin and profile.
`share/licenses/utf8proc/LICENSE.md` retains the upstream MIT and Unicode data
notices. The library's Unicode classification, normalization and width APIs do
not add Unicode input or rendering to Pyxis's terminal.
