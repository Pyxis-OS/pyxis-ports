return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  install(ctx.build .. "/libtree-sitter.a", outputs.library)
  install(ctx.source .. "/lib/include/tree_sitter/api.h", outputs.header)
  install(ctx.source .. "/lib/LICENSE", outputs.license)
  install(ctx.source .. "/lib/src/unicode/LICENSE", outputs.unicode_license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/tree-sitter" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=0001-pyxis-endian.patch\n",
    "profile=static C library, upstream amalgamation, Wasm disabled\n",
    "omitted=parser modules, Wasmtime, shared library, CLI, upstream programs/tests\n"))
  assert(provenance:close())
end
