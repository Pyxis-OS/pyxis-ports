return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libfmt.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.library })
  ctx.run({ "mkdir", "-p", "--", ctx.stage .. "/dev/include" })
  ctx.run({ "cp", "-R", "--", ctx.source .. "/include/fmt", ctx.stage .. "/dev/include/" })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })

  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/fmt" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static formatting library, FMT_USE_LOCALE 0\n",
    "omitted=os.h file/process helpers (FMT_OS), modules, shared library, tests\n"))
  assert(provenance:close())
end
