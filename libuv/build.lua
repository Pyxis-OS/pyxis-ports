return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libuv.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.library })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/examples/manifest.json",
    ctx.stage .. "/" .. ctx.metadata.outputs.sample_manifest })
  ctx.run({ "install", "-D", "-m", "755", ctx.build .. "/uv-relay.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.sample_program })
  for _, name in ipairs({ "uv.h", "uv/errno.h", "uv/version.h", "uv/threadpool.h" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/include/" .. name,
      ctx.stage .. "/dev/include/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/include/uv/pyxis.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.platform })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/include/uv/pyxis-native.h",
    ctx.stage .. "/" .. ctx.metadata.outputs.native })
  for _, name in ipairs({ "LICENSE", "LICENSE-extra" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/" .. name,
      ctx.stage .. "/share/licenses/libuv/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })
  for _, entry in ipairs({
    { source = "include/uv/tree.h", output = "tree_license" },
    { source = "src/heap-inl.h", output = "heap_license" },
    { source = "src/queue.h", output = "queue_license" },
  }) do
    for _, prefix in ipairs({ "", "dev_" }) do
      ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/" .. entry.source,
        ctx.stage .. "/" .. ctx.metadata.outputs[prefix .. entry.output] })
    end
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/libuv" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\nlicense=" .. ctx.metadata.license .. "\n",
    "patches=0001-pyxis-platform.patch\n",
    "profile=native one-thread loop, pipes, console, child observation, timers, synchronous filesystem\n",
    "omitted=Unix/Windows backends, workers, async filesystem, sockets, watches, signals, module loading\n"))
  assert(provenance:close())
  for _, name in ipairs({ "license", "extra_license", "notice", "provenance" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.stage .. "/" .. ctx.metadata.outputs[name],
      ctx.stage .. "/" .. ctx.metadata.outputs["dev_" .. name] })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/libuv.pc",
    ctx.stage .. "/dev/lib/pkgconfig/libuv.pc" })

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

  for _, name in ipairs({
    "Makefile",
    "include/uv/pyxis.h",
    "include/uv/pyxis-native.h",
    "pyxis/core.c",
    "pyxis/fs.c",
    "pyxis/internal.h",
    "pyxis/os.c",
    "pyxis/process.c",
    "pyxis/stream.c",
    "pyxis/thread.c",
    "pyxis/unsupported.c",
    "pyxis/watchers.c",
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/" .. name,
      ctx.stage .. "/dev/share/sources/libuv/" .. name })
  end

end
