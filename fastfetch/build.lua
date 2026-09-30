return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/fastfetch.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  -- The bundled yyjson header carries its complete MIT notice.
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/src/3rdparty/yyjson/yyjson.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.yyjson_license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.pyxis_license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })
end
