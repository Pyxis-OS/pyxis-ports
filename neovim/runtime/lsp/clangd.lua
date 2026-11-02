-- Pyxis: clangd from the hosted toolchain. Enable it in init.lua with
-- vim.lsp.enable('clangd'); change it with vim.lsp.config('clangd', { ... }).
--
-- clangd sizes its worker pool from the CPU allowance libc reports
-- (sysconf(_SC_NPROCESSORS_ONLN) through libc++), so no -j. Background indexing
-- would write .cache/clangd into the project and stays off.

local markers = { 'compile_commands.json', 'compile_flags.txt', '.git' }

--- @type vim.lsp.Config
return {
  cmd = {
    'clangd',
    '--resource-dir=clang_resources://',
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
