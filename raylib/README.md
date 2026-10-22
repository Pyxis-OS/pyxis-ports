# raylib 6.2 for Pyxis

Task 1 exports a static SDL2/rlsw build and its source closure. It compiles and
links against the real Pyxis SDK; drawing, input, scaling and frame pacing still
need the SDL integration task. Do not treat an archive build as runtime support.
Audio is omitted: raudio calls do not link.

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
