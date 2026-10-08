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
  patches = {
    "patches/0001-disable-locale-without-libcxx-localization.patch",
    "patches/0002-omit-wstring-without-libcxx-wide-characters.patch",
  },
  outputs = {
    library = "dev/lib/libfmt.a",
    header = "dev/include/fmt/format.h",
    package = "dev/lib/cmake/fmt/fmt-config.cmake",
    license = "dev/share/licenses/fmt/LICENSE",
    provenance = "dev/share/fmt/source.txt",
  },
}
