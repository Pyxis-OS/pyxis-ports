return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "SDL2=" .. ctx.sdl2, "SDL2_OSMESA=" .. ctx.sdl2_osmesa,
    "MESA=" .. ctx.mesa, "CROSS_COMPILE=" .. ctx.cross_compile })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  for _, name in ipairs({ "imgui.h", "imgui_internal.h", "imconfig.h",
    "imstb_rectpack.h", "imstb_textedit.h", "imstb_truetype.h" }) do
    install(ctx.source .. "/" .. name, "dev/include/" .. name)
  end
  for _, name in ipairs({ "imgui", "imgui_draw", "imgui_tables", "imgui_widgets", "imgui_demo" }) do
    install(ctx.source .. "/" .. name .. ".cpp", "dev/share/sources/imgui/" .. name .. ".cpp")
  end
  install(ctx.build .. "/libimgui.a", ctx.metadata.outputs.core)
  install(ctx.recipe .. "/imgui.pc", ctx.metadata.outputs.pc)
  install(ctx.recipe .. "/cmake/ImGuiConfig.cmake", ctx.metadata.outputs.cmake)
  install(ctx.source .. "/LICENSE.txt", ctx.metadata.outputs.license)
  install(ctx.recipe .. "/ProggyClean.txt", ctx.metadata.outputs.font_license)
  install(ctx.recipe .. "/Japanese-ranges.txt", ctx.metadata.outputs.ranges_notice)
  install(ctx.recipe .. "/PORT-NOTICE", "dev/share/licenses/imgui/PORT-NOTICE")
  for _, name in ipairs({ "sdl2", "opengl3" }) do
    install(ctx.build .. "/libimgui-" .. name .. ".a", "sdl2-gl-dev/lib/libimgui-" .. name .. ".a")
    install(ctx.source .. "/backends/imgui_impl_" .. name .. ".h", "sdl2-gl-dev/include/backends/imgui_impl_" .. name .. ".h")
    install(ctx.source .. "/backends/imgui_impl_" .. name .. ".cpp", "sdl2-gl-dev/share/sources/imgui/backends/imgui_impl_" .. name .. ".cpp")
  end
  install(ctx.recipe .. "/imgui-sdl2-opengl3.pc", ctx.metadata.outputs.backend_pc)
  install(ctx.recipe .. "/cmake/ImGuiSDL2OpenGL3Config.cmake", ctx.metadata.outputs.backend_cmake)
  install(ctx.build .. "/imgui-demo.pxe", ctx.metadata.outputs.executable)
  install(ctx.recipe .. "/manifest.json", ctx.metadata.outputs.manifest)
  for _, prefix in ipairs({ "sdl2-gl-dev/share/licenses/imgui", "bin/imgui-demo.pxb/app/licenses/imgui" }) do
    install(ctx.source .. "/LICENSE.txt", prefix .. "/LICENSE.txt")
    install(ctx.recipe .. "/ProggyClean.txt", prefix .. "/ProggyClean.txt")
    install(ctx.recipe .. "/Japanese-ranges.txt", prefix .. "/Japanese-ranges.txt")
    install(ctx.recipe .. "/PORT-NOTICE", prefix .. "/PORT-NOTICE")
  end
  -- The program's static closure includes SDL2, Mesa and the SDK C++ runtime.
  for _, prefix in ipairs({ ctx.sdl2 .. "/share/licenses", ctx.mesa .. "/share/licenses", ctx.sdk .. "/share/licenses" }) do
    ctx.run({ "cp", "-R", prefix .. "/.", ctx.stage .. "/bin/imgui-demo.pxb/app/licenses/" })
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/imgui" })
  install(ctx.sdk .. "/share/toolchain/LICENSE.TXT", "bin/imgui-demo.pxb/app/licenses/llvm/LICENSE.TXT")
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\nsha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "profile=void* ImTextureID; math operators; no shell helper; SDL2 one-window, static OpenGL3\n"))
  assert(provenance:close())
end
