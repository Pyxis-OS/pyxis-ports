return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/cksum.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/tee.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.tee })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/uniq.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.uniq })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/arg.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.argument_notice })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/libutil/strtonum.c",
    ctx.stage .. "/" .. ctx.metadata.outputs.strtonum_notice })
end
