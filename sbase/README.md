# sbase commands

Pinned sbase cksum, tee, uniq, sha256sum, wc, tail, sort and grep, built with the
Pyxis SDK. Source pin and internal mirror are in metadata.lua.

```sh
lua build.lua sbase --sdk /path/to/sdk --work /path/to/work
```

The eight executables are staged under `stage/bin`. Preserve the staged
MIT/contributor, ISC, BSD and Unicode notices. SHA-256 uses upstream's
public-domain implementation; its source notice is staged with the others.

```sh
cat host://input | tee -a home://log | cksum
grep -n main host://notes.c
grep -Ei 'error|warn' host://log.txt home://other.txt
cat host://words | grep -v '^#' | grep -c .
tail -n 20 home://log.txt
tail -f home://log.txt
sha256sum host://image.raw
```

The patches add these native seams and fixes:

- `util.h` uses libc's `strcasestr` instead of an unselected sbase helper.
- Grep uses a small newline-pattern splitter while libc lacks `fmemopen`.
  Output uses ordinary buffered FILE operations, flushing matched terminal/stdin
  lines and at file completion. The recursive-search extension is removed;
  unsupported options, matching, counts and failures follow upstream.
- `wc -c` counts an owned pipe FILE value by its metadata size, adopting and
  closing that value, while ordinary byte runs use `fread_some`. Other modes
  retain upstream decoding and counts.
- `tail -F` follows a name (chronium, 2026-11-02). After each one-second poll it
  compares the name's `st_dev`/`st_ino` with the open file. When the name refers
  to a different file, tail prints what the old file still holds, prints
  `NAME: file replaced` and follows the new file from its start; a removed name
  is reported once while the open file is still read until the name returns.
  Without valid identity bits on both sides a replacement cannot be proven and
  the open file is kept. A name missing at startup is an error, and stdin is not
  reopened.
- `sort -o FILE` reports a failed write or close of FILE with status 2, as
  upstream does for stdout.

Tee, tail, hashing, uniq, sort and cksum retain upstream behavior. Tee `-a` uses
libc `O_APPEND` and native FILE APPEND, atomic per operation of at most 4,080
bytes. Larger writes may interleave; neither append nor close implies storage
synchronization. Unsupported append targets refuse. Tee `-i` calls the
honest unsupported libc `signal` interface and fails when it returns `SIG_ERR`.
Tail `-f` retains upstream's one-second polling with real read/fstat/sleep calls;
no filesystem notification API is required by that algorithm. A truncated file
is reported and read again from its start, as upstream and GNU tail do. Writes onto input aliases retain upstream's data-loss behavior.

Wc, tail, sort and grep read terminal stdin until EOF. The framebuffer console
has no EOF key, so use files or a finite pipeline there. Remote terminals can
supply EOF. No new signals, termios or filesystem-wait mechanisms are supplied
by these ports.
