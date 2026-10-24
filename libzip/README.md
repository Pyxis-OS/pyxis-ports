# libzip

libzip 1.12.1, under BSD-3-Clause, builds as a static library with store and
deflate compression through the separately selected zlib development prefix.
All encryption, including built-in PKWARE and AES, is unsupported. Other
compressors, crypto dependencies, command-line tools, examples, documentation
and upstream regression/fuzzing programs are omitted. The recipe downloads
only the approved release cache and checks its SHA-256.

```sh
lua build.lua libzip --sdk /path/to/pyxis/build/sdk \
  --zlib /path/to/zlib/stage/dev
```

The development prefix contains `lib/libzip.a`, `zip.h`, generated `zipconf.h`,
the installed CMake package and `libzip::zip` target, and `libzip.pc` using
`sdk://` paths with a private zlib dependency. CMake consumers use
`find_package(libzip CONFIG REQUIRED)` and link `libzip::zip`, searching both
development prefixes. Direct consumers link libzip.a before libz.a and the SDK
runtime. Headers and archive use the same native configuration.

The native adaptation keeps FILE, descriptors, paths, temporary creation and
rename in libc. Pyxis has no Unix permission metadata: native file sources
advertise no invented permission/creator-system attributes and perform no
fchmod operation. Creation's 0666 argument selects libc's native creation
policy, without claiming Unix permissions. Existing ZIP attributes are retained;
new ZIP entries keep libzip's format defaults unless the caller sets attributes.
Native stat modification times are present only when `STAT_MTIME_VALID` is set;
memory buffers have no intrinsic modification time. A failed DOS/local calendar
conversion leaves `ZIP_STAT_MTIME` absent. Inspect validity flags before reading
metadata. No native inode, device or mode is fabricated.

Every archive open requires native random and clock grants for libzip's keyed
filename hash. Entropy uses libc getentropy, with real failure and no device
path, rand or time-seeded fallback. Temporary names use fallible secure entropy
before exclusive creation. A memory-source read does not separately read the
current timestamp. When a new ZIP entry lacks a supplied timestamp, writing
reads a checked native clock and reports failure if unavailable. Callers can
set explicit valid DOS timestamps instead; calendar conversion can also require
the configured TZ data. Grant file and parent-directory authority for the
selected native paths. Permission, entropy, timestamp, write, close and rename
failures are reported through libzip's existing errors; some generic internal
errors do not carry the underlying native errno.

File replacement uses a sibling temporary file, checked output close and
rename. The parent must already exist. No file sync is added, so a successful
close is not a power-loss durability promise. Rollback cleanup can itself fail
under denied or uncertain native operations; callers must inspect scratch files
after an error. Host sharing has its native no-symlink lookup contract.

`share/licenses/libzip` retains upstream BSD and original Pyxis MPL notices;
`share/libzip/source.txt` records the source and profile. Corresponding recipe,
patches and public manual-consumer source are in `share/sources/libzip`.
Preserve these with the development package and zlib's notices when distributing
a linked consumer. See [manual/README.md](manual/README.md) for the independent
installed-prefix consumer. A successful build establishes link closure only.
