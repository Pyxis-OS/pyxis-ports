return {
  source = {
    url = "https://github.com/lua/lua.git",
    commit = "7579fc9d7ed90240487251dfb69168f8e64e9294",
  },
  license = "MIT",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-upstream-gc-parameter-fix.patch",
    "patches/0002-pyxis-runtime.patch",
  },
  outputs = {
    executable = "bin/lua.pxe",
    license = "share/licenses/lua/lua.h",
  },
}
