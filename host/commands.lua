-- Host builds only. Recipes are trusted Lua; quote argv at the shell boundary.
local function quote(value)
  assert(type(value) == "string" and not value:find("\0", 1, true),
    "command arguments must be strings without NUL")
  return "'" .. value:gsub("'", "'\\''") .. "'"
end

local function command(arguments)
  local words = {}
  for i, value in ipairs(arguments) do
    words[i] = quote(value)
  end
  return table.concat(words, " ")
end

local function run(arguments)
  local line = command(arguments)
  print("+ " .. line)
  local ok, reason, status = os.execute(line)
  if not ok then
    error(string.format("command failed (%s %s): %s", reason, status, line), 0)
  end
end

local function capture(arguments)
  local line = command(arguments)
  local pipe = assert(io.popen(line, "r"))
  local output = pipe:read("a")
  local ok, reason, status = pipe:close()
  if not ok then
    error(string.format("command failed (%s %s): %s", reason, status, line), 0)
  end
  return (output:gsub("\n$", ""))
end

return { run = run, capture = capture }
