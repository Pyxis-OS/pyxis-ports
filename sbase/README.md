# sbase cksum

[Upstream sbase](https://git.suckless.org/sbase) is pinned to
`c546c3a5724c81cee9a11d816a38ccdf17472129`. This recipe builds only cksum,
libutil/eprintf.c and libutil/fshut.c. It stages `bin/cksum.pxe`, the complete
MIT license/contributor list and arg.h's individual notice under
`share/licenses/sbase`. Retain both notice files with the executable.

The single patch narrows private util.h to the declarations used by those
translation units, retaining its license notice. It avoids unrelated regex,
mode and offset declarations. Command/helper bodies and arg.h are unchanged;
open/read/close, stdio output, BUFSIZ and PRIu32 come from the real Pyxis SDK.
No compatibility headers are installed, and the SDK is never modified.

The shell resolves `cksum` to `app://cksum.pxe`:

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

Cksum emits a result only after EOF. Pyxis console input currently has no EOF
operation, so use a finite file redirect or pipeline for stdin. Positive short
reads are accumulated by the upstream loop. Broken pipe is reported through
stdio's EPIPE/error indicator without a SIGPIPE facility. Input close results
are ignored by upstream; libc still invalidates the descriptor under its close
policy. The existing signedness warning in the byte-processing loop is retained.

No other sbase command is built. Tee's writable-open, creation-mode, append and
signal requirements remain a separate scope decision after cksum acceptance.
