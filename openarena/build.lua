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

  -- These outputs are published beside the package, never packed into it.
  local source_dir = ctx.stage .. "/" .. metadata.outputs.source_index:match("^(.*)/")
  ctx.run({ "mkdir", "-p", source_dir })
  local sums = assert(io.open(ctx.stage .. "/" .. metadata.outputs.source_checksums, "w"))
  local verify = assert(io.open(ctx.build .. "/source-files.sha256", "w"))
  for _, file in ipairs(metadata.source_files) do
    local target = ctx.stage .. "/" .. file.output
    ctx.run({ "curl", "--fail", "--location", "--proto", "=https", "--proto-redir", "=https",
      "--output", target, "--", file.mirror })
    assert(sums:write(file.sha256, "  ", file.name, "\n"))
    assert(verify:write(file.sha256, "  ", target, "\n"))
  end
  local patch_name = "openarena-0.8.5-qagame-runtime.patch"
  local patch_digest = "c95be69796d53ee8200658f917e9ffca462749ba8d8846a0a40c3aa2935f92d6"
  install(ctx.recipe .. "/" .. patch_name, metadata.outputs.qvm_patch)
  assert(sums:write(patch_digest, "  ", patch_name, "\n"))
  assert(verify:write(patch_digest, "  ", ctx.stage, "/", metadata.outputs.qvm_patch, "\n"))
  assert(sums:close())
  assert(verify:close())
  ctx.run({ "sha256sum", "--check", "--status", ctx.build .. "/source-files.sha256" })
  install(ctx.recipe .. "/SOURCES.txt", metadata.outputs.source_index)
  install(ctx.recipe .. "/SOURCES.txt", metadata.outputs.source_notice)
  install(ctx.stage .. "/" .. metadata.outputs.source_checksums, metadata.outputs.source_inventory)
end
