return function(ctx)
  -- Stage the text database unchanged; it keeps upstream's own license header.
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/pci.ids",
    ctx.stage .. "/" .. ctx.metadata.outputs.database })
  for _, name in ipairs({ "LICENSE", "NOTICE" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/" .. name,
      ctx.stage .. "/share/licenses/pciids/" .. name })
  end

  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/pciids" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=BSD-3-Clause, elected from upstream's GPL-2.0-or-later or BSD-3-Clause\n",
    "patches=none\nlocal_changes=none; staged pci.ids is byte-for-byte upstream\n"))
  assert(provenance:close())
end
