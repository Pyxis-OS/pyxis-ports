# sbase cksum and tee

[Upstream sbase](https://git.suckless.org/sbase) is pinned to
`c546c3a5724c81cee9a11d816a38ccdf17472129`. This recipe builds cksum and a
restricted tee, plus their eprintf/fshut/ealloc/writeall helpers. It stages
`bin/cksum.pxe`, `bin/tee.pxe`, the complete
MIT license/contributor list and arg.h's individual notice under
`share/licenses/sbase`. Retain both notice files with the executable.

Patch 0001 narrows private util.h for cksum; 0002 adds tee's helper declarations
and its option/lifetime adaptation. The header retains its license notice and
avoids unrelated regex and offset APIs. Cksum, helper bodies and arg.h are
unchanged. Conventional I/O, BUFSIZ and PRIu32 come from the real Pyxis SDK.
No compatibility headers are installed, and the SDK is never modified.

## Cksum

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

## Tee

The shell resolves `tee` to `app://tee.pxe`:

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

Terminal input can be forwarded as it arrives, but cannot finish through EOF
until the console protocol supplies that operation. No other sbase tools are
built, and no kernel or signal interface is introduced.
