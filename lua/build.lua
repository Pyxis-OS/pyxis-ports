return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/lua.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  -- The pinned mirror carries its complete MIT notice in lua.h.
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/lua.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
end
