# jq

[jq](https://jqlang.github.io/jq/) 1.7.1, a JSON processor, pinned to the release
archive `jq-1.7.1.tar.gz`, SHA-256
`478c9ca129fd2e3443fe27314b455e211e0d8c60bc8ff7df703873deeee580c2`, fetched from
the owner's `raw-github` mirror on `repo.internal`. The recipe stages
`bin/jq.pxe`, upstream's `COPYING` and `PORT-NOTICE` under
`share/licenses/jq/`, and the source and checksum in `share/jq/source.txt`.
`COPYING` carries jq's MIT license and the notices for David Gay's dtoa.c and
IBM's decNumber (ICU License), both compiled in. It is also a plain program: it
needs only the usual console, memory and directory grants, and reads JSON from
files, scheme paths or stdin.

```text
jq . host://data.json
cat json+http://10.0.2.2:8080/jobs | jq '.jobs[].status'
jq -r .name < home://package.json
jq --arg who world -n '"hello " + $who'
```

Exit statuses follow upstream: 0 success, 1 under `-e` for a false or null last
output, 2 for an unreadable file or usage error, 3 for a compile error and 5 for
a runtime or input parse error.

## Build

The release archive has no `config.h`, because jq's configure script only
emits `-D` options, and it cannot run against the SDK. `Makefile` lists the
sources of upstream's `Makefile.am` and the `HAVE_` options each libc function
earns. The generated `parser.c` and `lexer.c` ship in the archive, and
`builtin.jq` becomes `builtin.inc` with the same `sed` upstream uses.

- decNumber is on (jq's default), so number literals keep their precision.
- Oniguruma is not built. `test`, `match`, `capture`, `scan`, `split/2`, `sub`
  and the other regular-expression builtins exist but fail at run time with jq's
  message that it was compiled without Oniguruma.
- The program builds as `-std=gnu17`, as upstream does. Its function pointer
  tables are written `jv (*)()`, which C23 reads as `(void)`.
- `__thread` and the `pthread_key_create` path are both on, as on Linux.

## Patches

1. `0001-environment-snapshot.patch` (under `PYXIS`): `$ENV` and `env` build
   their object from `pyxis_environment_snapshot_init` instead of `environ`,
   which libc deliberately does not provide.
2. `0002-buffer-stdout.patch` (under `PYXIS`): `main` calls `setvbuf` on stdout,
   line buffered on a terminal and fully buffered otherwise.

## libc additions

jq needed these from libc, which the Pyxis userland repository now provides
rather than the recipe working around them: `<signal.h>` (`raise`, which
decNumber calls on a trap jq disables), `<sys/time.h>`, `<libgen.h>` `dirname`,
a C-locale `strptime`, `isnormal`, a non-NULL result from `malloc(0)` (which jq
reads as out of memory), and the ISO C double math functions
`acosh asinh atanh cbrt copysign fdim fma fmax fmin hypot lgamma nearbyint
nextafter nexttoward remainder scalbln tgamma`.

## Unavailable

Functions libc does not have, so their `HAVE_` options are off:

- Non-ISO math: `j0 j1 jn y0 y1 yn gamma lgamma_r drem significand scalb exp10
  pow10`. The jq builtins of those names are undefined.
- `timegm`, `memmem` and `gettimeofday`: jq's own fallbacks are used. `mktime`
  calls libc `mktime` and adds `tm_gmtoff`, so it follows `TZ`; `now` calls
  `time`, so it has whole-second precision.
- `strptime` has no `%Z`; a format using it fails to match.

## Limits

jq colors its output when stdout is a terminal, as on Unix; use `-M` to turn that
off. Patch 3 makes stdout line buffered on a terminal and fully buffered
otherwise, as on Unix: without it jq makes one native write per output token
through libc's unbuffered stdout, and `jq -n '[range(100000)]' -c > file` took
about 15 seconds on QEMU/KVM. A pipe or file therefore receives output at exit or
when the buffer fills, unless `--unbuffered` is given.
