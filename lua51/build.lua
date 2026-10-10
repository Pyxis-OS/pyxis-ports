return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "LPEG=" .. ctx.extra.lpeg,
    "LUV=" .. ctx.extra.luv, "COMPAT53=" .. ctx.extra.compat53, "LIBUV=" .. ctx.libuv,
    "BUILD=" .. ctx.build, "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(mode, from, to)
    ctx.run({ "install", "-D", "-m", mode, from, ctx.stage .. "/" .. to })
  end
  install("644", ctx.build .. "/liblua5.1.a", outputs.lua_library)
  install("644", ctx.build .. "/liblpeg.a", outputs.lpeg_library)
  install("644", ctx.build .. "/libluv.a", outputs.luv_library)
  for _, name in ipairs({ "lua.h", "luaconf.h", "lauxlib.h", "lualib.h" }) do
    install("644", ctx.source .. "/src/" .. name, "dev/include/lua5.1/" .. name)
  end
  for _, name in ipairs({ "luv.h", "util.h", "lhandle.h", "lreq.h" }) do
    install("644", ctx.extra.luv .. "/src/" .. name, "dev/include/luv/" .. name)
  end
  install("644", ctx.recipe .. "/manifest.json", outputs.manifest)
  install("755", ctx.build .. "/lua5.1.pxe", outputs.interpreter)
  install("644", ctx.source .. "/COPYRIGHT", outputs.license)
  install("644", ctx.extra.lpeg .. "/lpeg.html", outputs.lpeg_license)
  install("644", ctx.extra.luv .. "/LICENSE.txt", outputs.luv_license)
  install("644", ctx.extra.compat53 .. "/LICENSE", outputs.compat53_license)
  install("644", ctx.recipe .. "/PORT-NOTICE.md", outputs.port_notice)
  for _, name in ipairs({ "license", "lpeg_license", "luv_license", "compat53_license" }) do
    install("644", ctx.stage .. "/" .. outputs[name], outputs["bundle_" .. name])
    install("644", ctx.stage .. "/" .. outputs[name], outputs["dev_" .. name])
  end
  install("644", ctx.stage .. "/" .. outputs.port_notice, outputs.dev_port_notice)
  for _, name in ipairs({ "LICENSE", "LICENSE-extra", "PORT-NOTICE" }) do
    install("644", ctx.libuv .. "/share/licenses/libuv/" .. name,
      "bin/lua5.1.pxb/app/metadata/licenses/libuv/" .. name)
  end
  install("644", ctx.libuv .. "/share/libuv/source.txt", outputs.libuv_provenance)
  install("644", ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  for _, name in ipairs({ "tlsf.h", "tlsf-upstream.md", "musl-COPYRIGHT",
    "musl-TRE-COPYRIGHT", "musl-upstream.md" }) do
    install("644", ctx.sdk .. "/share/licenses/" .. name,
      "bin/lua5.1.pxb/app/metadata/licenses/libc/" .. name)
  end
  for _, name in ipairs({ "LICENSE.TXT", "llvm-revision" }) do
    install("644", ctx.sdk .. "/share/toolchain/" .. name,
      "bin/lua5.1.pxb/app/metadata/licenses/toolchain/" .. name)
  end
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("lua=" .. ctx.metadata.source.archive.url .. "\n",
    "lua_sha256=" .. ctx.metadata.source.archive.sha256 .. "\n"))
  for _, entry in ipairs(ctx.metadata.source.extra) do
    assert(provenance:write(entry.name .. "=" .. entry.archive.url .. "\n",
      entry.name .. "_sha256=" .. entry.archive.sha256 .. "\n"))
  end
  assert(provenance:write("patches=0001-pyxis-runtime.patch luv-0001-pyxis-native.patch",
    " compat53-0001-no-reopen.patch\n",
    "license=MIT (Lua, LPeg, lua-compat-5.3), Apache-2.0 (luv)\n"))
  assert(provenance:close())
  install("644", ctx.stage .. "/" .. outputs.provenance, outputs.dev_provenance)
end
