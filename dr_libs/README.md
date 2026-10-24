# dr_libs

The immutable snapshot `da35f9d6c7374a95353fd1df1d394d44ab66cf01` provides
dr_wav 0.13.16, dr_mp3 0.6.39 and dr_flac 0.12.42. The recipe downloads only
the checksum-pinned archive from the owner's raw-github proxy.

Use `pkg-config --cflags --libs dr_libs` with the exported SDK. Metadata names
the merged `sdk://` view. Include the headers normally for declarations; define
the appropriate `DR_*_IMPLEMENTATION` before including each header in exactly
one implementation translation unit. The package supplies no implementation
archive, so it can coexist with SoH's existing implementation owner.

Headers and feature macros remain unchanged and caller-selected. Default
stdio, wide filename, conversion, CRC, container and SIMD behavior are retained.
On x86_64 the SDK's ordinary architecture baseline permits SSE2; do not use host
native-CPU flags. Allocators and FILE operations use the standard Pyxis libc.

The root license and each header's complete license tail are installed under
`dev/share/licenses/dr_libs`, including minimp3's dedication. Provenance and
the complete original Pyxis recipe source live under `dev/share/dr_libs`.
The upstream tests and their miniaudio submodule are not fetched or built.
