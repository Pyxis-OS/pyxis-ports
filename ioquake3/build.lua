return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile,
    "SDL2=" .. ctx.sdl2, "SDL2_OSMESA=" .. ctx.sdl2_osmesa, "MESA=" .. ctx.mesa })
  local out = ctx.metadata.outputs
  local app = ctx.stage .. "/bin/ioquake3.pxb/app"
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local function copy_tree(from, to)
    ctx.run({ "mkdir", "-p", to })
    ctx.run({ "cp", "-R", "--", from .. "/.", to })
  end
  install(ctx.build .. "/Release/ioquake3.pxe", out.executable)
  install(ctx.recipe .. "/manifest.json", out.manifest)
  install(ctx.source .. "/COPYING.txt", out.license)
  install(ctx.recipe .. "/PORT-NOTICE", out.notice)
  install(ctx.recipe .. "/../LICENSE", out.mpl)
  install(ctx.recipe .. "/../LICENSING.md", out.licensing)
  copy_tree(ctx.sdl2 .. "/share/licenses/sdl2", app .. "/share/licenses/sdl2")
  copy_tree(ctx.sdl2_osmesa .. "/share/licenses/sdl2-osmesa", app .. "/share/licenses/sdl2-osmesa")
  copy_tree(ctx.sdl2 .. "/share/sources/sdl2", app .. "/share/sources/sdl2")
  copy_tree(ctx.mesa .. "/share/licenses/mesa", app .. "/share/licenses/mesa")
  copy_tree(ctx.sdk .. "/share/licenses", app .. "/share/licenses/sdk")
  copy_tree(ctx.sdk .. "/share/toolchain", app .. "/share/licenses/sdk/toolchain")

  -- Keep the exact patched engine and original adaptation source with the
  -- distributable GPL bundle; assets are supplied separately through packages.
  local source_dir = app .. "/share/sources/ioquake3"
  ctx.run({ "mkdir", "-p", source_dir })
  ctx.run({ "tar", "--exclude=.git", "-czf", ctx.stage .. "/" .. out.source,
    "-C", ctx.source, "." })
  copy_tree(ctx.recipe, source_dir .. "/pyxis")
  ctx.run({ "mkdir", "-p", app .. "/share/ioquake3" })
  local f = assert(io.open(ctx.stage .. "/" .. out.provenance, "w"))
  assert(f:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "profile=static opengl1, SDL2 OSMesa softpipe, interpreted asset QVMs, bundled Vorbis/Ogg/zlib/JPEG\n",
    "omitted=network, native game modules, QVM compiler, OpenAL, HTTP, VoIP, Mumble, Opus\n"))
  for _, patch in ipairs(ctx.metadata.patches) do assert(f:write("patch=" .. patch .. "\n")) end
  assert(f:close())
end
