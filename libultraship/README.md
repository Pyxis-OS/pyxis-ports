# LibUltraShip native slice

Pinned `da6daf1a50a5e46d6641408d52227e31e85e46df`, matching SoH 9.3.1.
The reusable static library selects F3DEX_GBI_2, 32-bit controller buttons,
priority/pause-enabled real workers, read-only ZIP/O2R and desktop GL3.3
compatibility through SDL2/OSMesa. MPQ, DLL/TinyCC, scripting, file pickers,
networking and crash helpers are excluded. Native SDL GameController input uses
the delegated Pyxis gamepad session. No game code,
ROM or extracted game data belongs in this recipe or its fixture.

From the parent checkout, `make -f scripts/ports.mk libultraship` builds the
library after its SDK and dependency outputs exist. The standalone runner uses:

```sh
lua ports/build.lua libultraship --sdk /path/to/sdk --work /new/work \
  --dependencies '/prefix/imgui;/prefix/imgui-backends;/prefix/sdl2;/prefix/sdl2-gl;/prefix/mesa;...'
```

Supply all selected dependency development prefixes; the parent make rule is
the complete list. No dependencies are downloaded by CMake. `stage/dev` carries
`libultraship.a`, matching headers/profile, `libultraship.pc` and
`LibUltraShip::LibUltraShip`. Consumers must use the supplied target/module:
its forced profile header is part of the compiled C++ layout. Public GUI texture
headers carry the required stb include directory. Upstream ImGui creates device/font objects lazily on the next frame. Texture dispatch keeps explicit resolved-segment/resource ownership for low
heap addresses; its remaining image-membership fallback uses the SDK's
`__pyxis_image_start/end` symbols, never a fake loader.

## Ownership and paths

Explicit regular `.o2r`/`.zip` filenames use native paths. Folder archives,
legacy text-CVar conversion and hosted path discovery are excluded because this
build has no libc++ filesystem/fstream. `app://` bundles open ZIP read-only;
other archive paths retain upstream open/write behavior and ordinary grants.
Defaults are `app://` resources and `home://<shortName>/` state/logs, with
ordinary home authority, not isolation. Log filenames retain the full application
name.

Native JSON configuration publication requires file sync, checked close,
replacement and parent-directory sync. Failure after replacement starts may
already have published; no rollback/replay is claimed. `Config::Save` and
`ConsoleVariable::Save` return the native result. Owner-thread
`SaveWasUncertain`/`GetLastSaveError` describe the latest attempt, reset by the
next save; failure is logged once and does not impose a sticky process exit
status. Malformed JSON retains upstream parse-error behavior.

Audio fallback, queue handling, shared logging and Context lifecycle retain
upstream behavior. Applications that continue teardown must stop their own audio
producer before destroying its backend. Native relative capture, physical output
size and unsupported window/vsync operations use the SDL backend. Resource
workers use a minimal direct-load path for nested synchronous work, avoiding a
one-worker pool waiting on itself.

## Controller input

Applications declare `gamepad/input` alongside keyboard authority. Controller
initialization follows upstream, including optional database loading
and its existing diagnostics. Hotplug refreshes the manager and ImGui's controller
list; existing port-ignore, binding and handle-management behavior is retained.

Fresh port-1 configurations receive upstream keyboard, mouse and SDL defaults.
Existing configured ports reload their mappings without automatic migration or
erasure. Application settings UIs may add SDL defaults explicitly while keeping
keyboard mappings. Gamepad menu navigation follows the existing setting and
menu/popup state; keyboard navigation retains the upstream default. ImGui selects the
normal AutoAll controller mode for this LUS application, without changing the
shared ImGui default. LED/rumble/sensor output remains unsupported by the Pyxis
SDL backend; no game-specific output implementation is added.

## Public first check

`stage/bin/lus-demo.pxb` decodes public JSON from a generated deflate O2R, decodes an authored one-pixel texture, verifies its retained dispatch owner, does
an uncached bulk load with one worker, refuses missing resources and read-only
writes, and checks a real FBO/depth readback. Its UI runs the pinned Fast3D
interpreter and ImGui together at the real output size, 1:1, MSAA off/interval0.
Keyboard/pointer/resize use SDL; Escape exits. Keyboard release/reset visits
every binding, including both stick axes; other controller factories retain
upstream behavior. A real SDL queued-audio producer
submits silence (a lifecycle check, not an audibility check), then joins before
Context teardown. No launcher or network authority is requested. This optional
package is outside the boot archive and does not qualify the private game.

MIT/Fast3D and embedded Font Awesome 4.7.0 OFL notices, selected dependency
notices, pinned public source and covered native adaptation source accompany
the development files and demo package.
