return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile,
    "ZLIB=" .. ctx.zlib, "LIBPNG=" .. ctx.libpng, "FMT=" .. ctx.fmt, "SDL2=" .. ctx.sdl2,
    "BZIP2_SOURCE=" .. ctx.extra.bzip2, "LIBMPQ_SOURCE=" .. ctx.extra.libmpq,
    "LIBSMACKERDEC_SOURCE=" .. ctx.extra.libsmackerdec,
    "SIMPLEINI_SOURCE=" .. ctx.extra.simpleini, "SDL_IMAGE_SOURCE=" .. ctx.extra.sdl_image })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/devilutionx.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.executable })
  ctx.run({ "mkdir", "-p", "--", ctx.stage .. "/share/devilutionx" })
  ctx.run({ "cp", "-R", "--", ctx.build .. "/assets", ctx.stage .. "/share/devilutionx/" })

  -- The program ships with every license of the code linked into it; zlib's
  -- and libpng's are already staged by their own ports.
  local licenses = ctx.stage .. "/share/licenses/devilutionx/"
  local function license(from, name)
    ctx.run({ "install", "-D", "-m", "644", from, licenses .. name })
  end
  license(ctx.source .. "/LICENSE.md", "LICENSE.md")
  license(ctx.recipe .. "/PORT-NOTICE", "PORT-NOTICE")
  -- The bundled UTF-8 decoder's header carries its BSD notice.
  license(ctx.source .. "/3rdParty/hoehrmann_utf8/hoehrmann_utf8.h", "hoehrmann_utf8.h")
  license(ctx.extra.bzip2 .. "/LICENSE", "bzip2-LICENSE")
  license(ctx.extra.libmpq .. "/COPYING", "libmpq-COPYING")
  license(ctx.extra.libsmackerdec .. "/COPYING", "libsmackerdec-COPYING")
  license(ctx.extra.simpleini .. "/LICENCE.txt", "simpleini-LICENCE.txt")
  license(ctx.extra.sdl_image .. "/COPYING.txt", "SDL_image-COPYING.txt")
  license(ctx.sdl2 .. "/share/licenses/sdl2/LICENSE.txt", "SDL2-LICENSE.txt")
  license(ctx.sdl2 .. "/share/licenses/sdl2/PORT-NOTICE", "SDL2-PORT-NOTICE")
  license(ctx.fmt .. "/share/licenses/fmt/LICENSE", "fmt-LICENSE")

  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  for _, entry in ipairs(ctx.metadata.source.extra) do
    local pin = entry.commit or entry.archive.sha256
    assert(provenance:write("dependency=" .. entry.name .. " " .. entry.url .. " " .. pin .. "\n"))
  end
  assert(provenance:write("profile=single player, no network, no sound, loose assets\n",
    "distribution=personal use only; do not share images containing this program\n"))
  assert(provenance:close())
end
