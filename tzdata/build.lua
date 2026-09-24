return function(ctx)
  -- Build only the matching host compiler and standard data. No host zoneinfo,
  -- target libc, leap-second tree, default localtime link or upstream self-tests.
  -- A commit-only fetch has no release tags for upstream's git describe rule.
  -- Supply the pinned release's generated version file explicitly.
  local version = assert(io.open(ctx.source .. "/version", "w"))
  assert(version:write(ctx.metadata.version .. "\n"))
  assert(version:close())
  ctx.run({ "make", "-C", ctx.source, "--assume-old=version", "zic", "tzdata.zi",
    "CC=" .. (os.getenv("HOSTCC") or "cc"), "CFLAGS=-O2", "CPPFLAGS=", "LDFLAGS=",
    "VERSION=" .. ctx.metadata.version, "DATAFORM=main", "BACKWARD=backward",
    "PACKRATDATA=", "PACKRATLIST=", "REDO=posix_only" })
  local compiled = ctx.build .. "/zoneinfo"
  local destination = ctx.stage .. "/share/zoneinfo"
  ctx.run({ "mkdir", "-p", compiled, destination })
  ctx.run({ ctx.source .. "/zic", "-b", "slim", "-d", compiled,
    ctx.source .. "/tzdata.zi" })

  -- zic may use hard links or symlinks for aliases. The guest archive needs
  -- independent regular files, including when the host supports those links.
  ctx.run({ "cp", "-RL", "--no-preserve=links", compiled .. "/.", destination })
  for _, name in ipairs({ "tzdata.zi", "version", "iso3166.tab", "zone.tab",
                          "zone1970.tab", "zonenow.tab" }) do
    ctx.run({ "install", "-m", "644", ctx.source .. "/" .. name,
      destination .. "/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "mkdir", "-p", ctx.stage .. "/share/tzdata" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "compiler=zic from the same source pin\n",
    "dataform=main\nbackward=backward\npackratdata=\n",
    "zic_flags=-b slim; no range cutoff or leap-second table\n",
    "aliases=independent regular files\npatches=none\n"))
  assert(provenance:close())
end
