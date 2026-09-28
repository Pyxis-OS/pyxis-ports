return function(ctx)
  ctx.run({ "install", "-D", "-m", "644", ctx.source .. "/cacert.pem",
    ctx.stage .. "/" .. ctx.metadata.outputs.bundle })
  for _, name in ipairs({ "LICENSE", "NOTICE" }) do
    ctx.run({ "install", "-D", "-m", "644", ctx.recipe .. "/" .. name,
      ctx.stage .. "/share/licenses/ca-certificates/" .. name })
  end

  local checksum = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.checksum, "w"))
  assert(checksum:write(ctx.metadata.source.file.sha256 .. "  cacert.pem\n"))
  assert(checksum:close())

  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.file.url .. "\n",
    "sha256=" .. ctx.metadata.source.file.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "bytes=188900\ncertificates=121\n",
    "mozilla_data_time=Fri Sep 25 03:12:01 2026 GMT\n",
    "mozilla_source=https://raw.githubusercontent.com/mozilla-firefox/firefox/refs/heads/release/security/nss/lib/ckfw/builtins/certdata.txt\n",
    "mozilla_source_sha256=beb7e6dfe6499926e52c075c27bcfbe4c957f8609c575b3860273ae2806f63eb\n",
    "conversion=mk-ca-bundle.pl version 1.33 (curl upstream)\n",
    "conversion_documentation=https://curl.se/docs/caextract.html\n",
    "license=MPL-2.0\npatches=none\n",
    "local_changes=none; staged PEM is byte-for-byte upstream\n",
    "limits=PEM export omits Mozilla additional trust-store constraints\n"))
  assert(provenance:close())
end
