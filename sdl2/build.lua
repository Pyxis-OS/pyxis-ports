return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libSDL2.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.library })
  ctx.run({ "mkdir", "-p", "--", ctx.stage .. "/dev/include" })
  ctx.run({ "cp", "-R", "--", ctx.build .. "/include/SDL2", ctx.stage .. "/dev/include/" })
  for _, name in ipairs({ "SDL2Config.cmake", "SDL2ConfigVersion.cmake" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/cmake/" .. name,
      ctx.stage .. "/dev/lib/cmake/SDL2/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE.txt",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })

  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/sdl2" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static library, Pyxis video/input/timer/paths, software renderer\n",
    "omitted=threads, audio devices, haptics, sensors, HIDAPI, shared objects, OpenGL/Vulkan, SDL_main, SDL_test\n"))
  assert(provenance:close())
end
