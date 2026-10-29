return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile,
    "SDL2=" .. assert(ctx.sdl2, "SDL2_mixer needs --sdl2 PATH") })
  ctx.run({ "cmake", "--install", ctx.build, "--prefix", ctx.stage .. "/dev", "--component", "devel" })
  local function install(source, target)
    ctx.run({ "install", "-D", "-m", "644", source, ctx.stage .. "/" .. target })
  end
  local outputs = ctx.metadata.outputs
  install(ctx.source .. "/LICENSE.txt", outputs.license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/SDL2_mixer.pc", outputs.pc)
  for _, name in ipairs({ "Makefile", "build.lua", "metadata.lua", "README.md", "PORT-NOTICE", "SDL2_mixer.pc" }) do
    install(ctx.recipe .. "/" .. name, "dev/share/sources/sdl2-mixer/" .. name)
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/sdl2-mixer" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "archive=" .. ctx.metadata.source.archive.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=Zlib\n",
    "profile=static WAV streaming, WAV/AIFF/VOC samples, channel mixing\n",
    "dependencies=sdl2-dev, Pyxis libc/libpyxis\n",
    "omitted=MIDI, FLAC, GME, MOD, MP3, Opus, Vorbis, WavPack, external commands, shared libraries, samples\n"))
  assert(provenance:close())
end
