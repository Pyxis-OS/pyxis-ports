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
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/lua.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.dev_license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.dev_notice })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/config.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.dev_configuration_source })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/lua" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.dev_provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=5.5.1\nlicense=MIT\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static Lua core and selected standard libraries\n",
    "configuration=share/sources/lua/config.h\n",
    "omitted=math,debug,stock all-libraries initializer,interpreter native module/jobs\n"))
  assert(provenance:close())

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

end
