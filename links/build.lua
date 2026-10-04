return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/links.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/COPYING",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
end
