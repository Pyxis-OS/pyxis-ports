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
end
