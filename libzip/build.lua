return function(ctx)
  assert(ctx.zlib, "libzip requires the selected zlib development prefix")
  ctx.run({ "make", "-j4", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "ZLIB=" .. ctx.zlib,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "cmake", "--install", ctx.build, "--prefix", ctx.stage .. "/dev" })
  local function install(source, target)
    ctx.run({ "install", "-D", "-m", "644", source, ctx.stage .. "/" .. target })
  end
  install(ctx.source .. "/LICENSE", ctx.metadata.outputs.license)
  install(ctx.recipe .. "/PORT-NOTICE", ctx.metadata.outputs.notice)
  install(ctx.recipe .. "/../LICENSE", ctx.metadata.outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", ctx.metadata.outputs.licensing)
  install(ctx.recipe .. "/libzip.pc", ctx.metadata.outputs.pc)
  for _, name in ipairs({ "metadata.lua", "build.lua", "Makefile", "libzip.pc", "README.md", "PORT-NOTICE" }) do
    install(ctx.recipe .. "/" .. name, "dev/share/sources/libzip/" .. name)
  end
  for _, patch in ipairs(ctx.metadata.patches) do
    install(ctx.recipe .. "/" .. patch, "dev/share/sources/libzip/" .. patch)
  end
  for _, name in ipairs({ "CMakeLists.txt", "Makefile", "main.c", "README.md" }) do
    install(ctx.recipe .. "/manual/" .. name, "dev/share/sources/libzip/manual/" .. name)
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/libzip" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "archive=" .. ctx.metadata.source.archive.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=BSD-3-Clause\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=static store/deflate with separate zlib; native libc FILE/descriptors/getentropy\n",
    "metadata=unknown native timestamps/permissions remain absent; checked clock when a new ZIP entry needs a timestamp\n",
    "entropy=every archive open requires native random/clock for keyed hashing; temporary files fail before creation on entropy failure\n",
    "unsupported=all encryption, including PKWARE and AES; bzip2, LZMA, Zstandard\n",
    "omitted=tools, documentation, examples, upstream regressions/fuzzers, shared libraries\n"))
  assert(provenance:close())
end
