return function(ctx)
  local metadata = ctx.metadata
  local function install(from, output)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. output })
  end

  for _, pack in ipairs(metadata.packs) do
    install(ctx.source .. "/baseoa/" .. pack.name, metadata.outputs[pack.name])
  end
  for _, name in ipairs(metadata.notices) do
    install(ctx.source .. "/" .. name, metadata.outputs[name == "COPYING" and "license" or name])
  end
  install(ctx.recipe .. "/PORT-NOTICE", metadata.outputs.notice)

  local checksums = assert(io.open(ctx.build .. "/packs.sha256", "w"))
  local provenance = assert(io.open(ctx.stage .. "/" .. metadata.outputs.provenance, "w"))
  assert(provenance:write("source=", metadata.source.archive.url,
    "\narchive_sha256=", metadata.source.archive.sha256,
    "\nversion=", metadata.version,
    "\nlicense=", metadata.license,
    "\nlocal_changes=none; baseoa PK3s and upstream notices copied unchanged\n"))
  for _, pack in ipairs(metadata.packs) do
    assert(checksums:write(pack.sha256, "  ", ctx.stage, "/", metadata.outputs[pack.name], "\n"))
    assert(provenance:write("\narchive_member=baseoa/", pack.name,
      "\nsha256=", pack.sha256, "\n"))
  end
  assert(checksums:close())
  assert(provenance:close())
  ctx.run({ "sha256sum", "--check", "--status", ctx.build .. "/packs.sha256" })
end
