return {
  source = {
    url = "https://www.lua.org/ftp/lua-5.1.5.tar.gz",
    archive = {
      url = "https://www.lua.org/ftp/lua-5.1.5.tar.gz",
      mirror = "https://repo.internal/repository/raw-lua/ftp/lua-5.1.5.tar.gz",
      sha256 = "2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333",
    },
    -- The pins Neovim 0.12.5 itself uses (cmake.deps/deps.txt).
    extra = {
      { name = "lpeg", url = "https://github.com/neovim/deps",
        archive = {
          url = "https://github.com/neovim/deps/raw/d495ee6f79e7962a53ad79670cb92488abe0b9b4/opt/lpeg-1.1.0.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/neovim/deps/raw/d495ee6f79e7962a53ad79670cb92488abe0b9b4/opt/lpeg-1.1.0.tar.gz",
          sha256 = "4b155d67d2246c1ffa7ad7bc466c1ea899bbc40fef0257cc9c03cecbaed4352a",
        } },
      { name = "luv", url = "https://github.com/luvit/luv",
        archive = {
          url = "https://github.com/luvit/luv/archive/1.52.1-0.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/luvit/luv/archive/1.52.1-0.tar.gz",
          sha256 = "e8b8774b31d24be4fcf2b021b90599ecccc8e476c61efcc59c3c10cab813a885",
        },
        patches = { "patches/luv-0001-pyxis-native.patch" } },
      { name = "compat53", url = "https://github.com/lunarmodules/lua-compat-5.3",
        archive = {
          url = "https://github.com/lunarmodules/lua-compat-5.3/archive/v0.13.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/lunarmodules/lua-compat-5.3/archive/v0.13.tar.gz",
          sha256 = "f5dc30e7b1fda856ee4d392be457642c1f0c259264a9b9bfbcb680302ce88fc2",
        },
        patches = { "patches/compat53-0001-no-reopen.patch" } },
    },
  },
  version = "5.1.5",
  license = "MIT AND Apache-2.0",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install", "git" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = { "patches/0001-pyxis-runtime.patch" },
  outputs = {
    lua_library = "dev/lib/liblua5.1.a",
    lpeg_library = "dev/lib/liblpeg.a",
    luv_library = "dev/lib/libluv.a",
    api_header = "dev/include/lua5.1/lua.h",
    config_header = "dev/include/lua5.1/luaconf.h",
    auxiliary_header = "dev/include/lua5.1/lauxlib.h",
    library_header = "dev/include/lua5.1/lualib.h",
    luv_header = "dev/include/luv/luv.h",
    manifest = "share/lua51/lua5.1.pxb/manifest.json",
    interpreter = "share/lua51/lua5.1.pxb/app/bin/lua5.1.pxe",
    license = "share/licenses/lua51/COPYRIGHT",
    lpeg_license = "share/licenses/lua51/lpeg.html",
    luv_license = "share/licenses/lua51/luv-LICENSE.txt",
    compat53_license = "share/licenses/lua51/compat53-LICENSE",
    provenance = "share/lua51/source.txt",
  },
}
