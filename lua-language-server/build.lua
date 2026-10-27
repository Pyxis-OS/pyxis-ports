return function(ctx)
  ctx.run({ "make", "-j8", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "RECIPE=" .. ctx.recipe, "BUILD=" .. ctx.build,
    "BEE=" .. assert(ctx.extra.bee), "LPEGLABEL=" .. assert(ctx.extra.lpeglabel),
    "VERSION=" .. ctx.metadata.version, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "python3", ctx.recipe .. "/stage.py", "--recipe", ctx.recipe,
    "--source", ctx.source, "--build", ctx.build, "--stage", ctx.stage,
    "--bee", ctx.extra.bee, "--parser", ctx.extra.lpeglabel,
    "--sdk", ctx.sdk, "--json", ctx.extra.json, "--serialization-notice", ctx.extra.serialize_notice })
  local file = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(file:write("version=" .. ctx.metadata.version .. "\n",
    "source=" .. ctx.metadata.source.url .. "\ncommit=" .. ctx.metadata.source.commit .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "profile=native byte-pipe stdio; embedded Lua55, real workers, upstream Lua configuration and plugins\n",
    "omitted=TCP, subprocess, watchers, formatter\n"))
  for _, entry in ipairs(ctx.metadata.source.extra) do
    assert(file:write("extra=" .. entry.name .. " " .. entry.commit .. " " .. entry.archive.sha256 .. "\n"))
    for _, patch in ipairs(entry.patches or {}) do assert(file:write("extra_patch=" .. patch .. "\n")) end
  end
  for _, patch in ipairs(ctx.metadata.patches) do assert(file:write("patch=" .. patch .. "\n")) end
  assert(file:close())
end
