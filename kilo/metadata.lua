return {
  source = {
    url = "https://github.com/antirez/kilo.git",
    mirror = "https://git.internal/mirrors/kilo",
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
    "patches/0003-monotonic-status-expiry.patch",
    "patches/0004-terminal-eof.patch",
    "patches/0005-page-navigation-bounds.patch",
    "patches/0006-ctrl-c-passthrough.patch",
    "patches/0007-terminal-resize.patch",
  },
  outputs = {
    executable = "bin/kilo.pxe",
    license = "share/licenses/kilo/LICENSE",
  },
}
