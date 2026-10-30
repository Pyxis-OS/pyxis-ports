return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "MESA=" .. ctx.mesa, "CROSS_COMPILE=" .. ctx.cross_compile })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local outputs = ctx.metadata.outputs
  install(ctx.build .. "/libGLU.a", outputs.library)
  install(ctx.source .. "/include/GL/glu.h", outputs.header)
  install(ctx.recipe .. "/cmake/GLUConfig.cmake", outputs.package)
  install(ctx.recipe .. "/glu.pc", outputs.pc)
  install(ctx.recipe .. "/PORT-NOTICE", outputs.notice)
  install(ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  install(ctx.recipe .. "/../LICENSING.md", outputs.licensing)

  -- The tarball has no licence file; each source carries its notice. Most
  -- carry SGI Free Software License B 2.0, the NURBS sources the 1.1 notice.
  local function first_comment(path, output)
    local source = assert(io.open(ctx.source .. "/" .. path, "r"))
    local text = assert(source:read("a"))
    assert(source:close())
    local notice_file = ctx.build .. "/" .. output
    local notice = assert(io.open(notice_file, "w"))
    assert(notice:write(assert(text:match("^(/%*.-%*/)")), "\n"))
    assert(notice:close())
    install(notice_file, outputs[output])
  end
  first_comment("include/GL/glu.h", "license")
  first_comment("src/libnurbs/interface/bezierEval.cc", "nurbs_license")

  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/glu" })
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.url .. "\n",
    "mirror=" .. ctx.metadata.source.archive.mirror .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=none\nprofile=static library: utility, tessellator and NURBS; over static OSMesa\n",
    "omitted=shared library\n"))
  assert(provenance:close())
end
