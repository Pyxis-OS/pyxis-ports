# Fastfetch for Pyxis

The Pyxis image installs `bin://fastfetch.pxe` and its notices under
`boot://share/licenses/fastfetch`. The shell resolves the command `fastfetch`.

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
Three ordered patches cover the SDK build, native adapters (including Disk and
Battery) and the owner's unchanged ASCII logo. The build selects eleven modules
and links SDK startup/static libraries with compiler-rt builtins. LLD writes the
P1F executable directly; there is no host-libc link.

Native adapters provide identity, CPU, allocator, clock, console, disk and battery
observations with existing grants. Unix locale/signals, process helpers, optional
graphics/interpreter libraries and Fastfetch's threaded detection closure remain
omitted. Pyxis itself supports threads; that omission describes this port's
selected dependency closure. Standard libc buffering and atexit are used.

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
| Disk | Exported environment directory entries with native directory FILESYSTEM_INFO authority |
| Battery | `system_info` READ power and battery queries |
| Colors, Break, Separator | Upstream presentation helpers |

The default module order follows upstream. Modules outside the selected closure
use upstream unsupported-module diagnostics; use `--structure` or configuration
to select the native modules listed above.
Battery prints nothing without a battery, as upstream does with errors hidden.
The built-in `Pyxis`/`Pyxis OS` logo is selected from native OS identity. These
queries are separate observations, not one atomic snapshot. CPU count is not
physical cores or process allowance, and allocator total is not installed RAM.

Missing optional fields render empty/unset through upstream formatting; JSON
uses null where appropriate. Uptime has no inferred boot epoch: `bootTime` is
null, and calendar format slots are unset. For example,
`{name}{?freq-max} @ {freq-max}{?}` omits unknown CPU frequency.

Disk enumerates only the caller's exported directory entries. Each native directory
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

Battery reads `system_info_get_power` and `system_info_get_battery`, the
kernel's latest ACPI poll. Each inserted battery with a known percentage is a
row: `capacity` is the kernel's percentage, and manufacturer, model,
technology and serial are the firmware's OEM, model, type and serial strings.
`[AC Connected]` follows the AC adapter. While discharging, time remaining is
remaining capacity times 3600 over the present rate, as Linux computes it.
Temperature and manufacture date are unavailable. Power Adapter is not built:
ACPI reports no adapter wattage, which that module prints.

Actual stdout's live descriptor binding controls automatic plain-output mode, separately
from named console authority. Redirected defaults use plain text; upstream ASCII logo layout may remain
(`--logo none` suppresses it). TerminalSize can still query its named console.
JSON follows upstream terminal setup and skips logo output. No keyboard input or terminal escape-response query is used.

## Configuration and limits

Configuration follows upstream parsing and precedence. Native relative paths and
scheme paths reach libc directly; `.json`, `.jsonc`, `.json5`, extension inference,
`none` and stdin `-` retain upstream behavior. Existing `home://.config/` and
`home://` directories supply config paths; existing `home://.local/share/` and
`home://` supply data/preset paths. Missing directories or grants are unavailable.
Discovery uses those paths without Unix filesystem assumptions. `--list-config-paths`
and `--list-data-paths` show them. Native recursive preset/logo listing is omitted.

Noninteractive `--gen-config` writes an explicit native/relative file or stdout
`-`, retaining upstream generation and diagnostics. Its writer creates parent
directories through native mkdir and abandons a failed provider transaction;
fclose supplies the real publication result. The interactive termios/poll/signal
configuration UI remains omitted. Dynamic refresh uses native nanosleep and
upstream cache invalidation/output behavior.

Upstream module keys, formats, colors, spacing and CLI overrides remain.
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

Recursive preset/logo listing, interactive config generation, image logos,
Lua/JS formats, Fastfetch detection threads, executable helpers and other modules
are outside this port. Upstream help still lists broader options. Unsupported module/format/logo
requests follow upstream diagnostics or fallback behavior; the port does not
prevalidate every option or redefine the process exit status for module errors.
In particular, a module error does not guarantee a nonzero exit status. Explicit
native clock failure during requested timing reports an error and exits.

Normal teardown releases Fastfetch state; process exit reclaims remaining
resources. Allocation failure handling and diagnostic visibility retain upstream
semantics, including release-build assertions disabled by `NDEBUG`. This port
makes no guarantee of graceful OOM recovery or terminal restoration after a fault.

## Validation

The patch cleanup passed an ordinary fetch/apply/configure/build/stage with the
current LLVM 23.1.3 toolchain and SDK. Reconstructed source matched the ordinary
runner's patched source. The battery adapter retains its `system_info` handle
through power and per-battery queries, closing it on completion or query refusal.
Its earlier close-before-query bug is fixed locally. Runtime battery observation,
changed configuration behavior and dynamic refresh need the parent task's manual
QEMU/native qualification; the ordinary build does not prove those outcomes.

Earlier native/Disk runs described in the parent subsystem documentation concern
the previous profile. No native battery or changed-profile runtime pass is claimed
here. No new tests or fault injection were added.
