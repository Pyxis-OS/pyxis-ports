return function(ctx)
  assert(ctx.prefixes and #ctx.prefixes > 0, "libultraship requires --dependencies PREFIX;PREFIX;...")
  local prefixes = table.concat(ctx.prefixes, ";")
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile", "SDK=" .. ctx.sdk,
    "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build, "RECIPE=" .. ctx.recipe,
    "DEPS=" .. prefixes, "CROSS_COMPILE=" .. ctx.cross_compile })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/include", ctx.stage .. "/dev/share/sources" })
  ctx.run({ "cp", "-R", ctx.source .. "/include/.", ctx.stage .. "/dev/include/" })
  install(ctx.recipe .. "/native/include/ship/NativePaths.h", "dev/include/ship/NativePaths.h")
  install(ctx.build .. "/pyxis_profile.h", ctx.metadata.outputs.profile)
  install(ctx.build .. "/upstream/libultraship.a", ctx.metadata.outputs.library)
  install(ctx.recipe .. "/libultraship.pc", ctx.metadata.outputs.pc)
  install(ctx.recipe .. "/cmake/LibUltraShipConfig.cmake", ctx.metadata.outputs.cmake)
  install(ctx.source .. "/LICENSE", ctx.metadata.outputs.license)
  install(ctx.source .. "/src/fast/LICENSE.txt", "dev/share/licenses/libultraship/Fast3D-MIT.txt")
  install(ctx.recipe .. "/FontAwesome-OFL-1.1.txt", ctx.metadata.outputs.font_license)
  install(ctx.recipe .. "/PORT-NOTICE", "dev/share/licenses/libultraship/PORT-NOTICE")
  install(ctx.recipe .. "/../LICENSING.md", "dev/share/licenses/libultraship/LICENSING.md")
  install(ctx.recipe .. "/../LICENSE", "dev/share/licenses/libultraship/MPL-2.0")
  ctx.run({ "cp", "-R", ctx.source, ctx.stage .. "/dev/share/sources/libultraship" })
  ctx.run({ "rm", "-rf", ctx.stage .. "/dev/share/sources/libultraship/.git" })
  ctx.run({ "cp", "-R", ctx.recipe, ctx.stage .. "/dev/share/sources/pyxis-libultraship" })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/libultraship/shaders" })
  ctx.run({ "cp", "-R", ctx.source .. "/src/fast/shaders/opengl", ctx.stage .. "/dev/share/libultraship/shaders/" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.source, "w"))
  assert(provenance:write("commit=" .. ctx.metadata.source.commit .. "\nsha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "profile=Pyxis; F3DEX_GBI_2; controller buttons uint32; priority/pause workers; read-only ZIP/O2R; static GL3.3compat\n"))
  assert(provenance:close())
  install(ctx.build .. "/lus-demo.pxe", ctx.metadata.outputs.executable)
  install(ctx.recipe .. "/manifest.json", ctx.metadata.outputs.manifest)
  ctx.run({ "python3", ctx.recipe .. "/manual/fixture.py", ctx.source,
    ctx.stage .. "/" .. ctx.metadata.outputs.fixture })
  install(ctx.recipe .. "/manual/readonly.json", "bin/lus-demo.pxb/app/readonly.json")
  ctx.run({ "mkdir", "-p", ctx.stage .. "/bin/lus-demo.pxb/app/licenses" })
  ctx.run({ "cp", "-R", ctx.stage .. "/dev/share/licenses/.", ctx.stage .. "/bin/lus-demo.pxb/app/licenses/" })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/bin/lus-demo.pxb/app/sources" })
  for _, prefix in ipairs(ctx.prefixes) do
    ctx.run({ "cp", "-R", prefix .. "/share/licenses/.", ctx.stage .. "/bin/lus-demo.pxb/app/licenses/" })
    ctx.run({ "sh", "-c", 'if [ -d "$1/share/sources" ]; then cp -R "$1/share/sources/." "$2"; fi',
      "sources", prefix, ctx.stage .. "/bin/lus-demo.pxb/app/sources/" })
  end
  ctx.run({ "cp", "-R", ctx.sdk .. "/share/licenses/.", ctx.stage .. "/bin/lus-demo.pxb/app/licenses/" })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/bin/lus-demo.pxb/app/sources" })
  ctx.run({ "cp", "-R", ctx.stage .. "/dev/share/sources/libultraship", ctx.stage .. "/bin/lus-demo.pxb/app/sources/" })
end
