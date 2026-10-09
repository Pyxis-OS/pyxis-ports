# Manual SDL clipboard exercise

This opt-in program uses only public SDL APIs. It draws once and waits on
`SDL_WaitEvent` while idle, so the same executable can qualify unchanged and
updated adapters. It is not part of the SDL recipe stage or ordinary images.
Every clipboard operation requires an operator key; there is no automatic run.

```sh
make -f sdl2/manual/Makefile SDK=/path/to/pyxis/build/sdk \
  SDL2_STAGE=/path/to/sdl2/stage BUILD=/path/to/manual-build
```

Use the SDK's Pyxis compiler environment. Load `manual-build/sdl-clipboard.pxe`
through an explicitly chosen FILE/HOST upload or opt-in guest image. Launch it
from a foreground local shell, so the shell's separate clipboard grants and
graphics input/display sessions reach it. The tool does not obtain grants or
native activation on its own. Results appear on screen and on stdout, including
byte count, FNV-1a 64-bit hash, sanitized preview and SDL error.

- `1` ASCII including LF/Tab, `2` multibyte UTF-8 including CRLF, `3` empty,
  `4` exactly 64 KiB, `5` surrogate UTF-8, `6` 64 KiB plus one byte: choose the
  payload, then physically press local Ctrl+C or shared Super+Shift+C.
- Ctrl+V or shared Super+Shift+V calls Has then Get. `D` selects direct Get;
  `G` restores Has then Get. `R` toggles a second immediate call after Copy or
  Paste to inspect one-attempt consumption.
- `U` makes unarmed Set, Has and Get calls. `N` pushes synthetic Ctrl+V through
  SDL; it cannot arm an action. `F` chooses refusal mode: the next physical Copy
  calls Get and the next Paste calls Set. `G`/`D` restore ordinary mode.
- `T` cycles zero, two and six seconds between delivered command and clipboard
  call. Use two seconds for focus/overlay or resize revocation before activation
  expiry, and six seconds to inspect expiry itself. The wait is only after a
  physical Copy/Paste command.
- `Q` pauses for two seconds, allowing the operator to queue overlapping
  local/shared gestures. `B` toggles batch GET: queued events are removed in a
  multi-event call and clipboard commands must refuse. `L` flushes the key-down
  queue before the next Copy/Paste API call, demonstrating cancellation of the
  delivered command, then returns to ordinary mode.
- Escape exits, releasing graphics/input ownership. Copied content can then be
  checked by another foreground consumer. Use a second source to replace a
  store and verify that future reads observe the replacement.

Keep baseline and updated runs matched for CPU count, devices, accelerator,
geometry, grants and the executable source. Actual comparisons, manual results
and limitations belong in the Pyxis qualification record. Invalid text, limit,
focus/owner loss, absent shared grants and non-ASCII terminal refusal require
manual observation; the tool asserts no pass/fail result.
