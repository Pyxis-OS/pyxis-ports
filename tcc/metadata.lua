return {
  source = {
    url = "https://github.com/TinyCC/tinycc.git",
    mirror = "https://git.internal/mirrors/tinycc",
    commit = "3dc99dbc82f8e07308c5d398136803e62f9676df",
  },
  license = "LGPL-2.1-or-later; libtcc1.c has GPL-2.0-or-later with its stated linking exception",
  dependencies = {
    host = { "make", "cc" },
    pyxis = { "libc", "libpyxis", "libterm" },
  },
  patches = {
    "patches/0001-pyxis-object-target.patch",
    "patches/0002-libgcc-conversion-ownership.patch",
    "patches/0003-reserve-fp-scratch.patch",
    "patches/0004-native-streams-and-paths.patch",
    "patches/0005-guest-driver.patch",
    "patches/0006-native-p1f-output.patch",
    "patches/0007-guest-clocks.patch",
  },
  outputs = {
    host_compiler = "host/bin/x86_64-pyxis-tcc",
    compiler = "bin/tcc.pxe",
    support = "lib/tcc/libtcc1.a",
    license = "share/licenses/tcc/COPYING",
    provenance = "share/tcc/source.txt",
  },
}
