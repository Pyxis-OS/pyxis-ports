# BusyBox tar adaptation

The tar executable uses pinned BusyBox commit
`f96d33d28a1f70fda5f27d221d5012b1ac0b7dad` (1.39.0.git), retaining its GPL
license. Patch `0004-pyxis-tar-ustar-subset.patch` selects upstream tar creation,
header reading/writing and unsafe-prefix stripping. The native support header
provides the selected libbb interfaces and archive fields;
`tar_pyxis.c` supplies the standalone entry and native filesystem callbacks.
The CLI, option parsing, recursion, member selection, listing and archive-stream
control use upstream BusyBox code. `libbb/getopt32.c` uses libc getopt; no
replacement parser or competing optind is linked.

```text
tar tf boot://share/manuals.tar
tar xf boot://share/manuals.tar
tar cvf tmp://tree.tar tmp://tree
```

Upstream short-option handling accepts the optional leading dash, `c`/`t`/`x`,
`-f ARCHIVE`, `-C DIR`, `-v`, `-O`, `-k` and member operands. Archive `-` uses
stdin/stdout. Compression, include/exclude files and long options remain
unselected. Extraction uses the inherited working directory or `-C DIR`.
Creation operands use libc scheme-path lookup and existing grants. A scheme
operand stores the part after `://`: `tmp://tree/file` becomes `tree/file`.
Traversal is the upstream recursive walker over native libc stat/opendir.

## Formats, paths and metadata

The selected reader accepts upstream `ustar` magic and numeric-field parsing,
including its base-256 representation. It accepts the ustar prefix field.
Upstream empty-header termination and trailing-data behavior apply; there is no
extra all-zero-block or exact-magic/version validation. Bad checksums and short member reads follow upstream error handling. The writer retains upstream GNU-compatible ustar magic
and GNU long-name records: a member name or link target over 99 bytes is written as
a `././@LongLink` record and read back the same way, so no name length limits what
`tar` archives (GNU tar, bsdtar and Python read the records).

Regular files (including the archive's contiguous-file type), directories and
hardlinks are supported. Symlinks, devices and FIFOs cannot be created on native
filesystems. GNU long-name records are the only GNU extension enabled; the upstream reader skips PAX
metadata under this profile rather than applying extended fields. Directory size and otherwise unused metadata follow
upstream handling. Extraction ignores owner, group, mode and timestamps; native
mutation authority comes from grants. Created headers use uid/gid zero, mode
0644/0755 and empty owner/group names, since native files have no Unix owner or
permission metadata. A valid nonnegative native mtime supplies the header time;
otherwise the header uses zero. File creation uses libc's supported 0666 mode.

Upstream removes absolute and parent-containing unsafe name prefixes. The
native confinement check then rejects a remaining scheme-qualified name,
absolute name, empty name or `..` component before filesystem mutation. Native
lookup does not follow symlinks. Extraction remains under the inherited working
root and cannot acquire new root authority from archive bytes.

## I/O and limits

Listing and extraction stream one owned archive descriptor through upstream
header/action callbacks. There is no whole-input snapshot or conflict preflight:
an error in a later member can leave earlier extracted files, and malformed
input can leave a partially written current file. Extraction creates missing
parents and uses upstream default unlink/create replacement (parent REMOVE and
CREATE authority); `-k` keeps existing names. `-O` streams member contents to stdout. There is
no archive-wide rollback or transactional output publication.

Creation opens each regular source and streams its recorded size through an
owned descriptor and a bounded copy buffer. The upstream walker discovers entries as it recurses. Changes after discovery
follow the upstream exact-size copy behavior; an early EOF fails and can leave
partial output. The upstream self-archive check compares the held output
descriptor's fstat identity against each source only when both native identity
fields are valid. Hardlink tracking likewise requires valid identity and link
count; unknown metadata never becomes a fabricated zero identity. This does not
provide a snapshot or pre-truncation alias guarantee: use a separate output path. Directory enumeration is live, and creation uses
recursive traversal.
