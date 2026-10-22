return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "RAYLIB=" .. ctx.raylib, "SDL2=" .. ctx.sdl2,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  install(ctx.build .. "/minesweeper.pxe", outputs.executable)
  install(ctx.source .. "/resources/flag.png", outputs.texture)
  install(ctx.recipe .. "/manifest.json", outputs.manifest)
  install(ctx.source .. "/LICENSE", outputs.license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/minesweeper" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "license=MIT\npatch=patches/0001-pyxis-runtime.patch\n"))
  assert(provenance:close())
end
