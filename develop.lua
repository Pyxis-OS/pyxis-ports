-- Build inputs for downstream programs; these are not boot-archive contents.
return function(inputs)
  return {
    { tree = inputs.ports .. "/lua/stage/dev", at = "lua" },
    { tree = inputs.ports .. "/picohttpparser/stage/dev", at = "picohttpparser" },
  }
end
