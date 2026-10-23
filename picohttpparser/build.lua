return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libpicohttpparser.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.library })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/picohttpparser.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.header })
  -- Upstream carries the complete MIT notice in its public header.
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/picohttpparser.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/picohttpparser.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.dev_license })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/picohttpparser" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.dev_provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "license=MIT\npatches=none\nprofile=static HTTP parser library\n"))
  assert(provenance:close())
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/picohttpparser.pc",
    ctx.stage .. "/dev/lib/pkgconfig/picohttpparser.pc" })

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

end
