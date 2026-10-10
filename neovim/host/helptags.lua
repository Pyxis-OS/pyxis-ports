local source = table.remove(arg, 1)
package.path = source .. '/runtime/lua/?.lua;' .. package.path
_G.vim = require 'vim._core.shared'

local function quote(value)
  assert(not value:find('\0', 1, true))
  return "'" .. value:gsub("'", "'\\''") .. "'"
end

-- Host-only adaptation of the upstream generator's directory reader. GNU find
-- emits real entry names/types; NUL framing preserves spaces and newlines.
local function opendir(path, callback, count)
  assert(callback == nil and count == 1)
  local pipe = assert(io.popen('find -- ' .. quote(path)
    .. " -mindepth 1 -maxdepth 1 -printf '%f\\0%y\\0'", 'r'))
  local entries = assert(pipe:read('*a'))
  assert(pipe:close(), 'host directory enumeration failed: ' .. path)
  local position = 1
  local kinds = { f = 'file', d = 'directory', l = 'link', b = 'block',
    c = 'char', p = 'fifo', s = 'socket' }
  return {
    readdir = function()
      if position > #entries then
        return nil
      end
      local name_end = assert(entries:find('\0', position, true))
      local kind_end = assert(entries:find('\0', name_end + 1, true))
      local name = entries:sub(position, name_end - 1)
      local kind = entries:sub(name_end + 1, kind_end - 1)
      position = kind_end + 1
      return { { name = name, type = assert(kinds[kind], 'unknown host entry type') } }
    end,
  }
end

vim.uv = { fs_opendir = opendir }
arg[0] = source .. '/src/gen/gen_helptags.lua'
assert(loadfile(arg[0]))()
