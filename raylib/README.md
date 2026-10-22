# raylib 6.2 for Pyxis

Static SDL2/rlsw drawing library: shapes, textures, default/TTF text and basic
filled 3D. The Pyxis platform uses a fixed logical canvas and aspect-fit point
scaling into the real display area. QEMU checks are recorded in the parent
raylib experiment; native ThinkPad/vela qualification is pending. Audio is
omitted: raudio calls do not link.

## Build and link

Use the ordinary compiler container and an exported SDK containing the float
math and `strtok` additions. Build SDL2 first, then raylib:

```sh
lua ports/build.lua sdl2 --sdk build/sdk --work build/ports/sdl2
lua ports/build.lua raylib --sdk build/sdk --work build/ports/raylib \
  --sdl2 build/ports/sdl2/stage/dev
```

The development prefix is `build/ports/raylib/stage/dev`; the parent image build
stages it as `build/ports-dev/raylib`. Include `raylib.h` and use SDK compile and
link flags. Link application objects, `libraylib.a`, `libSDL2.a`, then the SDK
runtime group. No host OpenGL, X11 or system library is used. CMake consumers use
`find_package(raylib CONFIG REQUIRED)` and target `raylib`, with both development
prefixes on the SDK toolchain’s `CMAKE_FIND_ROOT_PATH`. Public `rlgl.h` declarations are available;
standalone `RLGL_IMPLEMENTATION` is not an exported header-only product.

## Compiled profile

`pyxis_config.h` is the single explicit configuration. It selects portable scalar
rlsw, shapes, textures, text/default font, direct basic 3D, PNG/TTF, generated
images, and compression. Model-file loaders, image export, SIMD requirements,
GPU/shader features, audio and shared objects are omitted. Software render-texture
helpers need separate qualification; this is not full OpenGL support.

File helpers preserve native `name://` roots and use real libc paths. Missing
modification metadata is refused with `ENOTSUP`; `GetApplicationDirectory()` has
no native executable-directory authority and returns the upstream empty result.
Assets need an explicit delegated root such as `app://`. Lexical path helpers do
not prove resource existence. `IsPathDirectory` requires a successful native stat.

The cached release archive, hash, ordered patches and profile are recorded in
`share/raylib/source.txt`. See `PORT-NOTICE` and `THIRD-PARTY-LICENSES`; the reached
source closure includes zlib and MIT components, not only raylib's zlib licence.

## Window, input and pacing

`InitWindow(w,h)` chooses positive logical drawing dimensions; `GetScreenWidth`
and `GetRenderWidth` describe that canvas. The actual content area comes from
SDL, remains separate, and can resize without reallocating the renderer. Readback
is top-down BGRA; SDL handles target channel masks/pitch, nearest scaling and black
letterbox. Pointer coordinates use the inverse transform; the letterbox is outside
the canvas. Missing optional pointer authority still permits drawing/keyboard use.
Native focus resets held input. Cursor lock/warp refusals do not synthesize success,
and focus loss never relocks automatically. Frame pacing really sleeps; it does
not busy-wait or promise vsync/scanout completion.

The canvas stays fixed until `CloseWindow`; `SetWindowSize` warns and preserves
it because upstream renderer resize does not expose allocation failure safely.
OS window placement, minimize/maximize, monitor choice, DPI/refresh claims and
GPU APIs are outside this profile. Unsupported window controls warn without
changing state. Render textures and the wider raylib API need qualification.

## Editable example

The recipe stages `bin/raylib-shapes.pxb`, separate from ordinary boot programs,
and `share/raylib/examples/shapes.c` in the development prefix. It has generated
assets and requests only memory, clock, display, keyboard and optional pointer.
The parent supplies opt-in `raylib-shapes` and `raylib-dev` package recipes.

`raylib-shapes [width height]` defaults to 960x540 at a 60 FPS cap. Space pauses
the texture, F1 switches 2D/3D, the wheel changes the circle, L toggles native
cursor lock, W requests centre positioning, and Escape exits. RGBY corners make
orientation visible. The near orange cube is drawn before the distant blue one,
so depth occlusion is visible. Exit prints frames, elapsed time and mean FPS;
this measures the capped application, not exclusive renderer/CPU cost.
