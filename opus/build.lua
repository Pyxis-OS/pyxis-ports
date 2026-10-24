return function(ctx)
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, output)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. output })
  end
  install(ctx.build .. "/libopus.a", outputs.library_opus)
  install(ctx.recipe .. "/opus.pc", outputs.pc_opus)
  install(ctx.source .. "/include/opus.h", "dev/include/opus/opus.h")
  install(ctx.source .. "/include/opus_types.h", "dev/include/opus/opus_types.h")
  install(ctx.source .. "/include/opus_defines.h", "dev/include/opus/opus_defines.h")
  install(ctx.source .. "/include/opus_multistream.h", "dev/include/opus/opus_multistream.h")
  install(ctx.source .. "/include/opus_projection.h", "dev/include/opus/opus_projection.h")
  install(ctx.source .. "/COPYING", outputs.license)
  install(ctx.source .. "/AUTHORS", outputs.authors)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/Makefile", "dev/share/opus/pyxis/Makefile")
  install(ctx.recipe .. "/build.lua", "dev/share/opus/pyxis/build.lua")
  install(ctx.recipe .. "/metadata.lua", "dev/share/opus/pyxis/metadata.lua")
  install(ctx.recipe .. "/README.md", "dev/share/opus/pyxis/README.md")
  install(ctx.recipe .. "/PORT-NOTICE", "dev/share/opus/pyxis/PORT-NOTICE")
  install(ctx.recipe .. "/opus.pc", "dev/share/opus/pyxis/opus.pc")
  install(ctx.recipe .. "/notices.py", "dev/share/opus/pyxis/notices.py")
  install(ctx.build .. "/config.h", outputs.config)
  ctx.run({ "python3", ctx.recipe .. "/notices.py", ctx.source, ctx.build,
    ctx.stage .. "/" .. outputs.source_notices })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/opus" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=BSD-3-Clause\npatches=none\n",
    "profile=static floating-point Opus encoder/decoder, baseline x86-64/SSE2\n",
    "omitted=shared libraries, programs/tests, SSE4.1/AVX2, DRED/OSCE, native CPU flags\n"))
  assert(provenance:close())
end
