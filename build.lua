-- Recipes are trusted Lua. Quote argument arrays at the shell boundary rather
-- than asking each recipe to concatenate command strings.
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

local function require_file(path)
  local file = assert(io.open(path, "rb"), "missing file: " .. path)
  file:close()
end

local function relative(path)
  assert(type(path) == "string" and path ~= "" and path:sub(1, 1) ~= "/",
    "expected a relative recipe path")
  for part in path:gmatch("[^/]+") do
    assert(part ~= "..", "recipe paths must not escape their directory")
  end
  return path
end

local function make_path(path)
  -- The SDK's exported Make flags use these paths without Make escaping.
  assert(path:match("^[%w_./+%-]+$"),
    "use letters, digits, underscores, dots, slashes, plus and minus in build paths")
  return path
end

local function check_commit(commit, what)
  assert(type(commit) == "string" and #commit == 40 and commit:match("^[0-9a-f]+$"),
    what .. " needs an exact Git commit")
end

local function check_mirror(mirror, what)
  assert(type(mirror) == "string" and mirror:match("^https://"),
    what .. " needs an HTTPS mirror URL")
end

local function check_download(download, what)
  assert(type(download.url) == "string" and download.url:match("^https://"),
    what .. " download needs an HTTPS upstream URL")
  assert(type(download.mirror) == "string" and download.mirror:match("^https://"),
    what .. " download needs an HTTPS mirror URL")
  assert(type(download.sha256) == "string" and #download.sha256 == 64 and
    download.sha256:match("^[0-9a-f]+$"), what .. " download needs a SHA-256 pin")
end

-- Download a mirror copy to file and verify its pinned checksum.
local function download_file(download, file)
  run({ "curl", "--fail", "--location", "--proto", "=https", "--proto-redir", "=https",
    "--output", file, "--", download.mirror })
  assert(capture({ "sha256sum", "--", file }):match("^([0-9a-f]+)") == download.sha256,
    "source download checksum mismatch: " .. download.mirror)
end

-- Extract a release archive without its single top-level directory.
local function extract_archive(file, directory)
  run({ "tar", "--extract", "--file", file, "--directory", directory,
    "--strip-components=1", "--no-same-owner" })
  -- git apply resolves paths from the enclosing repository, which may be
  -- the checkout holding the work directory; give the source its own.
  run({ "git", "init", "--quiet", directory })
end

local function fetch_commit(mirror, commit, directory)
  run({ "git", "init", "--quiet", directory })
  run({ "git", "-C", directory, "fetch", "--depth=1", "--", mirror, commit })
  run({ "git", "-C", directory, "checkout", "--quiet", "--detach", "FETCH_HEAD" })
  assert(capture({ "git", "-C", directory, "rev-parse", "HEAD" }) == commit,
    "source pin mismatch: " .. mirror)
end

local function main()
  if arg[1] == "--help" or not arg[1] then
    print("Usage: lua build.lua PORT --sdk PATH [--work PATH] [--cross-prefix PREFIX]")
    print("The work directory must not exist; default: build/PORT.")
    print("Lua also requires --mbedtls PATH to its configured development prefix.")
    print("Libpng also requires --zlib PATH to its development prefix.")
    print("DevilutionX also requires --zlib, --libpng, --fmt and --sdl2 development prefixes.")
    print("Chocolate Doom also requires --sdl2 PATH to its development prefix.")
    return
  end
  local name, options = arg[1], {}
  assert(name:match("^[a-z0-9][a-z0-9_-]*$"), "invalid port name")
  local allowed = { ["--sdk"] = true, ["--work"] = true, ["--cross-prefix"] = true,
    ["--mbedtls"] = name == "lua", ["--zlib"] = name == "libpng" or name == "devilutionx",
    ["--libpng"] = name == "devilutionx", ["--fmt"] = name == "devilutionx",
    ["--sdl2"] = name == "devilutionx" or name == "chocolate-doom" }
  for i = 2, #arg, 2 do
    local option = arg[i]
    assert(allowed[option] and arg[i + 1] and not options[option],
      "unknown, repeated or incomplete option: " .. option)
    options[option] = arg[i + 1]
  end
  assert(options["--sdk"], "select an exported SDK with --sdk PATH")

  local root = capture({ "realpath", "-e", "--", arg[0]:match("^(.*)/") or "." })
  local catalog = dofile(root .. "/ports.lua")
  assert(catalog[name], "unknown port: " .. name)
  local recipe = make_path(root .. "/" .. relative(catalog[name]))
  local metadata = dofile(recipe .. "/metadata.lua")
  local archive = metadata.source.archive
  local source_file = metadata.source.file
  assert(not (archive and source_file), "select an archive or standalone file")
  local revision = metadata.source.commit
  if not source_file then
    -- A release archive is pinned by its checksum; a Git commit is optional
    -- for projects that publish no Git history.
    if revision ~= nil or not archive then
      check_commit(revision, "source")
    end
    if not archive then
      check_mirror(metadata.source.mirror, "Git source")
    end
  else
    assert(not revision, "standalone files use a checksum instead of a Git commit")
    assert(type(source_file.name) == "string" and
      source_file.name:match("^[%w_][%w_.%-]*$"), "source file needs a plain filename")
    assert(#metadata.patches == 0, "standalone file recipes do not apply patches")
  end
  -- Sources come only from the internal mirrors; url stays upstream provenance.
  local download = archive or source_file
  if download then
    check_download(download, "source")
  end
  -- Extra sources are pinned like the main one and left unpatched; the recipe
  -- finds each at ctx.extra[name].
  local extra = metadata.source.extra or {}
  local extra_names = {}
  for _, entry in ipairs(extra) do
    assert(type(entry.name) == "string" and entry.name:match("^[a-z0-9][a-z0-9_-]*$") and
      not extra_names[entry.name], "extra sources need distinct plain names")
    extra_names[entry.name] = true
    assert(type(entry.url) == "string" and entry.url:match("^https://"),
      "extra source " .. entry.name .. " needs an HTTPS upstream URL")
    if entry.commit ~= nil or not entry.archive then
      check_commit(entry.commit, "extra source " .. entry.name)
    end
    if entry.archive then
      check_download(entry.archive, "extra source " .. entry.name)
    else
      check_mirror(entry.mirror, "extra source " .. entry.name)
    end
  end
  assert(metadata.license and metadata.outputs.license, "record and stage the upstream license")

  local sdk = make_path(capture({ "realpath", "-e", "--", options["--sdk"] }))
  local work = make_path(capture({ "realpath", "-m", "--", options["--work"] or "build/" .. name }))
  assert(work ~= sdk and work:sub(1, #sdk + 1) ~= sdk .. "/",
    "work must be outside the consumed SDK")
  require_file(sdk .. "/share/pyxis.mk")
  require_file(sdk .. "/sysroot/usr/lib/crt0.o")
  local mbedtls
  if name == "lua" then
    assert(options["--mbedtls"], "Lua needs --mbedtls PATH to the configured development prefix")
    mbedtls = make_path(capture({ "realpath", "-e", "--", options["--mbedtls"] }))
    require_file(mbedtls .. "/share/mbedtls.mk")
  end
  local zlib
  if name == "libpng" or name == "devilutionx" then
    assert(options["--zlib"], name .. " needs --zlib PATH to its development prefix")
    zlib = make_path(capture({ "realpath", "-e", "--", options["--zlib"] }))
    require_file(zlib .. "/include/zlib.h")
    require_file(zlib .. "/include/zconf.h")
    require_file(zlib .. "/lib/libz.a")
  end
  -- CMake consumers find these packages; check the files they will look for.
  local libpng, fmt, sdl2
  if name == "devilutionx" then
    for _, option in ipairs({ "--libpng", "--fmt", "--sdl2" }) do
      assert(options[option], "DevilutionX needs " .. option .. " PATH to its development prefix")
    end
    libpng = make_path(capture({ "realpath", "-e", "--", options["--libpng"] }))
    require_file(libpng .. "/include/png.h")
    require_file(libpng .. "/lib/libpng.a")
    fmt = make_path(capture({ "realpath", "-e", "--", options["--fmt"] }))
    require_file(fmt .. "/lib/cmake/fmt/fmt-config.cmake")
  end
  if name == "devilutionx" or name == "chocolate-doom" then
    assert(options["--sdl2"], name .. " needs --sdl2 PATH to its development prefix")
    sdl2 = make_path(capture({ "realpath", "-e", "--", options["--sdl2"] }))
    require_file(sdl2 .. "/lib/cmake/SDL2/SDL2Config.cmake")
  end
  for _, library in ipairs(metadata.dependencies.pyxis) do
    require_file(sdk .. "/sysroot/usr/lib/" .. library .. ".a")
  end
  for _, tool in ipairs(metadata.dependencies.host) do
    run({ "sh", "-c", 'command -v "$1" >/dev/null || { echo "Missing host tool: $1" >&2; exit 1; }', "sh", tool })
  end
  local cross = options["--cross-prefix"] or os.getenv("CROSS_COMPILE") or "x86_64-unknown-pyxis-"
  make_path(cross)
  assert(capture({ cross .. "clang", "-dumpmachine" }) == "x86_64-unknown-pyxis",
    "use the Pyxis target compiler")

  -- Refuse existing work instead of deleting source edits or stale stage files.
  run({ "mkdir", "-p", "--", work:match("^(.*)/") })
  run({ "mkdir", "--", work })
  local source, build, stage = work .. "/source", work .. "/build", work .. "/stage"
  run({ "mkdir", "--", source, build, stage })
  if download then
    local downloaded = source_file and source .. "/" .. source_file.name or work .. "/source.tar"
    download_file(download, downloaded)
    if archive then
      extract_archive(downloaded, source)
    end
  else
    fetch_commit(metadata.source.mirror, revision, source)
  end
  local extra_sources = {}
  if #extra > 0 then
    run({ "mkdir", "--", work .. "/extra" })
  end
  for _, entry in ipairs(extra) do
    local directory = work .. "/extra/" .. entry.name
    run({ "mkdir", "--", directory })
    if entry.archive then
      local downloaded = work .. "/extra/" .. entry.name .. ".tar"
      download_file(entry.archive, downloaded)
      extract_archive(downloaded, directory)
    else
      fetch_commit(entry.mirror, entry.commit, directory)
    end
    extra_sources[entry.name] = directory
  end
  for _, patch in ipairs(metadata.patches) do
    run({ "git", "-C", source, "apply", "--whitespace=error-all", "--", recipe .. "/" .. relative(patch) })
  end

  local build_port = dofile(recipe .. "/build.lua")
  build_port({ sdk = sdk, sysroot = sdk .. "/sysroot", cross_compile = cross,
    recipe = recipe, source = source, build = build, stage = stage,
    metadata = metadata, run = run, mbedtls = mbedtls, zlib = zlib,
    libpng = libpng, fmt = fmt, sdl2 = sdl2, extra = extra_sources })
  for _, output in pairs(metadata.outputs) do
    require_file(stage .. "/" .. relative(output))
  end
  print("Staged " .. name .. " in " .. stage)
end

local ok, message = pcall(main)
if not ok then
  io.stderr:write("ports: " .. tostring(message) .. "\n")
  os.exit(1)
end
