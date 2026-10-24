return function(ctx)
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local outputs = ctx.metadata.outputs
  install(ctx.source .. "/LICENSE", outputs.license)
  for _, codec in ipairs({ "wav", "mp3", "flac" }) do
    local header = ctx.source .. "/dr_" .. codec .. ".h"
    install(header, outputs[codec .. "_header"])
    local source = assert(io.open(header, "rb"))
    local text = assert(source:read("a"))
    assert(source:close())
    -- Preserve each header's exact license tail, including minimp3's notice.
    local notice = assert(text:match("(/%*%s*This software is available as a choice of the following licenses%..*)$"))
    local destination = assert(io.open(ctx.stage .. "/" .. outputs[codec .. "_license"], "wb"))
    assert(destination:write(notice))
    assert(destination:close())
  end
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)
  install(ctx.recipe .. "/dr_libs.pc", outputs.pc_dr_libs)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/dr_libs/pyxis" })
  ctx.run({ "cp", "-R", ctx.recipe .. "/.", ctx.stage .. "/dev/share/dr_libs/pyxis/" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "archive=" .. ctx.metadata.source.archive.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=unchanged dr_wav, dr_mp3 and dr_flac headers\n",
    "header_versions=dr_wav 0.13.16; dr_mp3 0.6.39; dr_flac 0.12.42\n",
    "implementation=consumer-owned, exactly one per header\n",
    "configuration=caller-selected; standard FILE APIs and codec defaults retained\n",
    "pyxis_source=share/dr_libs/pyxis\n"))
  assert(provenance:close())
end
