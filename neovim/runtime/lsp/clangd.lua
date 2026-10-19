-- Pyxis: clangd from the hosted toolchain. Enable it in init.lua with
-- vim.lsp.enable('clangd'); change it with vim.lsp.config('clangd', { ... }).
--
-- libc reports no CPU count, so -j sets the worker count. Background indexing
-- would write .cache/clangd into the project and stays off.

local markers = { 'compile_commands.json', 'compile_flags.txt', '.git' }

--- @type vim.lsp.Config
return {
  cmd = {
    'home://toolchain/clang/payload/clangd.pxb',
    '--resource-dir=clang_resources://',
    '-j=4',
    '--background-index=false',
    '--pch-storage=memory',
  },
  filetypes = { 'c', 'cpp' },
  root_dir = function(bufnr, on_dir)
    local name = vim.api.nvim_buf_get_name(bufnr)
    local root = name:match('^([%w._+-]+)://')
    if root and require('vim._pyxis_roots').views[root] then
      -- A header from clangd's own roots belongs to the running session.
      local client = vim.lsp.get_clients({ name = 'clangd' })[1]
      if client then
        on_dir(client.root_dir)
      end
      return
    end
    on_dir(vim.fs.root(bufnr, markers) or vim.fs.dirname(name))
  end,
}
