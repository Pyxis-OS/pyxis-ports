return {
  source = {
    url = "https://github.com/nothings/stb.git",
    commit = "0bc88af4de5fb022db643c2d8e549a0927749354",
    archive = {
      url = "https://github.com/nothings/stb/archive/0bc88af4de5fb022db643c2d8e549a0927749354.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/nothings/stb/archive/0bc88af4de5fb022db643c2d8e549a0927749354.tar.gz",
      sha256 = "0e05332f08c3c45405fda367ecd290558f3d8493bc9f536cc53e0e00ebe98898",
    },
  },
  version = "2.29",
  license = "MIT OR Unlicense",
  dependencies = {
    host = { "curl", "sha256sum", "tar", "gzip", "install", "cp" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    header = "dev/include/stb_image.h",
    pc_stb = "dev/lib/pkgconfig/stb.pc",
    license = "dev/share/licenses/stb/LICENSE",
    notice = "dev/share/licenses/stb/PORT-NOTICE",
    pyxis_license = "dev/share/licenses/stb/MPL-2.0",
    licensing = "dev/share/licenses/stb/LICENSING.md",
    provenance = "dev/share/stb/source.txt",
    recipe = "dev/share/stb/pyxis/build.lua",
    recipe_metadata = "dev/share/stb/pyxis/metadata.lua",
  },
}
