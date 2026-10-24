return function(ctx)
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local outputs = ctx.metadata.outputs
  install(ctx.source .. "/stb_image.h", outputs.header)
  install(ctx.source .. "/LICENSE", outputs.license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/stb.pc", outputs.pc_stb)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/stb/pyxis" })
  ctx.run({ "cp", "-R", ctx.recipe .. "/.", ctx.stage .. "/dev/share/stb/pyxis/" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "archive=" .. ctx.metadata.source.archive.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=unchanged stb_image header only\n",
    "header_sha256=c54b15a689e6a1f32c75e2ec23afa442e3e0e37e894b73c1974d08679b20dd5c\n",
    "implementation=consumer-owned STB_IMAGE_IMPLEMENTATION, exactly one\n",
    "configuration=caller-selected; stdio, image formats, HDR and TLS defaults retained\n",
    "omitted=other stb libraries, upstream tests\n",
    "pyxis_source=share/stb/pyxis\n"))
  assert(provenance:close())
end
