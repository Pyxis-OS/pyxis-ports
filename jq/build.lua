return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/jq.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/COPYING",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })

  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/jq" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=jq with bundled decNumber, without Oniguruma\n",
    "omitted=regular-expression builtins, docs, manual, tests, shared library\n"))
  assert(provenance:close())
end
