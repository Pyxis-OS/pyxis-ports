return function(ctx)
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local outputs = ctx.metadata.outputs
  install(ctx.source .. "/include/BS_thread_pool.hpp", outputs.header)
  install(ctx.source .. "/include/BS_thread_pool_utils.hpp", outputs.utility_header)
  install(ctx.source .. "/LICENSE.txt", outputs.license)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/thread-pool.pc", outputs.pc_thread_pool)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/thread-pool/pyxis" })
  ctx.run({ "cp", "-R", ctx.recipe .. "/.", ctx.stage .. "/dev/share/thread-pool/pyxis/" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "archive=" .. ctx.metadata.source.archive.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("profile=header-only, caller-selected feature macros, real C++ workers\n",
    "local_change=completion predicate remains under the existing task mutex\n",
    "accepted=chronium, 2026-10-24; remove local patch upon an upstream fix\n",
    "omitted=upstream test programs\n",
    "pyxis_source=share/thread-pool/pyxis\n"))
  assert(provenance:close())
end
