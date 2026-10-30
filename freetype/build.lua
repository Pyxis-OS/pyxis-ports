return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "cmake", "--install", ctx.build, "--prefix", ctx.stage .. "/dev" })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local outputs = ctx.metadata.outputs
  -- Upstream's generated freetype2.pc names the build machine's prefix.
  install(ctx.recipe .. "/freetype2.pc", outputs.pc_freetype2)
  install(ctx.source .. "/LICENSE.TXT", outputs.license)
  install(ctx.source .. "/docs/FTL.TXT", outputs.ftl)
  install(ctx.source .. "/docs/GPLv2.TXT", outputs.gpl)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/freetype/pyxis" })
  ctx.run({ "cp", "-R", ctx.recipe .. "/.", ctx.stage .. "/dev/share/freetype/pyxis/" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=upstream static library, default modules, internal zlib\n",
    "omitted=shared library, HarfBuzz, PNG, bzip2, Brotli, system zlib\n",
    "pyxis_source=share/freetype/pyxis\n"))
  assert(provenance:close())
end
