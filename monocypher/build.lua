return function(ctx)
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, output)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. output })
  end
  install(ctx.build .. "/libmonocypher.a", outputs.library)
  install(ctx.source .. "/src/monocypher.h", outputs.header)
  install(ctx.source .. "/src/optional/monocypher-ed25519.h", outputs.ed25519_header)
  install(ctx.recipe .. "/monocypher.pc", outputs.pc)
  for _, entry in ipairs({
    { "LICENCE.md", "license" }, { "AUTHORS.md", "authors" },
    { "src/monocypher.c", "core_notice" },
    { "src/monocypher.h", "core_header_notice" },
    { "src/optional/monocypher-ed25519.c", "ed25519_notice" },
    { "src/optional/monocypher-ed25519.h", "ed25519_header_notice" },
  }) do
    install(ctx.source .. "/" .. entry[1], outputs[entry[2]])
  end
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  for _, name in ipairs({ "Makefile", "build.lua", "metadata.lua",
    "monocypher.pc", "README.md", "PORT-NOTICE" }) do
    install(ctx.recipe .. "/" .. name, "dev/share/monocypher/pyxis/" .. name)
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/monocypher" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\npatches=none\n",
    "profile=static core and optional Ed25519/SHA-512, native C ABI\n",
    "omitted=shared libraries, docs, upstream tests and external vector generators\n"))
  assert(provenance:close())
end
