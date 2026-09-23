return {
  source = {
    url = "https://github.com/antirez/kilo.git",
    commit = "323d93b29bd89a2cb446de90c4ed4fea1764176e",
  },
  license = "BSD-2-Clause",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libterm", "libpyxis" },
  },
  patches = {
    "patches/0001-pyxis-terminal-and-file-access.patch",
    "patches/0002-editor-allocation-and-bounds.patch",
  },
  outputs = {
    executable = "bin/kilo.pxe",
    license = "share/licenses/kilo/LICENSE",
  },
}
