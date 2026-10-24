return function(ctx)
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "cmake", "--install", ctx.build, "--prefix", ctx.stage .. "/dev" })
  local function install(source, target)
    ctx.run({ "install", "-D", "-m", "644", source, ctx.stage .. "/" .. target })
  end
  install(ctx.source .. "/LICENSE", ctx.metadata.outputs.license)
  install(ctx.source .. "/include/spdlog/fmt/bundled/fmt.license.rst", ctx.metadata.outputs.fmt_license)
  install(ctx.recipe .. "/PORT-NOTICE", ctx.metadata.outputs.notice)
  install(ctx.recipe .. "/../LICENSE", ctx.metadata.outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", ctx.metadata.outputs.licensing)
  install(ctx.recipe .. "/spdlog.pc", ctx.metadata.outputs.pc)
  for _, name in ipairs({ "metadata.lua", "build.lua", "Makefile", "spdlog.pc", "README.md", "PORT-NOTICE" }) do
    install(ctx.recipe .. "/" .. name, "dev/share/sources/spdlog/" .. name)
  end
  for _, patch in ipairs(ctx.metadata.patches) do
    install(ctx.recipe .. "/" .. patch, "dev/share/sources/spdlog/" .. patch)
  end
  for _, name in ipairs({ "CMakeLists.txt", "Makefile", "main.cpp", "README.md" }) do
    install(ctx.recipe .. "/manual/" .. name, "dev/share/sources/spdlog/manual/" .. name)
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/spdlog" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "archive=" .. ctx.metadata.source.archive.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=MIT\n",
    "bundled_fmt=12.0.0 MIT, include/spdlog/fmt/bundled/base.h FMT_VERSION 120000\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static bundled fmt; native threads/TLS, exceptions, stdout/color, basic/rotating file, async logging\n",
    "native=checked libc clock/calendar; timestamp-free error reporter; name:// directory roots\n",
    "unsupported=%P process-ID pattern (configuration throws); no PID accessor\n",
    "omitted=external fmt, shared library, examples, benchmarks, upstream tests\n"))
  assert(provenance:close())
end
