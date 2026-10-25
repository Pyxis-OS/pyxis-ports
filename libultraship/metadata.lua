return {
  source = {
    url = "https://github.com/kenix3/libultraship",
    commit = "da6daf1a50a5e46d6641408d52227e31e85e46df",
    archive = {
      url = "https://github.com/kenix3/libultraship/archive/da6daf1a50a5e46d6641408d52227e31e85e46df.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/kenix3/libultraship/archive/da6daf1a50a5e46d6641408d52227e31e85e46df.tar.gz",
      sha256 = "1827b7852e8de7a97e8a837190c8ce2eec4909aa6aca57356821a09975fcb82f",
    },
  },
  version = "0.git.da6daf1a50a5",
  license = "MIT AND OFL-1.1",
  dependencies = {
    host = { "make", "cmake", "python3", "curl", "sha256sum", "tar", "install" },
    pyxis = { "libc", "libpyxis", "libc++", "libc++abi", "libunwind" },
  },
  patches = { "patches/0001-native-platform.patch", "patches/0002-font-atlas-refresh.patch", "patches/0003-keyboard-focus-release.patch", "patches/0004-config-save-results.patch" },
  outputs = {
    library = "dev/lib/libultraship.a",
    header = "dev/include/ship/Context.h",
    profile = "dev/include/libultraship/pyxis_profile.h",
    pc = "dev/lib/pkgconfig/libultraship.pc",
    cmake = "dev/lib/cmake/LibUltraShip/LibUltraShipConfig.cmake",
    license = "dev/share/licenses/libultraship/LICENSE",
    font_license = "dev/share/licenses/libultraship/FontAwesome-OFL-1.1.txt",
    executable = "bin/lus-demo.pxb/app/bin/lus-demo.pxe",
    manifest = "bin/lus-demo.pxb/manifest.json",
    fixture = "bin/lus-demo.pxb/app/fixture.o2r",
    source = "dev/share/libultraship/source.txt",
  },
}
