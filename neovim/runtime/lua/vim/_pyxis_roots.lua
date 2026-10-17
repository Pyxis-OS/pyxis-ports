-- Pyxis: read-only views of roots that a language server holds and the editor
-- does not. clangd reports headers as sysroot://usr/include/stdio.h; the
-- editor opens that name from the server bundle's own directory, so the buffer
-- keeps the name clangd knows. A view is an editor guard only: it grants no
-- authority and withholds none from the editor's own roots.
local M = {}

--- Root name to the directory that holds its files.
--- @type table<string, string>
M.views = {}

local group = vim.api.nvim_create_augroup('nvim.pyxis.roots', { clear = true })

--- @param args vim.api.keyset.create_autocmd.callback_args
local function read_view(args)
  local root, rest = args.match:match('^([%w._+-]+)://(.*)$')
  local lines = vim.fn.readfile(M.views[root] .. '/' .. rest)
  local buf = args.buf
  vim.bo[buf].swapfile = false
  vim.bo[buf].modifiable = true
  vim.api.nvim_buf_set_lines(buf, 0, -1, false, lines)
  vim.bo[buf].modified = false
  vim.bo[buf].readonly = true
  vim.bo[buf].modifiable = false
  vim.bo[buf].filetype = vim.filetype.match({ filename = args.match, buf = buf }) or ''
end

--- Shows the resource roots a bundle declares in its manifest. A root the
--- editor holds itself always wins, so it never gets a view.
--- @param bundle string path of a .pxb bundle
function M.add_bundle(bundle)
  if not bundle:match('%.pxb$') then
    return
  end
  local manifest = vim.json.decode(table.concat(vim.fn.readfile(bundle .. '/manifest.json'), '\n'))
  for root, dir in pairs(manifest.resource_dirs or {}) do
    if not vim.uv.fs_stat(root .. '://') then
      if not M.views[root] then
        vim.api.nvim_create_autocmd('BufReadCmd', {
          group = group,
          pattern = root .. '://*',
          callback = read_view,
        })
      end
      M.views[root] = bundle .. '/app/' .. dir
    end
  end
end

return M
