return {
  source = {
    url = "https://github.com/raysan5/raylib",
    commit = "02566453880e06bec97f5b8160fa6fe1e98397ca",
    archive = {
      url = "https://github.com/raysan5/raylib/archive/refs/tags/6.2.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/raysan5/raylib/archive/refs/tags/6.2.tar.gz",
      sha256 = "df7a13129b1bec9e73a6962efff170beba310ee6eab8f94cd000b7316b4db5ee",
    },
  },
  version = "6.2",
  license = "Zlib and MIT; embedded generators include public-domain dedications",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-native-file-paths-and-metadata.patch",
    "patches/0002-pyxis-sdl-software-platform.patch",
    "patches/0003-pyxis-sdl-audio.patch",
  },
  outputs = {
    dev_pyxis_license = "dev/share/licenses/raylib/MPL-2.0",
    dev_licensing = "dev/share/licenses/raylib/LICENSING.md",
    pc_raylib = "dev/lib/pkgconfig/raylib.pc",

    library = "dev/lib/libraylib.a",
    header = "dev/include/raylib.h",
    raymath_header = "dev/include/raymath.h",
    rlgl_header = "dev/include/rlgl.h",
    configuration = "dev/share/raylib/pyxis_config.h",
    audio_backend = "dev/share/raylib/pyxis_miniaudio.h",
    package = "dev/lib/cmake/raylib/raylibConfig.cmake",
    license = "dev/share/licenses/raylib/LICENSE",
    third_party = "dev/share/licenses/raylib/THIRD-PARTY-LICENSES",
    notice = "dev/share/licenses/raylib/PORT-NOTICE",
    provenance = "dev/share/raylib/source.txt",
    example = "bin/raylib-shapes.pxb/app/bin/raylib-shapes.pxe",
    manifest = "bin/raylib-shapes.pxb/manifest.json",
    example_source = "dev/share/raylib/examples/shapes.c",
    example_manifest = "dev/share/raylib/examples/manifest.json",
  },
}
