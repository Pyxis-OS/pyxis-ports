return function(ctx)
  assert(ctx.spdlog, "Prism needs --spdlog PATH to the compiled bundled-fmt development prefix")
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "SPDLOG=" .. ctx.spdlog,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  local outputs = ctx.metadata.outputs
  local function install(from, output)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. output })
  end
  install(ctx.build .. "/libprism.a", outputs.library)
  for _, name in ipairs({ "processor.h", "lexer.h", "ast.h", "utils/invoke.h",
    "utils/exceptions.h", "utils/gv.h" }) do
    install(ctx.source .. "/src/prism/" .. name, "dev/include/prism/" .. name)
  end
  install(ctx.source .. "/LICENSE", outputs.license)
  install(ctx.recipe .. "/prism.pc", outputs.pc)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  for _, name in ipairs({ "Makefile", "build.lua", "metadata.lua",
    "prism.pc", "README.md", "PORT-NOTICE" }) do
    install(ctx.recipe .. "/" .. name, "dev/share/prism/pyxis/" .. name)
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/prism" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=MIT\npatches=none\n",
    "profile=static C++20 library, C11 invoke bridge, exceptions and RTTI\n",
    "dependency=compiled spdlog with bundled fmt, SPDLOG_COMPILED_LIB\n",
    "omitted=standalone CLI, examples, DEBUG_PARSE, ASan, CMake fetches\n"))
  assert(provenance:close())
end
