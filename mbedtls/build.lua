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
  for _, name in ipairs({ "license", "crypto_license", "notice" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.stage .. "/" .. ctx.metadata.outputs[name],
      ctx.stage .. "/" .. ctx.metadata.outputs["dev_" .. name] })
  end
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/mbedtls" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.dev_provenance, "w"))
  local crypto = ctx.metadata.bundled_crypto
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "crypto_source=" .. crypto.url .. "\ncrypto_commit=" .. crypto.commit .. "\n",
    "crypto_version=" .. crypto.version .. "\nversion=4.1.1\nlicense=Apache-2.0\n",
    "configuration=include/mbedtls/pyxis_tls_config.h,include/mbedtls/pyxis_crypto_config.h\n",
    "profile=static TLS,X.509,TF-PSA-Crypto with native caller hooks\n",
    "omitted=programs,tests,filesystem I/O,persistent crypto storage,builtin entropy\n"))
  for _, patch in ipairs(ctx.metadata.patches) do
    assert(provenance:write("patch=" .. patch .. "\n"))
  end
  assert(provenance:write("sha256_compression=SDK libsha256; upstream SHA-224/SHA-256 framing\n"))
  assert(provenance:close())
  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/mbedtls.pc",
    ctx.stage .. "/dev/lib/pkgconfig/mbedtls.pc" })

  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/mbedx509.pc",
    ctx.stage .. "/dev/lib/pkgconfig/mbedx509.pc" })

  ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/tfpsacrypto.pc",
    ctx.stage .. "/dev/lib/pkgconfig/tfpsacrypto.pc" })

  for _, entry in ipairs({
    { source = "LICENSE", output = "dev_pyxis_license" },
    { source = "LICENSING.md", output = "dev_licensing" },
  }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/../" .. entry.source,
      ctx.stage .. "/" .. ctx.metadata.outputs[entry.output] })
  end

end
