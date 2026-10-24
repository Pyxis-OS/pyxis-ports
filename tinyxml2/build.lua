return function(ctx)
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "cmake", "--install", ctx.build, "--prefix", ctx.stage .. "/dev" })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local outputs = ctx.metadata.outputs
  install(ctx.source .. "/LICENSE.txt", outputs.license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/tinyxml2.pc", outputs.pc_tinyxml2)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/tinyxml2/pyxis" })
  ctx.run({ "cp", "-R", ctx.recipe .. "/.", ctx.stage .. "/dev/share/tinyxml2/pyxis/" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "archive=" .. ctx.metadata.source.archive.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=upstream static library, standard FILE APIs\n",
    "omitted=shared library, upstream tests\n",
    "limit=upstream SaveFile does not report output or fclose failures\n",
    "pyxis_source=share/tinyxml2/pyxis\n"))
  assert(provenance:close())
end
