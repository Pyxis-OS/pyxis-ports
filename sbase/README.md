# sbase cksum, tee, uniq and sha256sum

[Upstream sbase](https://git.suckless.org/sbase) is pinned to
`c546c3a5724c81cee9a11d816a38ccdf17472129`. This recipe builds cksum, a
restricted tee, uniq and sha256sum, plus their eprintf/fshut/ealloc/writeall/
strtonum/crypt/sha256 helpers. It stages `bin/cksum.pxe`, `bin/tee.pxe`,
`bin/uniq.pxe`, `bin/sha256sum.pxe`, the complete MIT license/contributor list,
arg.h's individual notice and `libutil/strtonum.c` with its OpenBSD ISC notice
under `share/licenses/sbase`. `libutil/sha256.c` is marked public domain and
needs no separate notice. Retain the notice files with the executables.

Patch 0001 narrows private util.h for cksum; 0002 adds tee's helper declarations
and its option/lifetime adaptation. Patch 0003 restores upstream util.h's
`compat.h` include (for `INT_MAX`) and strtonum declarations for uniq, unchanged
from upstream. The header retains its license notice and avoids unrelated regex
and offset APIs. Cksum, uniq, sha256sum, helper bodies and arg.h are unchanged.
Conventional I/O, `getline`, `isblank`, BUFSIZ and PRIu32 come from the real
Pyxis SDK. No compatibility headers are installed, and the SDK is never modified.

## Cksum

The shell resolves `cksum` to `bin://cksum.pxe`:

```text
cksum host://hello.c
cat host://hello.c | cksum
cksum < host://hello.c > home://checksum.txt
```

Relative input names use the inherited working directory; explicit capability
paths use existing grants. With no input operands, cksum reads stdin. A `-`
operand reads the same stdin and labels its output `<stdin>`, matching this
upstream version.
Results contain CRC and byte count, plus the input name for named operands.
Missing or unreadable inputs report errors and processing continues with later
operands; input or detected output errors produce a nonzero exit status.

Cksum emits a result only after EOF. The framebuffer console has no EOF
operation, so use a finite file redirect or pipeline for stdin there.
Independent terminal sessions, including remote `END_INPUT`, deliver EOF;
Ctrl+D is an ordinary byte. Positive short reads are accumulated by the
upstream loop. Broken pipe is reported through stdio's EPIPE/error indicator
without a SIGPIPE facility. Input close results are ignored by upstream; libc
still invalidates the descriptor under its close policy. The existing signedness
warning in the byte-processing loop is retained.

## Tee

The shell resolves `tee` to `bin://tee.pxe`:

```text
cat host://input | tee home://first home://second | cksum
```

Tee copies stdin to stdout and each named output. Named outputs use
O_WRONLY|O_CREAT|O_TRUNC with 0666; creation and truncation follow the caller's
grants. In the current SDK, 0666 requests native creation policy and promises no
Unix permission bits. No permission or user model is added by this port.

Only file operands are accepted. Options -a and -i fail before opening outputs;
there is no non-atomic O_APPEND approximation or fake signal handler. Use `--`
before an operand beginning with a dash. This is a restricted upstream port,
not the full POSIX tee interface.

Patch 0002 validates stdin and snapshots stdout availability with zero-length
calls before opening files. Thus reuse of an absent standard slot cannot make a
named output act as stdout. Missing stdin fails before any file is touched;
missing stdout is an error but named outputs can still work.

Failed outputs are diagnosed and closed once while surviving outputs continue.
Once all outputs have failed, tee stops reading and closes stdin so an upstream
writer can observe closure. On EOF or input error it closes stdin and all live
outputs, reporting close failures without retrying. Detected open/read/write/
close errors produce a nonzero status. A broken stdout does not prevent named
outputs from receiving the rest of the input. Writeall retains its upstream
loop over positive short writes. Output can be partial after an error; opening
an output that aliases the input can destroy its contents, as in upstream tee.

Terminal input is forwarded as it arrives. Framebuffer console input cannot
finish through EOF; independent terminal sessions can, as for cksum.

## Uniq

The shell resolves `uniq` to `bin://uniq.pxe`:

```text
uniq host://input
cat host://input | uniq -c
uniq -d -f 1 host://input home://duplicates
uniq -u - home://unique < host://input
```

Uniq writes each run of adjacent identical lines once. It does not sort input or
detect non-adjacent repeats. The upstream options are supported: `-c` prefixes
counts, `-d` prints only repeated lines, `-u` prints only unrepeated lines,
`-f N` skips N blank-separated fields and `-s N` skips N further characters
before comparing. Both `-d` and `-u` together print nothing, as upstream.
Operands are `[input [output]]`; `-` selects stdin or stdout. A named output is
opened with `fopen(..., "w")`, creating or truncating it through the caller's
grants after the input opens successfully.

Field skipping uses libc `isblank`, so only ASCII space and tab separate fields.
Lines are compared as bytes, including embedded NULs; a final line without a
newline is compared and written without one. Lines are read with libc
`getline`, which issues one native read per byte because streams are unbuffered;
large inputs are slow, especially from `host://` or native filesystems.

A missing input or unopenable output reports an error and exits 1 before any
output. A read error stops input immediately; upstream `fshut` then reports it
with status 1. Output errors, including a closed output pipe, do not stop the
input loop: uniq reads to EOF, then `fshut` reports them with status 1. There is
no SIGPIPE. Framebuffer console input has the same EOF limit as cksum. GCC's
`loff` may-be-uninitialized warning in upstream code is a false positive and is
retained.

## Sha256sum

The shell resolves `sha256sum` to `bin://sha256sum.pxe`:

```text
sha256sum host://image.raw
cat host://input | sha256sum
sha256sum a b c > home://SHA256SUMS
sha256sum -c home://SHA256SUMS
```

Each operand prints its lowercase SHA-256 digest, two spaces and its name; stdin
is labelled `<stdin>`, including an explicit `-`. Unopenable operands are
reported and later operands continue, with status 1. `-b` and `-t` are accepted
and ignored, as upstream. Files are hashed through `read` in BUFSIZ blocks, not
byte by byte.

With `-c`, each operand (or stdin, or `-`) is a manifest of `digest  name` or
`digest *name` lines; trailing CR/LF is stripped. Listed names are opened
relative to the working directory. It prints `name: OK` or `name: FAILED`, then
reports counts of malformed lines, unreadable files and mismatches, each giving
status 1. An unopenable manifest is reported and skipped with status 1.
Manifest lines use libc `getline`, one native read per byte, which matters only
for very large manifests. A listed file whose read fails keeps its descriptor
open until exit, as upstream. Each digest is written with unbuffered `printf`
calls. The existing signedness warning in `libutil/crypt.c` is retained.

No other sbase tools are built, and no kernel or signal interface is introduced.
