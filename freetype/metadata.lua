return {
  source = {
    url = "https://download.savannah.gnu.org/releases/freetype/freetype-2.15.0.tar.xz",
    archive = {
      url = "https://download.savannah.gnu.org/releases/freetype/freetype-2.15.0.tar.xz",
      mirror = "https://repo.internal/repository/raw-sourceforge/freetype/freetype2/2.15.0/freetype-2.15.0.tar.xz",
      sha256 = "93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5",
    },
  },
  version = "2.15.0",
  license = "FTL OR GPL-2.0-or-later; bundled zlib under Zlib",
  dependencies = {
    host = { "make", "cmake", "curl", "sha256sum", "tar", "xz", "install", "cp", "sed" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    library = "dev/lib/libfreetype.a",
    header = "dev/include/freetype2/ft2build.h",
    package = "dev/lib/cmake/freetype/freetype-config.cmake",
    pc_freetype2 = "dev/lib/pkgconfig/freetype2.pc",
    license = "dev/share/licenses/freetype/LICENSE.TXT",
    ftl = "dev/share/licenses/freetype/FTL.TXT",
    gpl = "dev/share/licenses/freetype/GPLv2.TXT",
    notice = "dev/share/licenses/freetype/PORT-NOTICE",
    pyxis_license = "dev/share/licenses/freetype/MPL-2.0",
    licensing = "dev/share/licenses/freetype/LICENSING.md",
    provenance = "dev/share/freetype/source.txt",
    recipe = "dev/share/freetype/pyxis/build.lua",
    recipe_metadata = "dev/share/freetype/pyxis/metadata.lua",
  },
}
