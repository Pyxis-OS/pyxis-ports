local patches = {
  "patches/0001-pyxis-object-target.patch",
  "patches/0002-runtime-conversion-ownership.patch",
  "patches/0003-reserve-fp-scratch.patch",
  "patches/0004-native-streams-and-paths.patch",
  "patches/0005-guest-driver.patch",
  "patches/0006-native-p1f-output.patch",
  "patches/0007-guest-clocks.patch",
}

-- Everything the recipe stages, so the Pyxis build can derive its dependencies from here.
local outputs = {
  host_compiler = "host/bin/x86_64-pyxis-tcc",
  compiler = "bin/tcc.pxe",
  support = "lib/tcc/libtcc1.a",
  stddef = "lib/tcc/include/stddef.h",
  stdarg = "lib/tcc/include/stdarg.h",
  stdbool = "lib/tcc/include/stdbool.h",
  float = "lib/tcc/include/float.h",
  license = "share/licenses/tcc/COPYING",
  libtcc1_notice = "share/licenses/tcc/libtcc1.c",
  va_list_notice = "share/licenses/tcc/va_list.c",
  builtin_notice = "share/licenses/tcc/builtin.c",
  provenance = "share/tcc/source.txt",
}
for index, patch in ipairs(patches) do
  outputs["patch" .. index] = "share/tcc/" .. patch
end

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
  patches = patches,
  outputs = outputs,
}
