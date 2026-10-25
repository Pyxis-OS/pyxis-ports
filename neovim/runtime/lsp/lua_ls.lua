-- Pyxis: enable with vim.lsp.enable('lua_ls') after installing lua-language-server.
-- Project JSON/JSONC settings override these Neovim configuration defaults.

local markers = { '.luarc.json', '.luarc.jsonc', '.git' }

--- @type vim.lsp.Config
return {
  cmd = { 'lua-language-server' },
  filetypes = { 'lua' },
  root_dir = function(bufnr, on_dir)
    local name = vim.api.nvim_buf_get_name(bufnr)
    local root = name:match('^([%w._+-]+)://')
    if root == 'nvim_runtime' or (root and require('vim._pyxis_roots').views[root]) then
      -- Read-only API and server resources belong to an existing session.
      local client = vim.lsp.get_clients({ name = 'lua_ls', bufnr = bufnr })[1]
        or vim.lsp.get_clients({ name = 'lua_ls' })[1]
      if client then
        on_dir(client.root_dir)
      end
      return
    end
    -- Group markers at equal priority; a file without one uses its directory.
    on_dir(vim.fs.root(bufnr, { markers }) or vim.fs.dirname(name))
  end,
  capabilities = {
    workspace = {
      didChangeWatchedFiles = { dynamicRegistration = false },
    },
  },
  settings = {
    Lua = {
      runtime = { version = 'Lua 5.1' },
      workspace = { library = { 'nvim_runtime://lua' } },
    },
  },
}
