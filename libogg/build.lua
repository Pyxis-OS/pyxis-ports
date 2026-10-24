return function(ctx)
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, output)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. output })
  end
  install(ctx.build .. "/libogg.a", outputs.library_ogg)
  install(ctx.recipe .. "/ogg.pc", outputs.pc_ogg)
  install(ctx.source .. "/include/ogg/ogg.h", "dev/include/ogg/ogg.h")
  install(ctx.source .. "/include/ogg/os_types.h", "dev/include/ogg/os_types.h")
  install(ctx.recipe .. "/config_types.h", "dev/include/ogg/config_types.h")
  install(ctx.source .. "/COPYING", outputs.license)
  install(ctx.source .. "/AUTHORS", outputs.authors)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/Makefile", "dev/share/libogg/pyxis/Makefile")
  install(ctx.recipe .. "/build.lua", "dev/share/libogg/pyxis/build.lua")
  install(ctx.recipe .. "/metadata.lua", "dev/share/libogg/pyxis/metadata.lua")
  install(ctx.recipe .. "/README.md", "dev/share/libogg/pyxis/README.md")
  install(ctx.recipe .. "/PORT-NOTICE", "dev/share/libogg/pyxis/PORT-NOTICE")
  install(ctx.recipe .. "/ogg.pc", "dev/share/libogg/pyxis/ogg.pc")
  install(ctx.recipe .. "/config_types.h", "dev/share/libogg/pyxis/config_types.h")
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/libogg" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=BSD-3-Clause\npatches=none\n",
    "profile=static Ogg framing and bit packing, native fixed-width types\n",
    "omitted=shared libraries, programs and tests\n"))
  assert(provenance:close())
end
