return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local outputs = ctx.metadata.outputs
  install(ctx.build .. "/pkgconf.pxe", outputs.executable)
  install(ctx.build .. "/pkgconf.pxe", outputs.alias)
  install(ctx.source .. "/COPYING", outputs.license)
  install(ctx.source .. "/AUTHORS", outputs.authors)
  install(ctx.source .. "/cli/getopt_long.c", outputs.getopt_source)
  install(ctx.source .. "/cli/getopt_long.h", outputs.getopt_header)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/pkgconf" })
  local file = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(file:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(file:write("patch=" .. patch .. "\n"))
  end
  assert(file:write("profile=static pkgconf CLI, native scheme paths, static output by default\n",
    "omitted=installed libpkgconf API, bomtool, spdxtool, pccritic, manuals, tests\n"))
  assert(file:close())
end
