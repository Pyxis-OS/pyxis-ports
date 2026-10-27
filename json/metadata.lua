return {
  source = {
    url = "https://github.com/nlohmann/json",
    archive = {
      url = "https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz",
      sha256 = "4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187",
    },
  },
  version = "3.12.0",
  license = "MIT",
  dependencies = { host = { "make", "cmake" }, pyxis = { "libc", "libc++" } },
  patches = {},
  outputs = {
    header = "dev/include/nlohmann/json.hpp",
    package = "dev/share/cmake/nlohmann_json/nlohmann_jsonConfig.cmake",
    pc = "dev/lib/pkgconfig/nlohmann_json.pc",
    license = "dev/share/licenses/json/LICENSE.MIT",
    provenance = "dev/share/json/source.txt",
    pyxis_license = "dev/share/licenses/json/MPL-2.0",
    licensing = "dev/share/licenses/json/LICENSING.md",
    source = "dev/share/sources/json/build.lua",
  },
}
