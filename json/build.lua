return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile", "SDK=" .. ctx.sdk,
    "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "cmake", "--install", ctx.build, "--prefix", ctx.stage .. "/dev" })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/dev/" .. to })
  end
  install(ctx.source .. "/LICENSE.MIT", "share/licenses/json/LICENSE.MIT")
  install(ctx.recipe .. "/nlohmann_json.pc", "lib/pkgconfig/nlohmann_json.pc")
  install(ctx.recipe .. "/../LICENSE", "share/licenses/json/MPL-2.0")
  install(ctx.recipe .. "/../LICENSING.md", "share/licenses/json/LICENSING.md")
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/sources/json" })
  for _, file in ipairs({ "metadata.lua", "build.lua", "Makefile", "nlohmann_json.pc", "README.md" }) do
    install(ctx.recipe .. "/" .. file, "share/sources/json/" .. file)
  end
  for _, patch in ipairs(ctx.metadata.patches) do
    install(ctx.recipe .. "/" .. patch, "share/sources/json/" .. patch)
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/json" })
  local provenance = assert(io.open(ctx.stage .. "/dev/share/json/source.txt", "w"))
  assert(provenance:write("source=", ctx.metadata.source.archive.url,
    "\nsha256=", ctx.metadata.source.archive.sha256,
    "\nprofile=header-only; upstream filesystem conversions\n"))
  assert(provenance:close())
end
