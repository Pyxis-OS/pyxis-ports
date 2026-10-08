-- Build inputs for downstream programs; these are not boot-archive contents.
return function(inputs)
  return {
    { tree = inputs.ports .. "/lua/stage/dev", at = "lua" },
    { tree = inputs.ports .. "/picohttpparser/stage/dev", at = "picohttpparser" },
    { tree = inputs.ports .. "/mbedtls/stage/dev", at = "mbedtls" },
    { tree = inputs.ports .. "/zlib/stage/dev", at = "zlib" },
  }
end
