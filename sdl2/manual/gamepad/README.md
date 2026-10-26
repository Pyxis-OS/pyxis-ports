# Manual SDL joystick and GameController viewer

An opt-in, asset-free software viewer using public SDL2 APIs. It opens up to
16 devices, displays one at a time, logs identity and automatic mappings, and
closes handles on removal and exit. It is outside ordinary images and recipe
staging. The bitmap lettering is original Pyxis source under MPL-2.0.

Use the SDK compiler environment from the ports README. Build each archive
profile separately; they produce separate bundles from the same viewer source:

```sh
make -f sdl2/manual/gamepad/Makefile SDK=/absolute/path/to/sdk \
  SDL2_STAGE=/absolute/path/to/sdl2/stage PROFILE=ordinary
make -f sdl2/manual/gamepad/Makefile SDK=/absolute/path/to/sdk \
  SDL2_STAGE=/absolute/path/to/sdl2/stage PROFILE=osmesa \
  MESA_STAGE=/absolute/path/to/mesa/stage/dev
```

`BUILD` defaults to `build/manual-sdl-gamepad`. Each `BUILD/ordinary` or
`BUILD/osmesa` directory contains `manifest.json` and `app/bin/sdl-gamepad.pxe`.
Publish the chosen directory through an explicitly selected manual bundle
upload or guest image. Preserve SDL's and, for OSMesa, Mesa's staged dependency
notices with a distributed bundle. Both profiles draw through the SDL window
surface; neither creates a GL context. OSMesa selection qualifies linkage and
the SDL input path in that archive, not GL rendering. The 1160 by 800 diagnostic
canvas scales in software to the actual window surface, including after resize.

Launch from a foreground local shell with display, keyboard and `gamepad/input`
authority. Tab selects the next open device; Escape exits. Attach logs include
name, instance ID, GUID, VID/PID/version, raw counts and full mapping. The left
panel shows six raw axes, 19 buttons and hat 0 using the native DualSense Edge
order. The right shows SDL's six standardized axes and 21 buttons, including
mapped d-pad, touchpad, misc and paddles. Teal buttons are pressed; dark boxes
and `N/A` axes have no binding. Raw trigger range is -32768 to 32767; mapped
trigger range is 0 to 32767. Raw labels describe the native mapping and do not
claim to identify arbitrary other controller layouts.

R requests brief low rumble, T trigger rumble, L a red LED and E an empty opaque
effect. Each key invokes the joystick API and, for a mapped device, the matching
GameController API, printing each return value and failure message. These are
operator requests: rumble/LED/effect must explicitly fail in the current Pyxis
backend. No output operation runs automatically.

The loop draws only after SDL events and sleeps in `SDL_WaitEvent` without a
refresh timer. After leaving the viewer idle, use only the pad and check that
the bars and event counters advance. Keep keyboard and pointer untouched during
that check. Manually check startup with/without a pad, hotplug, all raw and mapped
controls, unplug/replug instance identity, then focus away and return while
holding sticks, triggers, buttons and hat. Background joystick events are
disabled; focus events and state are visible and logged. These are observation
steps, with no automatic pass/fail claim. No-pad QEMU runs cannot qualify native
USB input or controller-only wake.
