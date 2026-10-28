# Lua 5.1 with LPeg and luv

PUC Lua 5.1.5 with its standard libraries, LPeg 1.1.0, luv 1.52.1-0 and
lua-compat-5.3 0.13, built as one recipe over the [native libuv](../libuv/README.md).
These are the pins Neovim 0.12.5 uses. The Lua 5.5 port in `lua/` is separate;
the two coexist. This interpreter has no native `pyxis` module.
Ordinary `io.open` reads consume HTTP streams through libc. A staged HTTP
`"w"` or `"w+"` file commits on an explicit file flush or final owning close;
`"w+"` then reads its reply from the same file and rejects later writes.
Garbage collection abandons pending requests without submission. A successful
standalone interpreter shutdown closes its Lua state normally; an unhandled
Lua error selects abandonment. `os.exit(status)` follows upstream process exit
without forcing `lua_close`. Explicitly check flush/close results; automatic
cleanup adds no Lua diagnostics or exit-status aggregation. These operations
use existing declared grants. See the [Lua HTTP stream examples](../lua/README.md#http-files);
Lua 5.1 has no lexical-scope `__close` support.

Embeddings call `lua_pyxis_io_shutdown(state, normal)` immediately before
`lua_close(state)` to select normal publication or abandonment. No caller
pointer is borrowed. Without this hook, GC abandons pending requests. The hook
is harmless when `io` was not opened.

```sh
lua build.lua lua51 --sdk /path/to/build/sdk --libuv /path/to/libuv/stage/dev
```

The recipe downloads only the owner mirrors and verifies each SHA-256 before
applying patches. It stages:

- `bin/lua5.1.pxb`, the interpreter bundle;
- `dev/lib/liblua5.1.a`, `liblpeg.a` and `libluv.a` (with compat-5.3's C API),
  headers under `dev/include/lua5.1/` and `dev/include/luv/`, for Neovim;
- notices and source provenance in the bundle's `app/metadata/`, including
  linked libuv and SDK runtime notices under `licenses/`;
- independent library notices under `share/licenses/lua51/` and copies with
  provenance under `dev/share/`, for consumers of the development libraries.

## Running scripts

libuv's loop needs pipe creation, and spawning needs the launcher, so the
interpreter is a bundle whose manifest requests memory, clock (read and sleep),
launcher and pipe creation, with random optional for temporary files. Pyxis
stages it at `bin://lua5.1.pxb`. Bare `lua5.1` selects it without a catalog
on live media and installed revisions. From a local shell:

```text
lua5.1 script.lua [args]
bin://lua5.1.pxb -e 'print(_VERSION)'
```

Upstream `lua.c`'s script, `-e`, `-l`, `-v`, `--` and `-` (stdin) options are
kept, together with upstream `-i` and its basic stdio REPL. With no script or
expression, a terminal uses that REPL and redirected stdin is executed as a
script. Signals remain unavailable. `LUA_INIT` is honored. `require` searches
`package.preload` and then `?.lua;?/init.lua` relative to the working
directory, or `LUA_PATH` when set, including upstream `;;` default expansion. The working directory and environment are
libc's shared stores: `uv.chdir` changes where relative paths and `require`
look, and `os.getenv` sees variables set with `uv.os_setenv`. `lpeg` and `luv` are linked in and found
through `package.preload`.

## Standard library profile

What libc cannot do is left out or reported, never imitated:

- **Absent:** `os.execute` and `os.clock`, because libc has
  no Unix process shell or CPU clock. `os.setlocale` uses real fixed-C libc
  selection/query and returns nil for other locale names. `io.popen` retains the
  upstream explicit unsupported fallback. `package.loadlib` and C searchers
  retain the upstream unsupported-loader implementation; default `cpath` is
  empty and no dynamic loading is provided. `file:setvbuf` uses current libc.
- **Calendar tables:** `os.time(table)` uses libc's `mktime` in the zone `TZ`
  selects. A wall time skipped by a DST gap, or repeated in a fold without an
  `isdst` field that settles it, returns `nil`, as upstream does for any
  `mktime` failure. Otherwise an `isdst` that disagrees with the zone reads the
  wall time in the requested kind of time, as in standard C.
- **Adapted:** strings compare in byte order (libc's `strcoll` in the only, C,
  locale), the decimal point is always `.`, files open in binary mode from the start (no `freopen`), and
  `os.tmpname` reserves an empty exclusive `tmp://lua_XXXXXX` file with
  `mkstemp`, which needs the random grant.
- **Complete:** the math library, with `asin`, `acos`, `sinh`, `cosh`, `tanh` and
  `exp` from libc's musl import, and the `debug` library.

## luv

luv runs over the native libuv: timers, idle/check/prepare, async, pipes,
console streams, child processes and synchronous filesystem calls.

- **Children:** `uv.spawn` returns the handle and `nil` instead of a PID. The
  exit callback receives `code, signal, reason`, where `reason` is `"exited"`,
  `"faulted"`, `"terminated"` or `"start_failed"`; startup failure preserves the
  native packed startup step and call status in `code`. A fault or termination
  reports code -1; signal is always 0. `process:get_pid()`, `uv.os_getpid()`, `uv.os_getppid()` and
  `uv.getpid()` return `nil, err, "ENOSYS"`. Children inherit console and pipe
  descriptors. Spawning with an inherited FILE descriptor is not qualified through
  luv, although the [libuv adapter](../libuv/README.md) accepts native and exported
  FILE descriptors for the standard streams, each with a cursor at zero. Spawn's
  `cwd` and `env` options work; without them a child gets the current working
  directory and environment.
- **Working directory and environment:** `uv.cwd`, `uv.chdir`, `uv.os_getenv`,
  `uv.os_setenv`, `uv.os_unsetenv` and `uv.os_environ` use libc's shared
  stores through libuv.
- **Operating system:** `uv.os_uname`, `uv.os_homedir`, `uv.os_tmpdir` and
  `uv.os_gethostname` answer from native sources, listed in the
  [libuv notes](../libuv/README.md#operating-system-queries).
  `uv.available_parallelism()` returns the startup usable CPU count through libuv.
- **Unsupported:** TCP, UDP, DNS, file watches, signals, `kill`, thread and work
  queueing, callback-style filesystem calls, user and group IDs, interface
  addresses, memory/load/CPU/metrics introspection, handle printing, passwd and
  group records, process priorities,
  UTF-16 conversion and pending pipe handles. Each returns
  `nil, err, "ENOSYS"` as luv reports any libuv error. `uv.new_work` still
  creates luv's context; `uv.queue_work` reports ENOSYS.

## Adaptations

- `0001-pyxis-runtime.patch` (Lua) implements the profile above, removes the
  interpreter's unsupported signal handling, names it `lua5.1` and
  preloads `lpeg` and `luv`.
- `0002-pyxis-file-abandonment.patch` separates explicit close from GC
  abandonment and gives the standalone driver normal/error shutdown selection
  while protecting standard files and preserving foreign FILE close callbacks.
- `luv-0001-pyxis-native.patch` adds `src/pyxis.c`, which replaces `dns.c`,
  `tcp.c`, `udp.c`, `fs_event.c` and `fs_poll.c` with ENOSYS functions under
  the same Lua names, plus luv's connect callback that pipes also use. It guards
  socket constants, reports the native PID and exit results, routes
  `pipe_connect` through `uv_pipe_connect2`, and returns ENOSYS where the
  libuv profile omits a function.
- `compat53-0001-no-reopen.patch` drops compat-5.3's binary-mode `freopen`, as
  in Lua's `luaL_loadfile`.
- Two upstream warnings remain: misleading indentation in Lua's `ltablib.c`
  and a sign comparison in luv's `fs.c`.
- compat-5.3's Lua modules and LPeg's `re.lua` are not staged.

Lua, LPeg and lua-compat-5.3 are MIT; luv is Apache-2.0.

On Pyxis, `process:pyxis_resources()` copies declared resource roots from the
exact command selection libuv launched. Its table maps root names to owned
read-only directory userdata. `root:read(relative_path)` reads within that held
root, including native traversal boundary checks; `root:same(other)` compares
held domain/object identity, returning `nil` when unavailable. Garbage collection
releases copies independently of process exit or handle close. The private app
root and external authority are not exposed. Image-backed jobs return no roots.
