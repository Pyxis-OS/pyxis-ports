# Native WPA2 RSN library

Build the pinned wpa_supplicant 2.12 RSN library with a current SDK:

```sh
lua build.lua wpa-supplicant --sdk /path/to/pyxis/build/sdk
```

The recipe fetches only the owner's `raw-w1` mirror and checks the release
SHA-256. It stages `dev/lib/libwpa-rsn.a`, the headers under
`dev/include/wpa-rsn/`, and `dev/share/wpa-rsn.mk`. It adds no executable or
ordinary image dependency. Include that Make fragment to obtain
`WPA_RSN_CPPFLAGS` and `WPA_RSN_LIBRARIES`; native consumers include
`<wpa-rsn.h>` with those flags. Do not redefine the shared build profile.

The library contains upstream four-way/group-key state machines, IE/KDE parsing,
common helpers and the internal AES, SHA-1, SHA-256 and MD5 closure listed in
`sources.lua`. MD5, AES-CMAC and SHA-256 remain in shared upstream helpers;
compiling a helper does not negotiate its algorithm. The native caller must
select only `WPA_PROTO_RSN`, `WPA_KEY_MGMT_PSK`, CCMP-128 pairwise/group, and
PMF disabled. It must reject unsupported AP/RSN requirements before association,
including required PMF, and reject requests outside that policy.

No generic daemon, socket driver, EAP method/state machine, configuration-file
parser/writer, control socket, D-Bus, SAE, FT, P2P, mesh, MLO driver or roaming
policy is built. `IEEE8021X_EAPOL` is unset: upstream's disabled-feature paths
supply no preauthentication or PMKSA reuse. The shared headers still describe
upstream structures and constants for omitted features. The source patch skips
unavailable socket headers, selects the native build profile and makes the
unused NTP helper unavailable at compile time. The native profile maps upstream
`os_memset` clears and `forced_memzero` to libc's C23 `memset_explicit`; it needs
that current SDK function. Cryptographic algorithms, handshake policy and the
remaining upstream cleanup paths are unchanged.

## Native integration

The caller owns one `wpa_sm` and its heap-allocated `wpa_sm_ctx`, association
policy, event loop, authority and all native resources. `wpa_sm_init()` owns the
context after success, and `wpa_sm_deinit()` releases it. The required callback
contract is upstream `rsn_supp/wpa.h`: state transitions, deauthentication,
EAPOL allocation/send, AP IEs and key installation/removal must return real
outcomes. Supply zero-initialized context/config structures and the real selected
AP, SSID and local address. No successful placeholder operation is provided.

The archive's external native adapter requirements are:

- `os_get_random`: fail closed when delegated entropy is unavailable; the
  optional userspace entropy pool is omitted and nonce generation calls this
  adapter directly.
- `os_zalloc`, `os_memdup` and `os_memcmp_const`: checked native allocation and
  constant-time comparison, following `utils/os.h`.
- `eloop_register_timeout` and `eloop_cancel_timeout`: real bounded timers,
  cancellation counts and wildcard matching from `utils/eloop.h`, driven by the
  native monotonic clock and event receiver. No socket/select loop is linked.

The remaining external symbols are ordinary SDK libc operations. The upstream
`forced_memzero` helper and clear/free routines are included. The caller must
also erase credentials and native request/key buffers it owns. Do not let a
replayed handshake reinstall keys or reset packet numbers. Key commands and
controlled-port opening must be confirmed before reporting readiness.

All upstream debug messages, identifier dumps and key dumps are compiled out;
the native caller reports bounded status independently. The static build does
not qualify EAPOL handling, RF behavior, firmware key installation, or secrecy
of a consumer's input/storage paths. Those need the kernel/userland integration
and QEMU/native qualification in the owning Wi-Fi milestone.

The release hash is the reproducible source pin. A detached signature is
available upstream; this recipe does not establish signer trust. README,
COPYING, PORT-NOTICE, extracted SOURCE-NOTICES and source provenance are staged
beside the library and available for its eventual application's notices.
