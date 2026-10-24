# Public manual consumer

Build only against installed libzip/zlib prefixes, from the ports root:

```sh
make -j4 -f libzip/manual/Makefile SDK=/path/to/sdk \
  LIBZIP=/path/to/libzip/stage/dev ZLIB=/path/to/zlib/stage/dev \
  RECIPE="$PWD/libzip" BUILD="$PWD/build/libzip-manual"
```

An independent target link establishes package closure. Actual behavior remains
unqualified until the P1F consumer runs in Pyxis. No ROM, assets or network input
is used. With native random/clock grants, the default invocation writes a public
memory ZIP with an explicitly authored DOS date, reopens it read-only, checks
deflate data/EOF and size/method, checks unknown memory-source mtime, and verifies
that malformed ZIP input, BZip2 and PKWARE encryption are refused.

An optional argument selects one public scratch ZIP file, for example
`tmp://libzip-public.zip`. It deliberately creates/replaces that file, adds one
public entry without an explicit timestamp, closes via native temporary-file
creation/rename, reopens and reads it back. The finite writer requires the
selected file/parent rights, random and clock authority, and any configured TZ
data. The parent directory must exist. Inspect the resulting ZIP and any sibling
`.part` file after an error; close success does not promise storage durability.

Repeat with an unavailable root, denied creation/rename/removal, missing random
authority, and invalid TZ. Native failures must remain failures; metadata without
a valid conversion must remain marked unknown. The program prints the library
error and returns nonzero when it cannot complete the selected operation.
