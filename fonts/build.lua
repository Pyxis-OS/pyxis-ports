return function(ctx)
  local metadata = ctx.metadata
  local roots = { main = ctx.source, sans = ctx.extra.sans,
    serif = ctx.extra.serif, recipe = ctx.recipe }
  local checksums = assert(io.open(ctx.build .. "/assets.sha256", "w"))
  for _, asset in ipairs(metadata.assets) do
    ctx.run({ "install", "-D", "-m", "644", roots[asset.source] .. "/" .. asset.path,
      ctx.stage .. "/" .. metadata.outputs[asset.key] })
    assert(checksums:write(asset.sha256, "  ", ctx.stage, "/",
      metadata.outputs[asset.key], "\n"))
  end
  assert(checksums:close())
  ctx.run({ "sha256sum", "--check", "--status", ctx.build .. "/assets.sha256" })

  for _, family in ipairs(metadata.families) do
    local archive = metadata.source.archive
    for _, extra in ipairs(metadata.source.extra) do
      if extra.name == family.source then
        archive = extra.archive
      end
    end
    local file = assert(io.open(ctx.stage .. "/" ..
      metadata.outputs[family.source .. "_provenance"], "w"))
    assert(file:write("family=", family.name, "\nrelease=", family.release,
      "\nsource=", archive.url, "\narchive_sha256=", archive.sha256,
      "\nlicense=", family.license,
      "\nlocal_changes=none; fonts and upstream notices are copied unchanged\n"))
    if family.note then
      assert(file:write("note=", family.note, "\n"))
    end
    if family.source == "main" then
      assert(file:write(
        "nerd_fonts_commit=", metadata.source.commit, "\n",
        "icons=Codicons and Font Awesome: CC-BY-4.0; Devicons, Font Awesome Extension, ",
        "Octicons, Seti/original, Powerline Extra/Symbols and IEC: MIT; ",
        "Hack extra glyphs: MIT and Bitstream Vera; MaterialDesign: Apache-2.0 ",
        "and Pictogrammers statement; Pomicons and Weather Icons: OFL-1.1; ",
        "Font Logos: Unlicense/public-domain dedication\n",
        "font_awesome_extension=MIT classification is from pinned Nerd Fonts metadata; ",
        "no separate original-author licence file was independently verified\n",
        "attribution=see icons-notices/nerd-fonts/src/glyphs/README.md, ",
        "icons-notices/nerd-fonts/LICENSE and individual source notices\n"))
    end
    for _, asset in ipairs(metadata.assets) do
      if asset.family == family.name then
        assert(file:write("\nfile=", asset.target,
          "\nsha256=", asset.sha256, "\nbytes=", tostring(asset.bytes), "\n"))
        if asset.style then
          assert(file:write("style=", asset.style, "\n"))
        end
        if asset.url then
          assert(file:write("source=", asset.url, "\n"))
        else
          assert(file:write("archive_member=", asset.path, "\n"))
        end
      end
    end
    assert(file:close())
  end
end
