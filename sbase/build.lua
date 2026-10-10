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
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/sha256sum.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.sha256sum })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/wc.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.wc })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/tail.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.tail })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/sort.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.sort })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/grep.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.grep })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/arg.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.argument_notice })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/libutil/strtonum.c",
    ctx.stage .. "/" .. ctx.metadata.outputs.strtonum_notice })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/libutil/memmem.c",
    ctx.stage .. "/" .. ctx.metadata.outputs.memmem_notice })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/libutil/reallocarray.c",
    ctx.stage .. "/" .. ctx.metadata.outputs.reallocarray_notice })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/queue.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.queue_notice })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/UNICODE-LICENSE.txt",
    ctx.stage .. "/" .. ctx.metadata.outputs.unicode_notice })
end
