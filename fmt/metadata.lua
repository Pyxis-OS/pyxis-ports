return {
  source = {
    url = "https://github.com/fmtlib/fmt.git",
    mirror = "https://git.internal/mirrors/fmt",
    commit = "1be298e1bd68957e4cd352e1f676f00e07dcfb57",
  },
  version = "12.2.0",
  license = "MIT",
  dependencies = {
    host = { "make", "cmake" },
    pyxis = { "libc", "libc++" },
  },
  patches = {},
  outputs = {
    dev_pyxis_license = "dev/share/licenses/fmt/MPL-2.0",
    dev_licensing = "dev/share/licenses/fmt/LICENSING.md",
    pc_fmt = "dev/lib/pkgconfig/fmt.pc",

    library = "dev/lib/libfmt.a",
    header = "dev/include/fmt/format.h",
    package = "dev/lib/cmake/fmt/fmt-config.cmake",
    license = "dev/share/licenses/fmt/LICENSE",
    provenance = "dev/share/fmt/source.txt",
  },
}
