-- Read-only resource views copied from the exact selection libuv launched.
-- Each buffer pins its original root through server exit and package switches.
local M = {}
M.views = {}

local registered = {}
local buffers = {}
local group = vim.api.nvim_create_augroup('nvim.pyxis.roots', { clear = true })

local function forget_unused(root)
  if next(M.views[root] or {}) then
    return
  end
  for _, pinned in pairs(buffers) do
    if pinned.root == root then
      return
    end
  end
  M.views[root] = nil
end

local function read_view(args)
  local root, rest = args.match:match('^([%w._+-]+)://(.*)$')
  local pinned = buffers[args.buf]
  if pinned and pinned.root ~= root then
    buffers[args.buf] = nil
    forget_unused(pinned.root)
    pinned = nil
  end
  local view = pinned and pinned.view
  if not view then
    for _, candidate in pairs(M.views[root] or {}) do
      if view and view:same(candidate) ~= true then
        error('Ambiguous language-server resource root: ' .. root .. '://')
      end
      view = candidate
    end
    assert(view, 'No active language-server resource root: ' .. root .. '://')
  end
  local bytes = view:read(rest)
  buffers[args.buf] = { root = root, view = view }
  local lines = vim.split(bytes, '\n', { plain = true })
  if lines[#lines] == '' then
    table.remove(lines)
  end
  local buf = args.buf
  vim.bo[buf].swapfile = false
  vim.bo[buf].modifiable = true
  vim.api.nvim_buf_set_lines(buf, 0, -1, false, lines)
  vim.bo[buf].modified = false
  vim.bo[buf].readonly = true
  vim.bo[buf].modifiable = false
  vim.bo[buf].filetype = vim.filetype.match({ filename = args.match, buf = buf }) or ''
end

vim.api.nvim_create_autocmd('BufWipeout', {
  group = group,
  callback = function(args)
    local pinned = buffers[args.buf]
    buffers[args.buf] = nil
    if pinned then
      forget_unused(pinned.root)
    end
  end,
})

-- Copies only declared read-only resource directories from the admitted child.
-- The editor's own root always wins. Bindings are independent per process.
function M.add_process(process)
  local binding = {}
  for root, view in pairs(process:pyxis_resources()) do
    if not vim.uv.fs_stat(root .. '://') then
      if not registered[root] then
        vim.api.nvim_create_autocmd('BufReadCmd', {
          group = group,
          pattern = root .. '://*',
          callback = read_view,
        })
        registered[root] = true
      end
      M.views[root] = M.views[root] or {}
      M.views[root][binding] = view
      binding[root] = true
    end
  end
  return binding
end

function M.remove_process(binding)
  for root in pairs(binding) do
    M.views[root][binding] = nil
    forget_unused(root)
  end
end

return M
