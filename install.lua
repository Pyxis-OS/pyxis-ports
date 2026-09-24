-- Guest paths are owned here; host tools and build intermediates stay out.
-- Pyxis's staging runner supplies the per-port work directory as inputs.ports.
return function(inputs)
  local root = inputs.ports
  return {
    { tree = root .. "/kilo/stage/bin", at = "" },
    { tree = root .. "/kilo/stage/share", at = "share" },
    { tree = root .. "/doom/stage/bin", at = "" },
    { tree = root .. "/doom/stage/share", at = "share" },
    { tree = root .. "/tcc/stage/bin", at = "" },
    { tree = root .. "/tcc/stage/lib/tcc", at = "sdk/lib/tcc" },
    { tree = root .. "/tcc/stage/share", at = "sdk/share" },
  }
end
