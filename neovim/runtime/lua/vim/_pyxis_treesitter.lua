-- Pyxis: start Tree-sitter highlighting for every statically linked grammar.
-- Upstream ftplugins already do this for Lua, Markdown, help and query; C and
-- Vim would otherwise keep their legacy syntax files. To turn it off, add this
-- to init.lua: vim.api.nvim_clear_autocmds({ group = 'pyxis_treesitter' })

local group = vim.api.nvim_create_augroup('pyxis_treesitter', {})

vim.api.nvim_create_autocmd('FileType', {
  group = group,
  pattern = { 'c', 'lua', 'markdown', 'query', 'vim', 'help' },
  callback = function(args)
    if not vim.treesitter.highlighter.active[args.buf] then
      -- A language without a parser leaves the legacy syntax in place.
      pcall(vim.treesitter.start, args.buf)
    end
  end,
})
