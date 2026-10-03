return function(ctx)
  -- Stage the text database unchanged, preserving its maintainer header.
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/usb.ids",
    ctx.stage .. "/" .. ctx.metadata.outputs.database })
  for _, name in ipairs({ "LICENSE", "NOTICE" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/" .. name,
      ctx.stage .. "/share/licenses/usbids/" .. name })
  end

  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/usbids" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=BSD-3-Clause, elected from the project site's GPL-2.0-or-later or BSD-3-Clause database grant\n",
    "license_source=https://usb-ids.gowdy.us/\n",
    "mirror_license=GPL-3.0 text; see NOTICE for the separate database grant\n",
    "patches=none\nlocal_changes=none; staged usb.ids is byte-for-byte upstream\n"))
  assert(provenance:close())
end
