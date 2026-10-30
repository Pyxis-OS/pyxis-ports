return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "MESA=" .. ctx.mesa,
    "CROSS_COMPILE=" .. ctx.cross_compile, "all", "osmesa" })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libSDL3.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.library })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libSDL3-osmesa.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.gl_library })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/sdl3-osmesa.pc",
    ctx.stage .. "/" .. ctx.metadata.outputs.gl_pc })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/cmake/SDL3OSMesaConfig.cmake",
    ctx.stage .. "/" .. ctx.metadata.outputs.gl_package })
  -- SDL3's public headers need no build configuration; SDL_build_config.h
  -- is internal to the library.
  ctx.run({ "mkdir", "-p", "--", ctx.stage .. "/dev/include" })
  ctx.run({ "cp", "-R", "--", ctx.source .. "/include/SDL3", ctx.stage .. "/dev/include/" })
  for _, name in ipairs({ "SDL3Config.cmake", "SDL3ConfigVersion.cmake" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/cmake/" .. name,
      ctx.stage .. "/dev/lib/cmake/SDL3/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/sdl3.pc",
    ctx.stage .. "/" .. ctx.metadata.outputs.pc_sdl3 })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE.txt",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/src/video/yuv2rgb/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.yuv_license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })
  local libm_source = assert(io.open(ctx.source .. "/src/libm/s_sin.c", "r"))
  local libm_text = assert(libm_source:read("a"))
  assert(libm_source:close())
  local libm_notice = assert(libm_text:match("(/%*.-%*/)"))
  local notice_file = ctx.build .. "/fdlibm-NOTICE"
  local notice = assert(io.open(notice_file, "w"))
  assert(notice:write(libm_notice, "\n"))
  assert(notice:close())
  ctx.run({ "install", "-D", "-m", "644", notice_file,
    ctx.stage .. "/" .. ctx.metadata.outputs.libm_notice })
  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

  local function write_provenance(output, profile, omitted, dependencies)
    local path = ctx.stage .. "/" .. ctx.metadata.outputs[output]
    ctx.run({ "mkdir", "-p", assert(path:match("^(.*)/")) })
    local provenance = assert(io.open(path, "w"))
    assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
      "commit=" .. ctx.metadata.source.commit .. "\n",
      "version=" .. ctx.metadata.version .. "\n",
      "license=" .. ctx.metadata.license .. "\n"))
    for _, patch in ipairs(ctx.metadata.patches) do
      assert(provenance:write("patch=" .. patch .. "\n"))
    end
    assert(provenance:write("profile=" .. profile .. "\n", "omitted=" .. omitted .. "\n"))
    if dependencies then assert(provenance:write("dependencies=" .. dependencies .. "\n")) end
    assert(provenance:close())
  end
  local common_profile = "static library, Pyxis video/input/threads/paths, software renderer"
  local common_omitted = "thread priorities, audio, clipboard, joysticks, haptics, sensors, HIDAPI, camera, GPU, shared objects, Vulkan, SDL_test"
  write_provenance("provenance", common_profile, common_omitted .. ", OpenGL backend")
  write_provenance("gl_provenance", common_profile .. ", PYXIS_USE_OSMESA, static Mesa OpenGL",
    common_omitted, "sdl3-dev (shared headers and MPL backend source), mesa-dev, libcxx-dev")

  -- The MPL-covered backend and build configuration, as sdl2-dev retains its own.
  for _, name in ipairs({
    "Makefile",
    "SDL_build_config.h",
    "pyxis/SDL_pyxisevents.c",
    "pyxis/SDL_pyxismouse.c",
    "pyxis/SDL_pyxisopengl.c",
    "pyxis/SDL_pyxisvideo.c",
    "pyxis/SDL_pyxisvideo.h",
    "pyxis/SDL_sysfilesystem.c",
    "pyxis/SDL_systhread.c",
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/" .. name,
      ctx.stage .. "/dev/share/sources/sdl3/" .. name })
  end

  -- sdl3-osmesa-dev depends on sdl3-dev for the shared public headers and
  -- covered backend source. Keep profile-specific notices beside its archive.
  for _, entry in ipairs({
    { "license", "gl_license" },
    { "yuv_license", "gl_yuv_license" },
    { "libm_notice", "gl_libm_notice" },
    { "notice", "gl_notice" },
    { "dev_pyxis_license", "gl_pyxis_license" },
    { "dev_licensing", "gl_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.stage .. "/" .. ctx.metadata.outputs[entry[1]],
      ctx.stage .. "/" .. ctx.metadata.outputs[entry[2]] })
  end
end
