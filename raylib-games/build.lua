return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "RAYLIB=" .. ctx.raylib, "SDL2=" .. ctx.sdl2,
    "GAMES=" .. table.concat(ctx.metadata.games, " "),
    "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  for _, game in ipairs(ctx.metadata.games) do
    install(ctx.build .. "/" .. game .. ".pxe", outputs[game])
    install(ctx.recipe .. "/manifests/" .. game .. ".json", outputs[game .. "_manifest"])
  end
  install(ctx.source .. "/LICENSE", outputs.license)
  install(ctx.source .. "/classics/LICENSE", outputs.classics_license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/raylib-games" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "license=Zlib\npatches=none\n",
    "games=" .. table.concat(ctx.metadata.games, " ") .. "\n"))
  assert(provenance:close())
end
