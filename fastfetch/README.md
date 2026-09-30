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
fastfetch --structure OS:Kernel:CPU:Memory:Uptime:TerminalSize:Colors
```

A standalone staged executable can also be exposed through `host://` and run
by its explicit URI.

## Adaptation

Fastfetch 2.69.0 is pinned to `0c3b852bf7bad2837a814c7a31bf332092048a2b`.
Three ordered patches separate the SDK build, native integration and owner-supplied
ASCII logo. The build selects the portable core and nine modules, links SDK startup
and static libraries with libgcc, then converts ELF to P1F. No compiler rebuild,
host-libc link, fake Unix services or optional graphics/thread/interpreter libraries
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
| Colors, Break, Separator | Upstream presentation helpers |

The six data modules run by default; presentation helpers remain selectable.
The built-in `Pyxis`/`Pyxis OS` logo is selected from native OS identity. These
queries are separate observations, not one atomic snapshot. CPU count is not
physical cores or process allowance, and allocator total is not installed RAM.

Missing optional fields render empty/unset through upstream formatting; JSON
uses null where appropriate. Uptime has no inferred boot epoch: `bootTime` is
null, and calendar format slots are unset. For example,
`{name}{?freq-max} @ {freq-max}{?}` omits unknown CPU frequency.

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

The reconstructed standalone fetch/apply/configure/build/stage recipe passed
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
