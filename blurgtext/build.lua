return function(ctx)
  for _, extra in ipairs(ctx.metadata.source.extra) do
    if extra.destination then
      local destination = ctx.source .. "/" .. extra.destination
      ctx.run({ "mkdir", "-p", destination })
      ctx.run({ "cp", "-R", ctx.extra[extra.name] .. "/.", destination })
    end
  end

  local native = ctx.build .. "/native"
  local fthb_args = "-DFREETYPE_URL=" .. ctx.extra.freetype .. ".tar;-DHARFBUZZ_URL=" .. ctx.extra.harfbuzz .. ".tar"
  ctx.run({ "cmake", "-S", ctx.source, "-B", native,
    "-DCMAKE_TOOLCHAIN_FILE=" .. ctx.sdk .. "/share/pyxis.cmake",
    "-DCMAKE_BUILD_TYPE=Release", "-DBT_BUILD_FTHB=ON",
    "-DBT_BUILD_SHARED=OFF", "-DBT_ENABLE_SYSTEMFONTS=ON", "-DBT_BUILD_DEMO=ON",
    "-DPLUTOVG_DISABLE_FONT_FACE_CACHE_LOAD=ON",
    "-DBT_FTHB_CMAKE_ARGS=" .. fthb_args,
    "-DCMAKE_FIND_ROOT_PATH=" .. ctx.sdl2 .. ";" .. ctx.sdl2_osmesa .. ";" .. ctx.mesa })
  ctx.run({ "cmake", "--build", native, "-j16" })

  local out = ctx.metadata.outputs
  local app = ctx.stage .. "/bin/blurgtext.pxb/app"
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local function copy_tree(from, to)
    ctx.run({ "mkdir", "-p", to })
    ctx.run({ "cp", "-R", "--", from .. "/.", to })
  end
  install(native .. "/demo/blurgtest.pxe", out.executable)
  install(ctx.recipe .. "/manifest.json", out.manifest)
  install(ctx.source .. "/include/blurgtext.h", out.header)
  install(ctx.recipe .. "/blurgtext.pc", out.pc)
  install(ctx.source .. "/LICENSE.txt", out.license)
  install(ctx.recipe .. "/PORT-NOTICE", out.notice)
  install(ctx.recipe .. "/../LICENSE", "bin/blurgtext.pxb/app/share/licenses/pyxis-ports/LICENSE")
  install(ctx.recipe .. "/../LICENSING.md", "bin/blurgtext.pxb/app/share/licenses/pyxis-ports/LICENSING.md")

  for name, source in pairs({
    blurgtext = "/libblurgtext.a", raqm = "/libraqm.a", sheenbidi = "/libsheenbidi.a",
    unibreak = "/libunibreak.a", plutosvg = "/deps/plutosvg/libplutosvg.a",
    plutovg = "/deps/plutosvg/plutovg/libplutovg.a",
    freetype = "/FTHB-prefix/lib/libfreetype.a", harfbuzz = "/FTHB-prefix/lib/libharfbuzz.a",
  }) do
    install(native .. source, out[name .. "_library"])
  end

  ctx.run({ "python3", ctx.recipe .. "/notices.py", ctx.source, native, ctx.recipe,
    app .. "/share/licenses/blurgtext" })
  copy_tree(ctx.sdl2 .. "/share/licenses/sdl2", app .. "/share/licenses/sdl2")
  copy_tree(ctx.sdl2_osmesa .. "/share/licenses/sdl2-osmesa", app .. "/share/licenses/sdl2-osmesa")
  copy_tree(ctx.sdl2 .. "/share/sources/sdl2", app .. "/share/sources/sdl2")
  copy_tree(ctx.mesa .. "/share/licenses/mesa", app .. "/share/licenses/mesa")
  copy_tree(ctx.sdk .. "/share/licenses", app .. "/share/licenses/sdk")
  copy_tree(ctx.sdk .. "/share/toolchain", app .. "/share/licenses/sdk/toolchain")
  copy_tree(ctx.recipe .. "/patches", app .. "/share/blurgtext/patches")
  -- Include the modified source files covered by the Pyxis ports licence.
  for _, path in ipairs({ "src/sysfonts_pyxis.c", "src/fontmanager.c", "src/blurgtext_internal.h",
      "CMakeLists.txt", "include/blurgtext.h", "deps/ft-hb-bundle/CMakeLists.txt", "demo/CMakeLists.txt",
      "demo/main.c", "demo/glad/src/glad.c" }) do
    install(ctx.source .. "/" .. path, "bin/blurgtext.pxb/app/share/sources/blurgtext/" .. path)
  end

  ctx.run({ "mkdir", "-p", app .. "/share/blurgtext" })
  local f = assert(io.open(ctx.stage .. "/" .. out.provenance, "w"))
  assert(f:write("commit=", ctx.metadata.source.commit, "\n",
    "source=", ctx.metadata.source.archive.url, "\n",
    "sha256=", ctx.metadata.source.archive.sha256, "\n",
    "profile=static bundled FreeType 2.15.0 and HarfBuzz 15.0.0; SDL2/OSMesa softpipe\n",
    "fonts=pkg://fonts only; unchanged upstream demo content and layout\n",
    "omitted=dynamic loading, fontconfig, plutovg filesystem font cache, host font discovery\n"))
  for _, extra in ipairs(ctx.metadata.source.extra) do
    assert(f:write("\ndependency=", extra.name, "\nsource=", extra.archive.url,
      "\nsha256=", extra.archive.sha256, "\n"))
    if extra.commit then assert(f:write("commit=", extra.commit, "\n")) end
  end
  for _, patch in ipairs(ctx.metadata.patches) do assert(f:write("patch=", patch, "\n")) end
  assert(f:close())
end
