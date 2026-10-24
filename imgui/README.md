# Dear ImGui

Pinned v1.91.9b-docking, MIT, with the native configuration in `imconfig.h`:
`ImTextureID` is `void*`, math operators are enabled, and the default shell
helper is disabled. Consumers must use these matching headers and archives.

```sh
lua ports/build.lua imgui --sdk /path/to/sdk --work /path/to/new/work \
  --sdl2 /path/to/sdl2/stage/dev \
  --sdl2-osmesa /path/to/sdl2/stage/gl-dev --mesa /path/to/mesa/stage/dev
```

`stage/dev` is the independent core (`libimgui.a`, headers, `imgui.pc`,
`ImGui::ImGui`). `stage/sdl2-gl-dev` adds the matching built-in SDL2/OpenGL3
backends (`imgui-sdl2-opengl3.pc`, `ImGui::SDL2OpenGL3`). Select that target or
pkg-config module instead of ordinary SDL2; it links `libSDL2-osmesa.a` and
static Mesa. The official custom GL loader option uses directly linked GL
symbols, without `dlopen` or an extra loader library. No third-party extensions
are bundled.

The native SDL profile owns one window. Multi-viewports are refused at init;
gamepad auto-discovery, global mouse capture and unsupported system cursor
shapes are not advertised. Ordinary keyboard text, pointer events and native
window resize remain enabled. Deferred ImGui clipboard actions use ImGui's
context-local buffer, not Pyxis's gesture-bound system clipboard. URL opening
and native IME are outside this profile. Consumers opt into only the runtime
grants they need; the development packages convey no authority.

`stage/bin/imgui-demo.pxb` is an asset-free demonstration with explicit display,
keyboard, pointer, memory, thread, pipe and precise-clock grants. Escape exits;
Tab/arrows navigate, the input field accepts text, and the canvas follows
the real output size at 1:1, including output resize. It requests GL 3.3 compatibility, no MSAA and interval
zero. It is an optional program package, never a boot-archive dependency.

Core sources, the embedded font and stb notices, backend sources, provenance
and port notices accompany the selected development files. The demo retains
the static SDL/Mesa/C++ closure's notices; its package also carries covered
Pyxis port source. This does not qualify a game or native hardware.
