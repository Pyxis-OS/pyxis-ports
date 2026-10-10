return {
  source = {
    url = "https://github.com/neovim/neovim.git",
    commit = "5885a30e1e1225349079e7a1c4a3848aa8e43e42",
    archive = {
      url = "https://github.com/neovim/neovim/archive/5885a30e1e1225349079e7a1c4a3848aa8e43e42.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/neovim/neovim/archive/5885a30e1e1225349079e7a1c4a3848aa8e43e42.tar.gz",
      sha256 = "314bb8d8695cc2c1c9b69e6c93df8c75108ca66588dfffb81c42369d2f85c90a",
    },
    -- Pristine native tools, separate from the adapted target Lua/LPeg recipe.
    extra = {
      { name = "host_lua", url = "https://www.lua.org/ftp/lua-5.1.5.tar.gz",
        archive = {
          url = "https://www.lua.org/ftp/lua-5.1.5.tar.gz",
          mirror = "https://repo.internal/repository/raw-lua/ftp/lua-5.1.5.tar.gz",
          sha256 = "2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333",
        } },
      { name = "host_lpeg", url = "https://github.com/neovim/deps",
        archive = {
          url = "https://github.com/neovim/deps/raw/d495ee6f79e7962a53ad79670cb92488abe0b9b4/opt/lpeg-1.1.0.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/neovim/deps/raw/d495ee6f79e7962a53ad79670cb92488abe0b9b4/opt/lpeg-1.1.0.tar.gz",
          sha256 = "4b155d67d2246c1ffa7ad7bc466c1ea899bbc40fef0257cc9c03cecbaed4352a",
        } },
    },
  },
  version = "0.12.5",
  license = "Apache-2.0 AND Vim",
  dependencies = {
    host = { "make", "cmake", "cc", "ar", "ranlib", "curl", "sha256sum", "tar", "gzip", "install", "git", "cp" },
    pyxis = { "libc", "libpyxis", "lua51", "libuv", "utf8proc", "tree-sitter" },
  },
  patches = {},
  outputs = {
    manifest = "share/neovim/nvim.pxb/manifest.json",
    executable = "share/neovim/nvim.pxb/app/bin/nvim.pxe",
    sysinit = "share/neovim/nvim.pxb/app/share/nvim/runtime/sysinit.vim",
    colorscheme = "share/neovim/nvim.pxb/app/share/nvim/runtime/colors/pyxis.vim",
    runtime = "share/neovim/nvim.pxb/app/share/nvim/runtime/filetype.lua",
    license = "share/licenses/neovim/LICENSE.txt",
    host_lua_license = "share/licenses/neovim/host-lua-COPYRIGHT",
    host_lpeg_license = "share/licenses/neovim/host-lpeg.html",
    provenance = "share/neovim/source.txt",
  },
}
