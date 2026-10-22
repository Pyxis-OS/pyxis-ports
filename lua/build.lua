return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile,
    "MBEDTLS=" .. ctx.mbedtls })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/lua.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/manifest.json",
    ctx.stage .. "/" .. ctx.metadata.outputs.manifest })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/commands.lua",
    ctx.stage .. "/" .. ctx.metadata.outputs.commands })
  -- The pinned mirror carries its complete MIT notice in lua.h.
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/lua.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/liblua.a",
    ctx.stage .. "/dev/lib/liblua.a" })
  for _, header in ipairs({ "lua.h", "luaconf.h", "lauxlib.h", "lualib.h" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/" .. header,
      ctx.stage .. "/dev/include/" .. header })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/lua.pc",
    ctx.stage .. "/dev/lib/pkgconfig/lua.pc" })

end
