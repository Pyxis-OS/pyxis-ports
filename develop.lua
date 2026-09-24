-- Build inputs for downstream programs; these are not boot-archive contents.
return function(inputs)
  return {
    { tree = inputs.ports .. "/lua/stage/dev", at = "lua" },
  }
end
