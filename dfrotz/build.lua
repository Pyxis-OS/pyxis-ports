return function(ctx)
  ctx.run({ "mkdir", "-p", ctx.build .. "/upstream" })
  ctx.run({ "cp", "-a", ctx.source .. "/.", ctx.build .. "/upstream/" })
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  local root = ctx.stage .. "/bin/dfrotz.pxb/"
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, root .. to })
  end
  install(ctx.build .. "/dfrotz.pxe", "app/bin/dfrotz.pxe")
  install(ctx.recipe .. "/manifest.json", "manifest.json")
  install(ctx.source .. "/COPYING", "app/metadata/frotz/COPYING")
  install(ctx.source .. "/AUTHORS", "app/metadata/frotz/AUTHORS")
  install(ctx.source .. "/DUMB", "app/metadata/frotz/DUMB")
  install(ctx.source .. "/doc/dfrotz.6", "app/metadata/frotz/dfrotz.6")
  install(ctx.recipe .. "/PORT-NOTICE", "app/metadata/PORT-NOTICE")
  install(ctx.recipe .. "/../LICENSE", "app/metadata/pyxis-ports-LICENSE")
  ctx.run({ "tar", "--sort=name", "--mtime=@0", "--owner=0", "--group=0",
    "--numeric-owner", "--exclude=./src/test", "-cf", root .. "app/metadata/frotz-source.tar",
    "-C", ctx.source, "." })
  local provenance = assert(io.open(root .. "app/metadata/source.txt", "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=2.55\nprofile=upstream dumb frontend, Blorb, no sound\npatches=none\n"))
  assert(provenance:close())
end
