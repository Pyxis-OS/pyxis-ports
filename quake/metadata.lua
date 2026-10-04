return {
  source = {
    url = "https://github.com/erysdren/quakegeneric.git",
    commit = "13052102577c629650cf07a46151a4b6e1b19c3c",
  },
  license = "GPL-2.0-or-later",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {
    "patches/0001-64-bit-quakec-strings.patch",
    "patches/0002-writable-game-directory.patch",
  },
  outputs = {
    executable = "bin/quake.pxe",
    license = "share/licenses/quake/LICENSE",
  },
}
