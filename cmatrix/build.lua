return function(ctx)
  local ncursesw = ctx.ncursesw
  assert(ncursesw, "cmatrix needs --ncursesw with the staged ncursesw development directory")
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "NCURSESW=" .. ncursesw,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  local root = ctx.stage .. "/bin/cmatrix.pxb/"
  local function install(source, target)
    ctx.run({ "install", "-D", "-m", "644", source, root .. target })
  end
  install(ctx.build .. "/cmatrix.pxe", "app/bin/cmatrix.pxe")
  install(ctx.recipe .. "/manifest.json", "manifest.json")
  install(ctx.source .. "/COPYING", "app/metadata/cmatrix/LICENSE")
  install(ncursesw .. "/share/licenses/ncursesw/COPYING", "app/metadata/ncursesw/COPYING")
  install(ncursesw .. "/share/ncursesw/source.txt", "app/metadata/ncursesw/source.txt")
  install(ctx.recipe .. "/../ncursesw/PORT-NOTICE", "app/metadata/ncursesw/PORT-NOTICE")
  install(ctx.recipe .. "/PORT-NOTICE", "app/metadata/PORT-NOTICE")
  install(ctx.recipe .. "/../LICENSE", "app/metadata/pyxis-ports-LICENSE")
  ctx.run({ "mkdir", "-p", root .. "app/metadata" })
  ctx.run({ "tar", "--sort=name", "--mtime=@0", "--owner=0", "--group=0",
    "--numeric-owner", "--exclude=.git", "-cf", root .. "app/metadata/cmatrix-source.tar", "-C", ctx.source, "." })
  local provenance = assert(io.open(root .. "app/metadata/source.txt", "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:close())
end
