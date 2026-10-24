# stb

stb_image 2.29, pinned to commit
`0bc88af4de5fb022db643c2d8e549a0927749354`, is the selected image header from
stb. The recipe downloads only the checksum-pinned archive from the owner's
raw-github proxy; other stb libraries are not installed.

Use `pkg-config --cflags --libs stb` with the exported SDK. Metadata names the
merged `sdk://` view. Include `stb_image.h` for declarations and define
`STB_IMAGE_IMPLEMENTATION` before including it in exactly one implementation
translation unit. There is no implementation archive. Libultraship can keep its
own implementation source without linking a second copy.

Feature macros are unchanged and caller-selected. Stdio, all upstream image
formats, HDR/linear conversion, allocator, TLS and SIMD defaults remain intact.
Use the SDK's ordinary architecture baseline rather than host native-CPU flags.

The root license matches the full license embedded in the installed header.
Notices and provenance are installed under `dev/share/licenses/stb` and
`dev/share/stb`, including complete original Pyxis recipe source. The upstream
programs and tests are not built.
