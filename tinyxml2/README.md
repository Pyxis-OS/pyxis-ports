# tinyxml2

Tinyxml2 10.0.0, pinned to commit
`321ea883b7190d4e85cae5512a12e5eaa8f8731f`, builds its unchanged static library
against the exported Pyxis SDK and libc++. The recipe downloads only the
checksum-pinned archive from the owner's raw-github proxy.

The development prefix provides `tinyxml2.h`, `libtinyxml2.a`, upstream CMake
targets and `tinyxml2.pc`. Use `find_package(tinyxml2 CONFIG REQUIRED COMPONENTS
static)` with `tinyxml2::tinyxml2`, or `pkg-config --cflags --libs tinyxml2`.
Metadata names the merged `sdk://` view and preserves `_FILE_OFFSET_BITS=64`.

Memory parsing and standard seekable-file APIs are included. Upstream SaveFile
does not report output or close failures; its success is not a complete-save
or durability guarantee. See `PORT-NOTICE` before using it to publish data.

Notices and provenance are installed under `dev/share/licenses/tinyxml2` and
`dev/share/tinyxml2`. The latter includes the complete Pyxis recipe source.
Shared libraries and upstream test programs are not built.
