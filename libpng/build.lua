return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "ZLIB=" .. ctx.zlib,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libpng.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.library })
  for _, name in ipairs({ "png.h", "pngconf.h" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/" .. name,
      ctx.stage .. "/dev/include/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/pnglibconf.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.configuration })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })

  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/libpng" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\npatches=none\n",
    "profile=static conventional read/write, stdio, setjmp error recovery\n",
    "configuration=upstream pnglibconf.mak with pyxis.dfa, target preprocessor\n",
    "omitted=simplified API, architecture acceleration, shared libraries, upstream programs/tests\n"))
  assert(provenance:close())
  for _, name in ipairs({ "license", "notice", "provenance" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.stage .. "/" .. ctx.metadata.outputs[name],
      ctx.stage .. "/" .. ctx.metadata.outputs["dev_" .. name] })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/libpng.pc",
    ctx.stage .. "/dev/lib/pkgconfig/libpng.pc" })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/pyxis.dfa",
    ctx.stage .. "/" .. ctx.metadata.outputs.dev_configuration_source })

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

end
