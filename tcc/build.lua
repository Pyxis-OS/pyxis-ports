return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "STAGE=" .. ctx.stage, "CROSS_COMPILE=" .. ctx.cross_compile })
  ctx.run({ "install", "-D", "-m", "755",
    ctx.build .. "/host/x86_64-pyxis-tcc",
    ctx.stage .. "/" .. ctx.metadata.outputs.host_compiler })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/guest/tcc.pxe",
    ctx.stage .. "/" .. ctx.metadata.outputs.compiler })
  ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/libtcc1.a",
    ctx.stage .. "/" .. ctx.metadata.outputs.support })

  for _, name in ipairs({ "stddef.h", "stdarg.h", "stdbool.h", "float.h" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/include/" .. name,
      ctx.stage .. "/lib/tcc/include/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/COPYING",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  -- Preserve the runtime sources' individual notices, including the exception.
  for _, name in ipairs({ "libtcc1.c", "va_list.c", "builtin.c" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/lib/" .. name,
      ctx.stage .. "/share/licenses/tcc/" .. name })
  end
end
