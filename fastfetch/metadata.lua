return {
  source = {
    url = "https://github.com/fastfetch-cli/fastfetch.git",
    commit = "0c3b852bf7bad2837a814c7a31bf332092048a2b",
  },
  license = "MIT AND MPL-2.0",
  dependencies = {
    host = { "make", "cmake" },
    pyxis = { "libc", "libterm", "libpyxis" },
  },
  patches = {
    "patches/0001-pyxis-build.patch",
    "patches/0002-pyxis-native-adapters.patch",
    "patches/0003-pyxis-logo.patch",
  },
  outputs = {
    executable = "bin/fastfetch.pxe",
    license = "share/licenses/fastfetch/LICENSE",
    yyjson_license = "share/licenses/fastfetch/yyjson.h",
    pyxis_license = "share/licenses/fastfetch/MPL-2.0",
    notice = "share/licenses/fastfetch/PORT-NOTICE",
  },
}
