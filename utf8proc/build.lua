return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  install(ctx.build .. "/libutf8proc.a", outputs.library)
  install(ctx.source .. "/utf8proc.h", outputs.header)
  install(ctx.source .. "/LICENSE.md", outputs.license)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/utf8proc" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=static library, shipped Unicode tables\n",
    "omitted=shared library, data regeneration, upstream programs/tests\n"))
  assert(provenance:close())
  for _, name in ipairs({ "license", "provenance" }) do
    install(ctx.stage .. "/" .. outputs[name], outputs["dev_" .. name])
  end
end
