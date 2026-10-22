return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "SDL2=" .. ctx.sdl2,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  install(ctx.build .. "/libraylib.a", outputs.library)
  for _, name in ipairs({ "raylib", "raymath", "rlgl" }) do
    install(ctx.source .. "/src/" .. name .. ".h", "dev/include/" .. name .. ".h")
  end
  install(ctx.recipe .. "/pyxis_config.h", outputs.configuration)
  install(ctx.recipe .. "/cmake/raylibConfig.cmake", outputs.package)
  install(ctx.source .. "/LICENSE", outputs.license)
  install(ctx.recipe .. "/THIRD-PARTY-LICENSES", outputs.third_party)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static SDL2/rlsw, shapes, textures, text, direct basic 3D, PNG, TTF, image generation, compression\n",
    "omitted=audio, GPU, shaders, SIMD requirement, model-file loaders, image export, shared objects\n"))
  assert(provenance:close())
end
