# Fastfetch for Pyxis

The Pyxis image installs `app://fastfetch.pxe` and its notices under
`app://share/licenses/fastfetch`. The shell resolves the command `fastfetch`.

For a standalone build, use an exported SDK with the Fastfetch libc prerequisites:

```sh
lua build.lua fastfetch --sdk /absolute/path/to/build/sdk --work /tmp/fastfetch-build
```

The work directory must not exist. The recipe uses the prebuilt Pyxis compiler,
CMake 3.21 or newer and GNU Make. It stages `bin/fastfetch.pxe` and licenses.
The normal image build selects this recipe and `install.lua` packages its stage.
Run in the guest:

```text
fastfetch
fastfetch --json
fastfetch --config home://fastfetch.jsonc
fastfetch --structure OS:Kernel:CPU:Memory:Uptime:TerminalSize:Disk:Colors
```

A standalone staged executable can also be exposed through `host://` and run
by its explicit URI.

## Adaptation

Fastfetch 2.69.0 is pinned to `0c3b852bf7bad2837a814c7a31bf332092048a2b`.
Four ordered patches cover the SDK build, native integration, owner-supplied
ASCII logo and native Disk consumer. The build selects the portable core and ten
modules, links SDK startup and static libraries with libgcc, then converts ELF to
P1F. No compiler rebuild, host-libc link, fake Unix services or optional graphics/thread/interpreter libraries
are involved.

| Changed area | Required adaptation |
| --- | --- |
| CMake | Pyxis platform/source selection, SDK link and P1F conversion |
| Platform/detection backends | Native identity, CPU, allocator, clock and console observations |
| I/O/time declarations and backends | Native file/path access and monotonic timing without Unix-only headers |
| Initialization and main | Omit unavailable locale, signals, buffering controls and atexit; retain normal lifecycle; explicit URI config loading and one-shot execution |
| Selected module frontends | Native query errors, unavailable fields, allocator label and Pyxis version name |
| General/logo options | Disable unavailable thread/process defaults, omit Unix path expansion and command/image helpers |
| String headers | Direct Pyxis-only includes of existing libc declarations |
| Logo registry | Owner's compass rose, unchanged bytes |

The formatter, string/list containers, module dispatcher, common diagnostic code
and bundled yyjson are unchanged from upstream. Upstream assertions and allocation
behavior are retained; there are no allocation wrappers, strict format validator
or aggregate module-failure policy. Upstream-derived changes remain MIT; original
native/build/logo material is MPL-2.0. See `PORT-NOTICE` and the staged licenses.

## Native observations

| Module | Meaning |
| --- | --- |
| OS | OS name and architecture from `system_info` READ |
| Kernel | Kernel name, running source commit and architecture; native ABI page size |
| CPU | Guest-visible BSP brand and online logical CPU count |
| Memory | **Memory (allocator)**: allocator total and allocated bytes |
| Uptime | Monotonic duration since HPET initialization, using clock READ |
| TerminalSize | Columns/rows from the named `output` console grant |
| Disk | Explicit startup root bindings with native directory FILESYSTEM_INFO authority |
| Colors, Break, Separator | Upstream presentation helpers |

The seven data modules run by default; presentation helpers remain selectable.
The built-in `Pyxis`/`Pyxis OS` logo is selected from native OS identity. These
queries are separate observations, not one atomic snapshot. CPU count is not
physical cores or process allowance, and allocator total is not installed RAM.

Missing optional fields render empty/unset through upstream formatting; JSON
uses null where appropriate. Uptime has no inferred boot epoch: `bootTime` is
null, and calendar format slots are unset. For example,
`{name}{?freq-max} @ {freq-max}{?}` omits unknown CPU frequency.

Disk enumerates only the caller's selected startup roots. Each native directory
with `FILESYSTEM_INFO` authority supplies its retained npfs volume observation;
roots without that authority and exported/provider roots are skipped. Archive,
RAM and HOST roots do not acquire an invented disk result. An attempted grant or
filesystem query failure reports a module error and releases partial rows.
No roots or no observable roots produces the upstream empty Disk JSON result;
text uses the upstream `No disks found` diagnostic when errors are enabled.

Default Disk text explicitly labels shared pool capacity, for example:

```text
Disk (data://): npfs [Read-only] - shared pool capacity: 64 MiB
```

The binding is the caller's root name plus `://`; `name` is the core volume name.
Repeated bindings and volumes remain separate rows. Equal pool IDs identify
shared storage: capacity must not be summed across these rows. Capacity excludes
the two superblocks and includes shared metadata/reserves; it is neither a fixed
volume size nor writable allowance. Ordinary opening verifies geometry and root
envelopes without reconciling global usage.

Disk JSON keeps `bytes.available`, `bytes.free`, `bytes.total`, `bytes.used`,
`files.total`, `files.used`, `createTime` and `mountFrom` null. All corresponding
size, percentage, file count, age/time and device placeholders remain unset in
custom formats, preserving their original numeric positions. External/hidden
classification and Unix key hyperlinks are also unset. Percentage/bar options
cannot create a usage display from unavailable values.

Appended native formats are `{pool-id}`, `{volume-id}`, `{generation}`,
`{pool-capacity}`, `{pool-capacity-bytes}`, `{is-gpt-degraded}` and
`{is-filesystem-degraded}`. The first capacity field uses upstream size formatting;
the second contains bytes. Disk JSON adds a `native` object with `poolId`,
`volumeId`, `selectedGeneration`, `poolAllocatableBytes`, `gptDegraded` and
`filesystemDegraded`. IDs are 32 lowercase hexadecimal digits in ABI byte order;
no UUID byte rearrangement is implied. Flags describe retained opening health.

Actual stdout's startup binding controls automatic plain-output mode, separately
from named console authority. Redirected defaults use plain text; upstream ASCII logo layout may remain
(`--logo none` suppresses it). TerminalSize can still query its named console. JSON skips terminal setup and
logo output. No keyboard input or terminal escape-response query is used.

## Configuration and limits

No config is discovered automatically. `--config` accepts one explicit native
URI ending in `.json` or `.jsonc`, or `none`. JSONC permits comments and trailing
commas. Reads use ordinary libc/yyjson file APIs with existing caller authority.
Upstream module order, keys, formats, colors, spacing and CLI overrides remain.
For example:

```json
{
  "logo": "Pyxis",
  "modules": [
    "os", "kernel",
    { "type": "cpu", "format": "{name} ({cores-online} online)" },
    "memory", "uptime", "terminalsize", "colors"
  ]
}
```

Disk folder and hide-folder filters are unsupported and produce a module error
when requested; the Pyxis hide-folder default is empty. `hideFS` accepts the
filesystem label `npfs`. Normal text type visibility options and module/key/size
formatting remain upstream; JSON retains upstream type-filter behavior. No Unix
folder glob runtime is added.

Automatic discovery, config/cache generation, dynamic refresh, image logos,
Lua/JS formats, threads, executable helpers and other modules are outside this
port. Upstream help still lists broader options. Unsupported module/format/logo
requests follow upstream diagnostics or fallback behavior; the port does not
prevalidate every option or redefine the process exit status for module errors.
In particular, a module error does not guarantee a nonzero exit status. Explicit
native clock failure during requested timing reports an error and exits.

Normal teardown releases Fastfetch state; process exit reclaims remaining
resources. Allocation failure handling and diagnostic visibility retain upstream
semantics, including release-build assertions disabled by `NDEBUG`. This port
makes no guarantee of graceful OOM recovery or terminal restoration after a fault.

## Validation

The Disk recipe passed a fresh standalone fetch/apply/configure/build/stage on
2026-09-30 with GCC 16.2.0, CMake 4.4.3 and SDK userland `3b9ba3f` from Pyxis
`98e6557`. All four ordered patches applied to the unchanged upstream pin. The
build linked the existing SDK libraries and staged the executable/notices. Two
existing unused-variable/parameter warnings remain in upstream JSON config code.
This establishes recipe/build behavior; Disk runtime and image integration are
validated by the parent Pyxis task.

The earlier reconstructed standalone fetch/apply/configure/build/stage recipe passed
with GCC 16.2.0, CMake 3.31.8 and SDK userland `c9ed311`. An ordinary Pyxis image
build passed. Four-CPU nested KVM with the documented QEMU AHCI fix, 256 MiB,
Fedora OVMF, virtio-net and private virtio-fs ran the staged executable through
the remote terminal. Native text/logo, JSON, explicit JSONC, optional-field
conditions and empty calendar fields were exercised. Redirected JSON was parsed
on the host and contained no escape bytes; it reported four online CPUs and the
remote console's 100x30 dimensions.

The logo SHA-256 matches the owner's source:
`657381d8eda6cba5aa5e872a24e283d8dae144e406f160d32363cb0b8d885e1d`.
Missing-grant paths were reviewed in code; no allocation fault injection or new
tests were added. Earlier validation of the superseded patch does not establish
behavior of this reconstruction.

Default-image integration subsequently passed an ordinary image build and
interactive local 160x48 and remote 100x30/40x12 runs of the packaged command.
JSONC, file redirection, a cat pipeline and repeated runs worked. A disposable
launcher omitting system_info, clock and named output grants received six JSON
module errors and exit status zero, as upstream permits. GDB matched CPU data
and allocator bytes to native kernel observations and uptime to clock
nanoseconds divided by one million. The narrow terminal retains upstream
layout: long logo/data lines may wrap; use `--logo none` and shorter formats.
No port patches changed for this integration.

Native Disk integration passed an ordinary parent image build and interactive
four-CPU nested-KVM runs with a read-only virtio-blk image. Local 160x48 and
remote 100x30 output showed three authorized bindings across two volumes sharing
one pool; a fourth `--no-info` binding stayed absent while ordinary reads worked.
Text labeled 67,100,672 bytes as 63.99 MiB shared pool capacity; JSON and GDB
agreed with host metadata on generation and capacity. Named/numeric formats,
conditions, custom keys, redirection, a pipeline and repeated JSON worked. Unknown
usage/time values stayed null or unset and captured output had no escape bytes.
Folder filters produced the documented module error; filesystem/type filtering
retained the documented behavior. Separate boots with only a non-observable root
and with no disk produced empty Disk JSON and the upstream text diagnostic. The
image hash was unchanged after detachment. Degraded flags and query-failure cleanup
were source-reviewed; no fault injection or new tests were added. Full evidence
is in the parent `docs/userland/fastfetch.md` Native Disk validation section.
