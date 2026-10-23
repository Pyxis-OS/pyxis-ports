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
  for _, name in ipairs({ "yuv_license", "runtime_yuv_license" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/src/video/yuv2rgb/LICENSE",
      ctx.stage .. "/" .. ctx.metadata.outputs[name] })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })
  local libm_source = assert(io.open(ctx.source .. "/src/libm/s_sin.c", "r"))
  local libm_text = assert(libm_source:read("a"))
  assert(libm_source:close())
  local libm_notice = assert(libm_text:match("^(/%*.-%*/)"))
  local notice_file = ctx.build .. "/fdlibm-NOTICE"
  local notice = assert(io.open(notice_file, "w"))
  assert(notice:write(libm_notice, "\n"))
  assert(notice:close())
  for _, name in ipairs({ "libm_notice", "runtime_libm_notice" }) do
    ctx.run({ "install", "-D", "-m", "644", notice_file,
      ctx.stage .. "/" .. ctx.metadata.outputs[name] })
  end

  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/sdl2" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static library, Pyxis video/input/audio/timer/threads/paths, software renderer\n",
    "omitted=thread priorities and detach, audio capture, haptics, sensors, HIDAPI, shared objects, OpenGL/Vulkan, SDL_main, SDL_test\n"))
  assert(provenance:close())
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/sdl2.pc",
    ctx.stage .. "/dev/lib/pkgconfig/sdl2.pc" })

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

  for _, name in ipairs({
    "Makefile",
    "SDL_config.h",
    "pyxis/SDL_pyxisaudio.c",
    "pyxis/SDL_pyxisclipboard.c",
    "pyxis/SDL_pyxisevents.c",
    "pyxis/SDL_pyxismouse.c",
    "pyxis/SDL_pyxisvideo.c",
    "pyxis/SDL_pyxisvideo.h",
    "pyxis/SDL_syscond.c",
    "pyxis/SDL_sysfilesystem.c",
    "pyxis/SDL_systhread.c",
    "pyxis/SDL_systimer.c",
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/" .. name,
      ctx.stage .. "/dev/share/sources/sdl2/" .. name })
  end

end
