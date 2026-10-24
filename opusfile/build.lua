return function(ctx)
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile, "OGG=" .. assert(ctx.ogg, "opusfile needs --ogg PATH"),
    "OPUS=" .. assert(ctx.opus, "opusfile needs --opus PATH") })
  local outputs = ctx.metadata.outputs
  local function install(from, output)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. output })
  end
  install(ctx.build .. "/libopusfile.a", outputs.library_opusfile)
  install(ctx.recipe .. "/opusfile.pc", outputs.pc_opusfile)
  install(ctx.recipe .. "/patches/0001-pyxis-unavailable-freopen.patch", outputs.patch)
  install(ctx.source .. "/include/opusfile.h", "dev/include/opus/opusfile.h")
  install(ctx.source .. "/COPYING", outputs.license)
  install(ctx.source .. "/AUTHORS", outputs.authors)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/Makefile", "dev/share/opusfile/pyxis/Makefile")
  install(ctx.recipe .. "/build.lua", "dev/share/opusfile/pyxis/build.lua")
  install(ctx.recipe .. "/metadata.lua", "dev/share/opusfile/pyxis/metadata.lua")
  install(ctx.recipe .. "/README.md", "dev/share/opusfile/pyxis/README.md")
  install(ctx.recipe .. "/PORT-NOTICE", "dev/share/opusfile/pyxis/PORT-NOTICE")
  install(ctx.recipe .. "/opusfile.pc", "dev/share/opusfile/pyxis/opusfile.pc")
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/opusfile" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=BSD-3-Clause\n",
    "patch=patches/0001-pyxis-unavailable-freopen.patch\n",
    "profile=static FILE/memory/callback Ogg Opus decoder, OP_HAVE_LRINTF\n",
    "omitted=op_freopen (compile-time unavailable), opusurl, HTTP/TLS, shared libraries, examples and tests\n"))
  assert(provenance:close())
end
