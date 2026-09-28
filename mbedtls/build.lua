return function(ctx)
  ctx.run({ "make", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe,
    "CROSS_COMPILE=" .. ctx.cross_compile })
  for _, name in ipairs({ "libmbedtls.a", "libmbedx509.a", "libtfpsacrypto.a" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.build .. "/library/" .. name,
      ctx.stage .. "/dev/lib/" .. name })
  end
  ctx.run({ "mkdir", "-p", "--", ctx.stage .. "/dev/include" })
  ctx.run({ "cp", "-R", "--", ctx.source .. "/include/mbedtls", ctx.stage .. "/dev/include/" })
  for _, name in ipairs({ "mbedtls", "psa", "tf-psa-crypto" }) do
    ctx.run({ "cp", "-R", "--", ctx.source .. "/tf-psa-crypto/include/" .. name,
      ctx.stage .. "/dev/include/" })
  end
  ctx.run({ "cp", "-R", "--", ctx.source .. "/tf-psa-crypto/drivers/builtin/include/mbedtls",
    ctx.stage .. "/dev/include/" })
  for _, name in ipairs({ "pyxis_tls_config.h", "pyxis_crypto_config.h" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/" .. name,
      ctx.stage .. "/dev/include/mbedtls/" .. name })
  end
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/mbedtls.mk",
    ctx.stage .. "/" .. ctx.metadata.outputs.make_config })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.license })
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/tf-psa-crypto/LICENSE",
    ctx.stage .. "/" .. ctx.metadata.outputs.crypto_license })
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/PORT-NOTICE",
    ctx.stage .. "/" .. ctx.metadata.outputs.notice })
end
