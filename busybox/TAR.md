# BusyBox tar adaptation

The tar executable uses the same pinned BusyBox source and GPL license as vi:
`f96d33d28a1f70fda5f27d221d5012b1ac0b7dad` (1.39.0.git).
Patch `0004-pyxis-tar-ustar-subset.patch` selects upstream `archival/tar.c`,
`archival/libarchive/get_header_tar.c` and
`archival/chksum_and_xwrite_tar_header.c`. Upstream header serialization,
octal fields, header checksums, member data padding and archive terminators
remain the tar implementation. `tar_support.h` supplies their selected libbb
interfaces and the header layout from upstream `include/bb_archive.h`;
`tar_pyxis.c` supplies the command interface, snapshots and native filesystem I/O.

```text
tar tf boot://share/manuals.tar
tar xf boot://share/manuals.tar
tar cf tmp://tree.tar tmp://tree
```

A leading dash on `tf`, `xf` or `cf` is accepted. Extraction uses the inherited
working directory. There is no `-C`, compression, archive stdin/stdout, member
selection or other tar option. Archive files and creation sources use libc
scheme-path resolution and the caller's existing grants. A scheme creation
operand stores its path after `://`: `tmp://tree/file` becomes `tree/file`.
Name a file or directory beneath a scheme root; a bare `tmp://` operand is refused.

## Archive validation and metadata

Only strict POSIX ustar (`ustar`, NUL, `00`) is accepted. The reader accepts the
ustar prefix field. The writer supports member names up to 99 bytes, plus the
trailing slash it adds to directory headers. Only regular files and directories
are supported. Links, devices, FIFOs, GNU/PAX extensions, sparse files, absolute
member paths, scheme-qualified member names and `..` path components are refused.
Malformed octal fields, incorrect header checksums, truncated data/padding,
missing terminator blocks and nonzero trailing archive blocks fail too.

The complete archive is read into memory and all members are validated before
listing or extraction. Canonical relative member names also reveal conflicts
between files, directories and their descendants before extraction writes.
Extraction uses that same immutable byte snapshot, so modifying the archive
path after validation does not replace the bytes being extracted.

Extracted owner, group, permissions and timestamps are ignored. Created headers
use uid/gid zero, mode 0644 for regular files or 0755 for directories, and mtime
zero; owner/group names are empty. Native object authority continues to come
from grants, with file creation using libc's supported 0666 policy argument.

## Writes and limits

Creation walks native directories and reads all source file contents before
opening the output archive. Unsupported or unreadable sources therefore fail
before output creation/truncation. Source bytes stay captured even if the output
path aliases a source. Directory enumeration is live: a detected concurrent
change fails with libc's error; this is not a filesystem-wide atomic snapshot.

Extraction creates missing parent directories and overwrites existing regular
files, matching ordinary tar behavior. Extracting into a read-only working root
fails on the first required mutation without creating partial files. There is
no archive-wide rollback: a later storage error or differing authority in a
nested destination can leave previously extracted entries or a partially written
file. Output archive writes are also not atomic. Diagnostics and nonzero status
report read, write, allocation, enumeration and close failures.

Memory must hold the whole input archive, or all creation file contents, plus
member/path bookkeeping. Directory traversal is recursive. These limits are
accepted for the initial documentation-archive use; larger archive streaming or
transactional extraction requires a separate design.
