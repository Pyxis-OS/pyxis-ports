-- Build inputs for downstream programs; these are not boot-archive contents.
return function(inputs)
  return {
    { tree = inputs.ports .. "/lua/stage/dev", at = "lua" },
    { tree = inputs.ports .. "/picohttpparser/stage/dev", at = "picohttpparser" },
    { tree = inputs.ports .. "/mbedtls/stage/dev", at = "mbedtls" },
    { tree = inputs.ports .. "/zlib/stage/dev", at = "zlib" },
    { tree = inputs.ports .. "/libpng/stage/dev", at = "libpng" },
    { tree = inputs.ports .. "/fmt/stage/dev", at = "fmt" },
    { tree = inputs.ports .. "/sdl2/stage/dev", at = "sdl2" },
  }
end
