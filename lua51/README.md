# Lua 5.1 with LPeg and luv

PUC Lua 5.1.5 with its standard libraries, LPeg 1.1.0, luv 1.52.1-0 and
lua-compat-5.3 0.13, built as one recipe over the [native libuv](../libuv/README.md).
These are the pins Neovim 0.12.5 uses. The Lua 5.5 port in `lua/` is separate
and unchanged; the two coexist.

```sh
lua build.lua lua51 --sdk /path/to/build/sdk --libuv /path/to/libuv/stage/dev
```

The recipe downloads only the owner mirrors and verifies each SHA-256 before
applying patches. It stages:

- `share/lua51/lua5.1.pxb`, the interpreter as a development bundle;
- `dev/lib/liblua5.1.a`, `liblpeg.a` and `libluv.a` (with compat-5.3's C API),
  headers under `dev/include/lua5.1/` and `dev/include/luv/`, for Neovim;
- the four licenses under `share/licenses/lua51/` and `share/lua51/source.txt`.

## Running scripts

libuv's loop needs pipe creation, and spawning needs the launcher, so the
interpreter is a bundle whose manifest requests memory, clock (read and sleep),
launcher and pipe creation, with random optional for temporary files. Pyxis
stages it at `boot://share/lua51/lua5.1.pxb`; it is not in the default command
catalog. From a local shell:

```text
boot://share/lua51/lua5.1.pxb script.lua [args]
boot://share/lua51/lua5.1.pxb -e 'print(_VERSION)'
```

Upstream `lua.c`'s script, `-e`, `-l`, `-v`, `--` and `-` (stdin) options are
kept. There is no interactive mode: without a script or `-e`, it prints usage
and exits with status 1. `LUA_INIT` is honored. `require` searches
`package.preload` and then `?.lua;?/init.lua` relative to the working
directory, or `LUA_PATH` when set. The working directory and environment are
libc's shared stores: `uv.chdir` changes where relative paths and `require`
look, and `os.getenv` sees variables set with `uv.os_setenv`. `lpeg` and `luv` are linked in and found
through `package.preload`.

## Standard library profile

What libc cannot do is left out or reported, never imitated:

- **Absent:** `io.popen`, `os.execute`, `os.clock`, `os.setlocale`,
  `file:setvbuf`, `package.loadlib` and the C module searchers, because libc has
  no `popen`, `system`, `clock`, locales, `setvbuf` or dynamic loading.
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
  `"faulted"` or `"terminated"`; a fault or termination reports code -1 and
  signal 0. `process:get_pid()`, `uv.os_getpid()`, `uv.os_getppid()` and
  `uv.getpid()` return `nil, err, "ENOSYS"`. Children inherit console and pipe
  descriptors, not files (libuv rejects FILE cursor inheritance). Spawn's
  `cwd` and `env` options work; without them a child gets the current working
  directory and environment.
- **Working directory and environment:** `uv.cwd`, `uv.chdir`, `uv.os_getenv`,
  `uv.os_setenv`, `uv.os_unsetenv` and `uv.os_environ` use libc's shared
  stores through libuv.
- **Unsupported:** TCP, UDP, DNS, file watches, signals, `kill`, thread and work
  queueing, callback-style filesystem calls, user and group IDs, interface
  addresses, memory/load/CPU/metrics introspection, handle printing, passwd,
  UTF-16 conversion and pending pipe handles. Each returns
  `nil, err, "ENOSYS"` as luv reports any libuv error. `uv.new_work` still
  creates luv's context; `uv.queue_work` reports ENOSYS.

## Adaptations

- `0001-pyxis-runtime.patch` (Lua) implements the profile above, removes the
  interpreter's signal handling and interactive mode, names it `lua5.1` and
  preloads `lpeg` and `luv`.
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
