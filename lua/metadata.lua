return {
  source = {
    url = "https://github.com/lua/lua.git",
    mirror = "https://git.internal/mirrors/lua",
    commit = "7579fc9d7ed90240487251dfb69168f8e64e9294",
  },
  license = "MIT",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libpyxis" },
    ports = { "mbedtls" },
  },
  patches = {
    "patches/0001-upstream-gc-parameter-fix.patch",
    "patches/0002-pyxis-runtime.patch",
    "patches/0003-pyxis-file-abandonment.patch",
  },
  outputs = {
    pc_lua = "dev/lib/pkgconfig/lua.pc",

    executable = "bin/lua.pxb/app/bin/lua.pxe",
    manifest = "bin/lua.pxb/manifest.json",
    commands = "share/lua/pyxis/commands.lua",
    license = "share/licenses/lua/lua.h",
    library = "dev/lib/liblua.a",
    api_header = "dev/include/lua.h",
    config_header = "dev/include/luaconf.h",
    auxiliary_header = "dev/include/lauxlib.h",
    library_header = "dev/include/lualib.h",
  },
}
