return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "CROSS_COMPILE=" .. ctx.cross_compile })
  local function install(from, to)
    ctx.run({ "install", "-D", "-m", "644", from, ctx.stage .. "/" .. to })
  end
  local out = ctx.metadata.outputs
  install(ctx.build .. "/mesa/src/gallium/targets/osmesa/libOSMesa.a", out.library)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile", "strip-library",
    "SDK=" .. ctx.sdk, "CROSS_COMPILE=" .. ctx.cross_compile,
    "STAGED_LIBRARY=" .. ctx.stage .. "/" .. out.library })
  for _, file in ipairs({ "GL/gl.h", "GL/glext.h", "GL/glcorearb.h", "GL/osmesa.h", "KHR/khrplatform.h" }) do
    install(ctx.source .. "/include/" .. file, "dev/include/" .. file)
  end
  install(ctx.recipe .. "/osmesa.pc", out.pc)
  install(ctx.recipe .. "/PORT-NOTICE", out.notice)
  install(ctx.recipe .. "/notices/GPL-3.0-or-later", out.bison_gpl)
  install(ctx.recipe .. "/../LICENSE", out.mpl)
  install(ctx.recipe .. "/../LICENSING.md", out.licensing)
  install(ctx.source .. "/docs/license.rst", out.license_guide)
  for _, name in ipairs({ "MIT", "BSL-1.0", "Apache-2.0", "SGI-B-2.0" }) do
    install(ctx.source .. "/licenses/" .. name, "dev/share/licenses/mesa/" .. name)
  end
  ctx.run({ "python3", ctx.recipe .. "/notices.py", ctx.source, ctx.build .. "/mesa", ctx.build .. "/NOTICES" })
  install(ctx.build .. "/NOTICES", out.notices)
  install(ctx.build .. "/mesa-triangle.pxe", out.manual)
  install(ctx.recipe .. "/manual/manifest.json", out.manifest)
  install(ctx.recipe .. "/manual/triangle.c", out.manual_source)
  install(ctx.recipe .. "/manual/manifest.json", out.manual_manifest)
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/mesa" })
  local f = assert(io.open(ctx.stage .. "/" .. out.provenance, "w"))
  assert(f:write("version=" .. ctx.metadata.version .. "\n",
    "source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "osmesa-source=https://archive.mesa3d.org/mesa-25.0.7.tar.xz\n",
    "osmesa-sha256=592272df3cf01e85e7db300c449df5061092574d099da275d19e97ef0510f8a6\n",
    "profile=static softpipe, OpenGL, native C11 synchronization, no JIT/LLVM\n",
    "omitted=OSMesaPostprocess, GPU, window systems, GLES, disk cache, DRM descriptor sharing\n"))
  for _, patch in ipairs(ctx.metadata.patches) do assert(f:write("patch=" .. patch .. "\n")) end
  assert(f:close())
end
