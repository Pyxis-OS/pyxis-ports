# LibUltraShip native slice

Pinned `da6daf1a50a5e46d6641408d52227e31e85e46df`, matching SoH 9.3.1.
The reusable static library selects F3DEX_GBI_2, 32-bit controller buttons,
priority/pause-enabled real workers, read-only ZIP/O2R and desktop GL3.3
compatibility through SDL2/OSMesa. MPQ, DLL/TinyCC, scripting, file pickers,
gamepad discovery, networking and crash helpers are excluded. No game code,
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
headers carry the required stb include directory. Fonts added after renderer
initialization rebuild the GL atlas before the next ImGui frame. Texture dispatch keeps explicit resolved-segment/resource ownership for low
heap addresses; its remaining image-membership fallback uses the SDK's
`__pyxis_image_start/end` symbols, never a fake loader.

## Ownership and paths

Explicit regular `.o2r`/`.zip` filenames are read-only. Folder archives, archive
writes, legacy text-CVar conversion and arbitrary host path discovery are not
in this profile. Native stdio/stat/mkdir implement paths; libc++ filesystem and
fstream are not enabled. Defaults are `app://` resources and
`home://<shortName>/` state/logs, with ordinary home authority, not isolation.
JSON configuration publication checks write/close/rename errors; it is not a
power-loss durability boundary. Malformed JSON is reported and preserved.

The application owner stops and joins its audio producer before changing the
backend/channels or destroying Context. Fixed stereo is the qualified profile.
A requested SDL backend failure is visible; it never silently selects Null.
Destroy Context from its owner, not a resource worker, with no concurrent new
API calls. Shutdown rejects new resource submissions, drains and joins its
resource pool while archives/renderers are alive, then releases GUI/GL/SDL/audio
and the owned logger worker. Release external GUI/resource owners before Context.
No global logger pool is installed. Partial initialization unwinds the same way.

## Public first check

`stage/bin/lus-demo.pxb` decodes public JSON from a generated deflate O2R, decodes an authored one-pixel texture, verifies its retained dispatch owner, does
an uncached bulk load with one worker, refuses missing resources and read-only
writes, and checks a real FBO/depth readback. Its UI runs the pinned Fast3D
interpreter and ImGui together at the real output size, 1:1, MSAA off/interval0.
Keyboard/pointer/resize use SDL; Escape exits. A real SDL queued-audio producer
submits silence (a lifecycle check, not an audibility check), then joins before
Context teardown. No launcher or network authority is requested. This optional
package is outside the boot archive and does not qualify the private game.

MIT/Fast3D and embedded Font Awesome 4.7.0 OFL notices, selected dependency
notices, pinned public source and covered native adaptation source accompany
the development files and demo package.
