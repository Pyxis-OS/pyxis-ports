return {
  source = {
    url = "https://github.com/leethomason/tinyxml2.git",
    commit = "321ea883b7190d4e85cae5512a12e5eaa8f8731f",
    archive = {
      url = "https://github.com/leethomason/tinyxml2/archive/321ea883b7190d4e85cae5512a12e5eaa8f8731f.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/leethomason/tinyxml2/archive/321ea883b7190d4e85cae5512a12e5eaa8f8731f.tar.gz",
      sha256 = "d5ae097578717b42b4f05f2c6b2d09f473a1c30d1552ca4e2dcb4eab5b34de84",
    },
  },
  version = "10.0.0",
  license = "Zlib",
  dependencies = {
    host = { "make", "cmake", "curl", "sha256sum", "tar", "gzip", "install", "cp" },
    pyxis = { "libc", "libc++" },
  },
  patches = {},
  outputs = {
    library = "dev/lib/libtinyxml2.a",
    header = "dev/include/tinyxml2.h",
    package = "dev/lib/cmake/tinyxml2/tinyxml2-config.cmake",
    pc_tinyxml2 = "dev/lib/pkgconfig/tinyxml2.pc",
    license = "dev/share/licenses/tinyxml2/LICENSE.txt",
    notice = "dev/share/licenses/tinyxml2/PORT-NOTICE",
    pyxis_license = "dev/share/licenses/tinyxml2/MPL-2.0",
    licensing = "dev/share/licenses/tinyxml2/LICENSING.md",
    provenance = "dev/share/tinyxml2/source.txt",
    recipe = "dev/share/tinyxml2/pyxis/build.lua",
    recipe_metadata = "dev/share/tinyxml2/pyxis/metadata.lua",
  },
}
