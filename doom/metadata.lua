return {
  source = {
    url = "https://github.com/ozkl/doomgeneric.git",
    commit = "dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284",
  },
  license = "GPL-2.0-or-later",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-pyxis-runtime-boundary.patch",
    "patches/0002-demo-name-lifetime-and-error-exit.patch",
  },
  outputs = {
    executable = "bin/doom.pxe",
    license = "share/licenses/doom/LICENSE",
  },
}
