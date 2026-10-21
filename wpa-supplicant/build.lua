return function(ctx)
  local sources = dofile(ctx.recipe .. "/sources.lua")
  local objects = {}
  for _, source in ipairs(sources) do
    objects[#objects + 1] = source:gsub("%.c$", "")
  end
  ctx.run({ "make", "-j8", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile,
    "SOURCES=" .. table.concat(objects, " ") })
  local outputs = ctx.metadata.outputs
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  install(ctx.build .. "/libwpa-rsn.a", outputs.library)
  for _, name in ipairs({ "wpa-rsn.h", "wpa-rsn-config.h" }) do
    install(ctx.recipe .. "/include/" .. name, "dev/include/wpa-rsn/" .. name)
  end
  local headers = dofile(ctx.recipe .. "/headers.lua")
  for _, header in ipairs(headers) do
    install(ctx.source .. "/src/" .. header, "dev/include/wpa-rsn/" .. header)
  end
  install(ctx.recipe .. "/wpa-rsn.mk", outputs.makefile)
  install(ctx.source .. "/README", outputs.license)
  install(ctx.source .. "/COPYING", outputs.copying)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)

  local notices = assert(io.open(ctx.stage .. "/" .. outputs.upstream_notices, "w"))
  -- Keep copyright/public-domain grants embedded in compiled files and headers,
  -- including the original Rijndael and hash implementation notices.
  for _, files in ipairs({ sources, headers }) do
    for _, name in ipairs(files) do
      local input = assert(io.open(ctx.source .. "/src/" .. name, "r"))
      local content = assert(input:read("a"))
      assert(input:close())
      assert(notices:write("\n== src/", name, " ==\n"))
      for comment in content:gmatch("/%*.-%*/") do
        local lower = comment:lower()
        if lower:find("copyright", 1, true) or lower:find("public domain", 1, true) or
            lower:find("redistribution", 1, true) then
          assert(notices:write(comment, "\n"))
        end
      end
    end
  end
  assert(notices:close())
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/wpa-rsn" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=0001-pyxis-native-library.patch\n",
    "profile=RSN supplicant static library; native caller owns policy/events/random\n",
    "omitted=daemon, socket drivers, EAP, config writer, control sockets, entropy pool, NTP helper\n"))
  for _, source in ipairs(sources) do
    assert(provenance:write("compiled=src/", source, "\n"))
  end
  assert(provenance:close())
  for _, name in ipairs({ "license", "copying", "notice", "upstream_notices", "provenance" }) do
    install(ctx.stage .. "/" .. outputs[name], outputs["dev_" .. name])
  end
end
