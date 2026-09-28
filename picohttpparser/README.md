# picohttpparser

HTTP header parsing and chunked decoding from upstream commit
`f4d94b48b31e0abae029ebeafcfd9ca0680ede58`, used under its MIT license.
The complete notice is retained in `picohttpparser.h` and staged at
`share/licenses/picohttpparser/picohttpparser.h`.

```sh
lua build.lua picohttpparser --sdk /path/to/pyxis/build/sdk
```

The recipe builds unmodified upstream `picohttpparser.c` against the exported
SDK, using its baseline x86-64 flags without SSE4.2. The SDK already provides
the required `assert.h`, `stddef.h`, `stdint.h`, `string.h` and `sys/types.h`;
the archive's runtime dependencies are `memmove` and `__assert_fail` from libc.
No compatibility headers, local patches or upstream tests are included.

Development outputs are `stage/dev/lib/libpicohttpparser.a` and
`stage/dev/include/picohttpparser.h`. Pyxis stages these separately at
`build/ports-dev/picohttpparser`; consumers link against the same SDK used to
build the archive. Only the license enters the boot archive.

This is a parser, not an HTTP client. Callers own sockets, deadlines, framing
policy, field and byte limits, content decoding policy and response storage.
Header slices borrow the input buffer. Chunk decoding edits the supplied
buffer in place and requires a zero-initialized decoder for each response.
