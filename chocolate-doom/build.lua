return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile, "SDL2=" .. ctx.sdl2 })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/src/chocolate-doom.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/COPYING.md",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })
  -- SDL2 is linked in statically.
  ctx.run({ "install", "-D", "-m", "644", ctx.sdl2 .. "/share/licenses/sdl2/LICENSE.txt",
    ctx.stage .. "/" .. ctx.metadata.outputs.sdl2_license })
end
