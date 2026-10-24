return function(ctx)
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile, "OGG=" .. assert(ctx.ogg, "libvorbis needs --ogg PATH") })
  local outputs = ctx.metadata.outputs
  local function install(from, output)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. output })
  end
  install(ctx.build .. "/libvorbis.a", outputs.library_vorbis)
  install(ctx.recipe .. "/vorbis.pc", outputs.pc_vorbis)
  install(ctx.build .. "/libvorbisenc.a", outputs.library_vorbisenc)
  install(ctx.recipe .. "/vorbisenc.pc", outputs.pc_vorbisenc)
  install(ctx.build .. "/libvorbisfile.a", outputs.library_vorbisfile)
  install(ctx.recipe .. "/vorbisfile.pc", outputs.pc_vorbisfile)
  install(ctx.source .. "/include/vorbis/codec.h", "dev/include/vorbis/codec.h")
  install(ctx.source .. "/include/vorbis/vorbisenc.h", "dev/include/vorbis/vorbisenc.h")
  install(ctx.source .. "/include/vorbis/vorbisfile.h", "dev/include/vorbis/vorbisfile.h")
  install(ctx.source .. "/COPYING", outputs.license)
  install(ctx.source .. "/AUTHORS", outputs.authors)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/Makefile", "dev/share/libvorbis/pyxis/Makefile")
  install(ctx.recipe .. "/build.lua", "dev/share/libvorbis/pyxis/build.lua")
  install(ctx.recipe .. "/metadata.lua", "dev/share/libvorbis/pyxis/metadata.lua")
  install(ctx.recipe .. "/README.md", "dev/share/libvorbis/pyxis/README.md")
  install(ctx.recipe .. "/PORT-NOTICE", "dev/share/libvorbis/pyxis/PORT-NOTICE")
  install(ctx.recipe .. "/vorbis.pc", "dev/share/libvorbis/pyxis/vorbis.pc")
  install(ctx.recipe .. "/vorbisenc.pc", "dev/share/libvorbis/pyxis/vorbisenc.pc")
  install(ctx.recipe .. "/vorbisfile.pc", "dev/share/libvorbis/pyxis/vorbisfile.pc")
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/libvorbis" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=BSD-3-Clause\npatches=none\n",
    "profile=static Vorbis codec, encoder and FILE/callback decoder\n",
    "omitted=shared libraries, examples and tests\n"))
  assert(provenance:close())
end
