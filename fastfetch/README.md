# Fastfetch for Pyxis

Build the standalone recipe against an exported SDK containing the Fastfetch
libc prerequisites:

```sh
lua build.lua fastfetch --sdk /absolute/path/to/build/sdk --work /tmp/fastfetch-build
```

The work directory must not already exist. The recipe uses the prebuilt Pyxis
compiler, CMake 3.21 or newer and GNU Make; it does not rebuild the compiler or
modify the SDK. It stages `bin/fastfetch.pxe` and notices under
`share/licenses/fastfetch`. Default image installation is a separate integration
task; `install.lua` does not include this recipe yet. For development, expose
its staged executable through the existing `host://` mount and run:

```text
host://fastfetch.pxe
host://fastfetch.pxe --json
host://fastfetch.pxe --config home://fastfetch.jsonc
host://fastfetch.pxe --structure OS:Kernel:CPU:Memory:Uptime:TerminalSize
```

## Sources and adaptation

Fastfetch 2.69.0 is pinned to
`0c3b852bf7bad2837a814c7a31bf332092048a2b`. The ordered patch
`patches/0001-pyxis-native-port.patch` contains the bounded CMake branch, native
runtime and detectors, module/error adaptations, allocation checks and ASCII
logo. Existing Fastfetch files retain upstream formatting and MIT terms.
New Pyxis native adapters/build support and the project logo use MPL-2.0; see
`PORT-NOTICE`. Bundled yyjson is unchanged and retains its MIT notice. The recipe
stages both licenses and the full yyjson header containing its notice.

CMake compiles 43 translation units, uses only SDK and compiler headers, links
the SDK startup and static libraries with libgcc, then runs the SDK elf2pxe for
P1F output. There are no host-libc links, fake Unix services, platform feature
probes or optional graphics/thread/interpreter dependencies. The common core
retains upstream text/JSON formatting, builtin ASCII logos and the bundled
`memrchr` fallback. The Pyxis compass rose is built in under `Pyxis`/`Pyxis OS`
and selected by default; its source bytes match the owner's ASCII file.

## Information and authority

Only these nine modules are registered:

| Module | Native meaning |
| --- | --- |
| OS | OS name and architecture from delegated `system_info` READ |
| Kernel | Kernel name, architecture and running source commit; page size is the native ABI constant |
| CPU | Cached guest-visible BSP brand and online logical CPU count |
| Memory | **Memory (allocator)**: allocator total and allocated bytes, excluding permanent reservations |
| Uptime | Monotonic duration since HPET initialization, through clock READ |
| TerminalSize | Columns/rows from the named `output` console grant, without reading input |
| Colors, Break, Separator | Text presentation helpers |

These are separate native observations, not an atomic snapshot. CPU count is
not physical cores or process CPU allowance; allocator memory is not installed
RAM or Linux-style available memory. Extra upstream fields have JSON null values
when unavailable. Uptime's `bootTime` is null. Named, positional and evaluated
automatic format placeholders that request unavailable fields report an error;
explicit references are also rejected inside skipped conditional branches.
Duration fields and the upstream `formatted` uptime field remain usable.

Default terminal text prints six data modules, a break, color swatches and the
logo. Default JSON and pipe-mode text select the six data modules. Explicit
requests for presentation-only modules in JSON return module errors. Colors
requires terminal output; Break and Separator can still produce plain text.

Stdout's actual startup binding controls terminal rendering independently of
the named `output` grant. File/pipe output forces plain mode, omits logo/swatches,
suppresses cursor-only key alignment and prints requested timing as plain lines.
`--pipe` can also select this mode on a console. JSON never emits the logo or
terminal setup sequences. TerminalSize can still query its explicitly delegated
console when stdout is redirected. No terminal escape-response queries consume
keyboard input.

## Configuration and failure behavior

No config is loaded automatically. `--config` accepts one explicit native URI
ending in `.json` or `.jsonc`; JSONC enables comments and trailing commas.
`--config none` keeps built-in defaults. Reads use ordinary libc/yyjson file APIs
and the caller's existing authority. A config may select module order, keys,
formats, colors, spacing and ASCII logo data or explicit native-URI logo files.
CLI presentation overrides are applied after the config. As in pinned upstream,
per-module customization belongs in JSON rather than removed module CLI options.
For example:

```json
{
  "logo": "Pyxis",
  "modules": [
    "os", "kernel",
    { "type": "cpu", "format": "{name} ({cores-online} online)" },
    { "type": "memory", "key": "Memory (allocator)" },
    { "type": "uptime", "format": "{days}d {hours}h {minutes}m {seconds}s" },
    "terminalsize"
  ]
}
```

Automatic discovery, generated config/cache writes, dynamic refresh, image
logos and their cache/aspect/animation options, Lua/JS formats, threading,
executable/network helpers and other modules are unsupported. Requests produce
errors instead of successful no-ops. Explicit config/logo file reads retain the
normal namespace/provider behavior; this port adds no networking authority.
The compiled upstream help includes options outside this bounded port; its
opening notice and this reference identify the supported boundary.

Allocation failure and checked buffer/list growth overflow produce a stderr
diagnostic and terminate nonzero. Core allocation sites are checked explicitly;
link wrappers cover bundled JSON and other allocator calls. Fatal exit may leave
partial stdout; it restores any cursor/line-wrap modes the program enabled.
Normal teardown releases owned Fastfetch state, and native process teardown
reclaims remaining resources. There are no atexit/signal facilities or global
changes to libc allocation semantics.

Failed selected module queries and unsupported fields report errors and permit
other modules to run, with a nonzero aggregate exit status. Text diagnostics go
to stderr; JSON represents failed module queries as error objects. Malformed
config and unsupported global options terminate nonzero. Output errors also
fail; partially written output is possible. Optional fields represented by null
do not themselves fail a query. Requested `--stat` requires clock READ and fails
if it cannot obtain timing.

## Validation

The standalone fetch/patch/configure/build/stage workflow passed with GCC 16.2.0,
CMake 3.31.8 and the SDK from Pyxis `cfb7f0d` / userland `c9ed311`. Native static
link and P1F conversion passed, and the packaged ASCII file matched its source
SHA-256 `657381d8eda6cba5aa5e872a24e283d8dae144e406f160d32363cb0b8d885e1d`.

Interactive validation used four-CPU nested KVM, QEMU 10.2.2 with the documented
AHCI fix, CPU max, 256 MiB, Fedora OVMF, entropy, virtio-net and a private HOST
export. Local text showed the complete logo and 160x48 dimensions; remote text
reported 100x30. JSON reported online count 4 and null unavailable values.
Explicit JSONC module formats worked. Unsupported named/positional/calendar
fields failed while a later module still printed; an inactive conditional did
not hide an explicit unavailable field. A command-logo request failed even with
JSON output. Redirected JSON and text with timing/key-width/right-logo options
contained no escape bytes. GDB observed the native CPU adapter return its BSP
brand and online count 4 in userspace.

A disposable local launcher passed only memory and stdout/stderr authority:
the six data modules returned explicit JSON errors for omitted system_info,
clock and named output grants, and the child exited with status 1. Allocation exhaustion was
reviewed, not fault-injected. No tests, self-tests, CI changes or boot automation
were added. Default image installation, narrow-console/pipeline coverage and
broader repeated-run acceptance remain the following integration task.
