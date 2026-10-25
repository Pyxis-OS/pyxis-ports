return {
  source = {
    url = "https://github.com/lua/lua.git",
    mirror = "https://git.internal/mirrors/lua",
    commit = "7579fc9d7ed90240487251dfb69168f8e64e9294",
  },
  license = "MIT",
  dependencies = {
    host = { "make" },
    pyxis = { "libc", "libpyxis", "libsha256" },
  },
  patches = {
    "patches/0001-upstream-gc-parameter-fix.patch",
    "patches/0002-pyxis-runtime.patch",
    "patches/0003-pyxis-file-abandonment.patch",
  },
  outputs = {
    dev_pyxis_license = "dev/share/licenses/lua/MPL-2.0",
    dev_licensing = "dev/share/licenses/lua/LICENSING.md",
    pc_lua = "dev/lib/pkgconfig/lua.pc",

    executable = "bin/lua.pxb/app/bin/lua.pxe",
    manifest = "bin/lua.pxb/manifest.json",
    commands = "share/lua/pyxis/commands.lua",
    license = "share/licenses/lua/lua.h",
    dev_license = "dev/share/licenses/lua/lua.h",
    dev_notice = "dev/share/licenses/lua/PORT-NOTICE",
    dev_provenance = "dev/share/lua/source.txt",
    dev_configuration_source = "dev/share/sources/lua/config.h",
    library = "dev/lib/liblua.a",
    api_header = "dev/include/lua.h",
    config_header = "dev/include/luaconf.h",
    auxiliary_header = "dev/include/lauxlib.h",
    library_header = "dev/include/lualib.h",
  },
}
