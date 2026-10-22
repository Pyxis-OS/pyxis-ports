-- Guest paths are owned here; host tools and build intermediates stay out.
-- Pyxis's staging runner supplies the per-port work directory as inputs.ports.
return function(inputs)
  local root = inputs.ports
  return {
    { tree = root .. "/ca-certificates/stage/share", at = "share" },
    { tree = root .. "/sbase/stage/bin", at = "" },
    { tree = root .. "/sbase/stage/share", at = "share" },
    { tree = root .. "/fastfetch/stage/bin", at = "" },
    { tree = root .. "/fastfetch/stage/share", at = "share" },
    { tree = root .. "/busybox/stage/bin", at = "" },
    { tree = root .. "/busybox/stage/share", at = "share" },
    { tree = root .. "/kilo/stage/bin", at = "" },
    { tree = root .. "/kilo/stage/share", at = "share" },
    { tree = root .. "/links/stage/bin", at = "" },
    { tree = root .. "/links/stage/share", at = "share" },
    { tree = root .. "/jq/stage/bin", at = "" },
    { tree = root .. "/jq/stage/share", at = "share" },
    { tree = root .. "/lua/stage/bin", at = "" },
    { tree = root .. "/lua/stage/share", at = "share" },
    { tree = root .. "/picohttpparser/stage/share", at = "share" },
    { tree = root .. "/mbedtls/stage/share", at = "share" },
    { tree = root .. "/zlib/stage/share", at = "share" },
    { tree = root .. "/libpng/stage/share", at = "share" },
    { tree = root .. "/libuv/stage/share", at = "share" },
    { tree = root .. "/lua51/stage/bin", at = "" },
    { tree = root .. "/lua51/stage/share", at = "share" },
    { tree = root .. "/utf8proc/stage/share", at = "share" },
    { tree = root .. "/tree-sitter/stage/share", at = "share" },
    { tree = root .. "/neovim/stage/bin", at = "" },
    { tree = root .. "/tzdata/stage/share", at = "share" },
    { tree = root .. "/pciids/stage/share", at = "share" },
    { tree = root .. "/usbids/stage/share", at = "share" },
  }
end
