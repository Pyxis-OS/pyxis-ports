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
  install(ctx.recipe .. "/pyxis_miniaudio.h", outputs.audio_backend)
  install(ctx.recipe .. "/cmake/raylibConfig.cmake", outputs.package)
  install(ctx.source .. "/LICENSE", outputs.license)
  install(ctx.recipe .. "/THIRD-PARTY-LICENSES", outputs.third_party)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.build .. "/raylib-shapes.pxe", outputs.example)
  install(ctx.recipe .. "/examples/manifest.json", outputs.manifest)
  install(ctx.recipe .. "/examples/shapes.c", outputs.example_source)
  install(ctx.recipe .. "/examples/manifest.json", outputs.example_manifest)
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static SDL2/rlsw, shapes, textures, text, direct basic 3D, PNG, TTF, image generation, compression, SDL audio/WAV\n",
    "omitted=capture, non-WAV audio decoders, GPU, shaders, SIMD requirement, model-file loaders, image export, shared objects\n"))
  assert(provenance:close())
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/raylib.pc",
    ctx.stage .. "/dev/lib/pkgconfig/raylib.pc" })

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

end
