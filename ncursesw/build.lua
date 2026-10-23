return function(ctx)
  ctx.run({ "make", "-j16", "-f", ctx.recipe .. "/Makefile",
    "SDK=" .. ctx.sdk, "SOURCE=" .. ctx.source, "BUILD=" .. ctx.build,
    "RECIPE=" .. ctx.recipe, "HOSTCC=" .. (os.getenv("HOSTCC") or "cc"),
    "CROSS_COMPILE=" .. ctx.cross_compile })
  local function install(source, target)
    ctx.run({ "install", "-D", "-m", "644", source, ctx.stage .. "/" .. target })
  end
  install(ctx.build .. "/lib/libncursesw.a", ctx.metadata.outputs.library)
  for _, name in ipairs({ "curses.h", "term.h", "termcap.h", "unctrl.h", "ncurses_dll.h" }) do
    install(ctx.build .. "/include/" .. name, "dev/include/" .. name)
  end
  install(ctx.recipe .. "/ncursesw.pc", ctx.metadata.outputs.pc)
  install(ctx.recipe .. "/ncurses.pc", ctx.metadata.outputs.pc_alias)
  install(ctx.recipe .. "/pyxis.ti", ctx.metadata.outputs.terminfo_source)
  ctx.run({ "tic", "-x", "-o", ctx.stage .. "/dev/share/terminfo", ctx.recipe .. "/pyxis.ti" })
  install(ctx.source .. "/COPYING", ctx.metadata.outputs.license)
  local bundle = "bin/ncurses-demo.pxb/"
  install(ctx.build .. "/ncurses-demo.pxe", bundle .. "app/bin/ncurses-demo.pxe")
  install(ctx.recipe .. "/manifest.json", bundle .. "manifest.json")
  install(ctx.source .. "/COPYING", bundle .. "app/metadata/ncurses-COPYING")
  install(ctx.recipe .. "/PORT-NOTICE", bundle .. "app/metadata/PORT-NOTICE")
  install(ctx.recipe .. "/../LICENSE", bundle .. "app/metadata/pyxis-ports-LICENSE")
  ctx.run({ "mkdir", "-p", ctx.stage .. "/dev/share/ncursesw" })
  local provenance = assert(io.open(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\nversion=6.6\n",
    "signature=https://ftp.gnu.org/gnu/ncurses/ncurses-6.6.tar.gz.sig\n",
    "signing_fingerprint=19882D92DDA4C400C22C0D56CC2AF4472167BE03\n",
    "patch=0001-native-terminal-driver.patch\n",
    "profile=static wide curses; embedded pyxis entry; native bytes, modes, resize, waits\n",
    "omitted=Unix terminal/signal backends, external databases, extended colours, mouse protocols, C++ and Ada bindings, tools, menu/form/panel libraries\n"))
  assert(provenance:close())
  install(ctx.stage .. "/" .. ctx.metadata.outputs.provenance, bundle .. "app/metadata/source.txt")
end
