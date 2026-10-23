# Caddy

Optional Caddy v2.11.7, built with the pinned native Pyxis Go toolchain and stock
`cmd/caddy` module imports. It is not included in ordinary images.

```sh
lua build.lua caddy --sdk /absolute/path/to/pyxis/build/sdk \
  --go /absolute/path/to/pyxis-go-prefix --work /absolute/fresh/caddy-work
```

`--go` selects the installed prefix produced by Pyxis's `toolchain/build-go.sh`.
This build never rebuilds the compiler or links libc. The recipe downloads the
SHA-256-pinned Caddy module ZIP from the internal Go proxy, then uses upstream's
`go.sum` to authenticate dependencies while creating a private vendor tree.
Ordered patches apply there; the final target build uses `GOPROXY=off` and
`-mod=vendor`. No shared module cache is modified or used for target compilation.

The bundle stages at `stage/bin/caddy.pxb`. Its metadata contains the Caddy and
Go licenses, compiler revision, original checksum/module list, complete notices
for compiled dependency modules and the applied patches. `stage/share/caddy`
records source, build profile and provenance. `-s -w` omits P1F debugging metadata;
Go retains its own pclntab for stack traces.

The first supported configuration is a foreground server with explicit concrete
IPv4/nonzero listeners, HTTP/1.1 or HTTP/2, supplied content/config roots and
optional supplied certificate/key files. Admin, automatic HTTPS/ACME and HTTP/3
are disabled in the qualification configuration. Caddy's default configuration
must not be assumed to meet that profile. The bundle requires declared TCP
CONNECT/LISTEN, native memory/threads/precise clock, RANDOM and service/create;
LISTEN must be available through the opted-in space. Optional `dns_udp` uses the
configured DNS_SERVER; numeric upstreams do not require DNS. It receives no
network-owner, launcher or disk-write authority from this recipe.

Build tags `nobbolt,nobadger,nomysql,nopgx,nosshagentkms` omit unsupported
Smallstep database and SSH-agent backends. The latter also needs its pure signer
wrapper separated from the disabled backend so upstream's type assertions
compile. Generic QUIC sources remain linked: Caddy has no HTTP/3 compile-out tag.
Logrus uses native console detection; readline uses plain/nonterminal input and
explicit unsupported raw/password/suspend operations. Tailscale state and the
certificate-transparency fork's platform root lookup are unsupported; explicit
roots still use that fork's pure-Go verifier. Normal Go HTTPS uses the held boot
CA bundle. No Unix signal numbers or successful fake operations are added.

Registered modules outside the qualified profile are not a support claim. Local
ACME service, persistent certificate/account storage, automatic trust
installation, Tailscale, background start/admin reload, subprocesses, the
readline interactive editor and arbitrary third-party modules remain outside
this delivery. First Ctrl+C follows Caddy's existing os.Interrupt graceful-stop
path; Pyxis's second press and forceful native termination remain forceful.
Runtime qualification is recorded in the parent repository.
