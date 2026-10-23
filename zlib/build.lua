return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libz.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.library })
  for _, name in ipairs({ "zlib.h", "zconf.h" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/" .. name,
      ctx.stage .. "/dev/include/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })

  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/zlib" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=static core, compress/uncompress, default allocators, static tables\n",
    "omitted=gz file I/O, contrib, shared libraries, upstream programs/tests\n"))
  assert(provenance:close())
  for _, name in ipairs({ "license", "notice", "provenance" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.stage .. "/" .. ctx.metadata.outputs[name],
      ctx.stage .. "/" .. ctx.metadata.outputs["dev_" .. name] })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/zlib.pc",
    ctx.stage .. "/dev/lib/pkgconfig/zlib.pc" })

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

end
