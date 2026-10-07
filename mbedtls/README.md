# Mbed TLS

Mbed TLS 4.1.1 with its bundled TF-PSA-Crypto 1.1.1, built from the official
release archive. The archive contains dependencies and generated sources;
`GEN_FILES=OFF` avoids generation and Python is optional. Use CMake 3.20.2 or
newer, Make, curl, sha256sum, tar and bzip2 with the Pyxis SDK and toolchain.

```sh
lua build.lua mbedtls --sdk /path/to/pyxis/build/sdk
```

The development export contains `libmbedtls.a`, `libmbedx509.a`,
`libtfpsacrypto.a`, public headers and the two Pyxis configuration headers.
Include `share/mbedtls.mk` and use its `MBEDTLS_CPPFLAGS` for every translation
unit consuming the headers, and `MBEDTLS_LIBRARIES` when linking. All consumers
must use the same configuration as the archives. The SDK remains independent
of TLS. Target sources use upstream C99 with the SDK's freestanding headers and
machine flags; no host C library is linked.

The profile keeps `MBEDTLS_HAVE_ASM` and the SDK's `-O2` on x86-64, with GCC
or Clang. This matches upstream SECURITY.md guidance and the unaffected
x86/assembly configuration in the [compiler-induced constant-time advisory](https://mbed-tls.readthedocs.io/en/latest/security-advisories/mbedtls-security-advisory-2026-03-compiler-induced-constant-time-violations/).
The advisory also lists Clang at its default optimization levels as not known
to be affected. No advanced compiler optimization passes, such as LLVM's
select-optimize, are enabled.

CMake receives the SDK's compile flags without `-MMD -MP`. CMake writes its own
dependency files, and Clang reports the duplicate `-MD` as unused, which the
upstream warnings-as-errors build rejects.

The profile retains TLS 1.2/1.3 clients, RSA/ECDSA verification, upstream default
certificate security policy, ALPN and SNI. It disables servers, DTLS, PSK,
early data, tickets, renegotiation, session serialization, sockets, platform
timing, filesystem I/O, self-tests and persistent PSA storage. Record content
buffers are fixed at 16 KiB in each direction; verified chains retain the
upstream eight-intermediate maximum. Client certificate configuration and
session restoration are omitted by the native adapter. The build retains
upstream software crypto algorithms and certificate-authenticated suites/groups.

Native userland installs process-local allocation and UTC hooks with
`mbedtls_platform_set_calloc_free` and `mbedtls_platform_set_time` before PSA
initialization, and implements `mbedtls_ms_time` and
`mbedtls_platform_get_entropy`. Ports supplies no successful fallback hooks.
Authority, deadlines, trust provisioning, memory accounting and TCP callbacks
belong to the native adapter; this recipe alone cannot perform a handshake.

The guest install includes both complete upstream licenses and package
provenance under `share/licenses`. Static archives and headers are development
inputs, excluded from guest installation. Upstream C sources are unchanged;
there are no patches.
