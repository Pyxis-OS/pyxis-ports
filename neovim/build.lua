return function(ctx)
  for _, name in ipairs({ "lua51", "libuv", "utf8proc", "tree_sitter" }) do
    assert(ctx[name], "Neovim needs the " .. name .. " development prefix")
  end
  local host_lua = assert(ctx.extra.host_lua) .. "/src"
  local host_lpeg = assert(ctx.extra.host_lpeg)
  local host_module = ctx.build .. "/host/nlua0.so"
  local lua = host_lua .. "/lua"
  ctx.run({ "make", "-j16", "-C", host_lua, "lua", "CC=cc",
    "MYCFLAGS=-DLUA_USE_POSIX -DLUA_USE_DLOPEN", "MYLIBS=-Wl,-E -ldl" })
  ctx.run({ "mkdir", "-p", ctx.build .. "/host" })
  local compile_host = { "cc", "-O2", "-shared", "-fPIC", "-DNVIM_NLUA0",
    "-I" .. host_lua, "-I" .. ctx.source .. "/src", "-I" .. host_lpeg,
    "-o", host_module, ctx.source .. "/src/nlua0.c", ctx.source .. "/src/bit.c" }
  for _, name in ipairs({ "conv", "lmpack", "mpack_core", "object", "rpc" }) do
    table.insert(compile_host, ctx.source .. "/src/mpack/" .. name .. ".c")
  end
  for _, name in ipairs({ "lpcap", "lpcode", "lpcset", "lpprint", "lptree", "lpvm" }) do
    table.insert(compile_host, host_lpeg .. "/" .. name .. ".c")
  end
  ctx.run(compile_host)

  ctx.run({ "cmake", "-S", ctx.source, "-B", ctx.build, "-G", "Unix Makefiles",
    "-DCMAKE_TOOLCHAIN_FILE=" .. ctx.sdk .. "/share/pyxis.cmake",
    "-DPYXIS_CROSS_COMPILE=" .. ctx.cross_compile,
    "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DCMAKE_C_FLAGS=-ffreestanding -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -march=x86-64",
    "-DCMAKE_FIND_ROOT_PATH=" .. ctx.lua51 .. ";" .. ctx.libuv .. ";" ..
      ctx.utf8proc .. ";" .. ctx.tree_sitter,
    "-DLUA_INCLUDE_DIR=" .. ctx.lua51 .. "/include/lua5.1",
    "-DLUA_LIBRARY=" .. ctx.lua51 .. "/lib/liblua5.1.a",
    "-DLUV_INCLUDE_DIR=" .. ctx.lua51 .. "/include",
    "-DLUV_LIBRARY=" .. ctx.lua51 .. "/lib/libluv.a",
    "-DLPEG_LIBRARY=" .. ctx.lua51 .. "/lib/liblpeg.a",
    "-DLIBUV_INCLUDE_DIR=" .. ctx.libuv .. "/include",
    "-DLIBUV_LIBRARY=" .. ctx.libuv .. "/lib/libuv.a",
    "-DUTF8PROC_INCLUDE_DIR=" .. ctx.utf8proc .. "/include",
    "-DUTF8PROC_LIBRARY=" .. ctx.utf8proc .. "/lib/libutf8proc.a",
    "-DTREESITTER_INCLUDE_DIR=" .. ctx.tree_sitter .. "/include",
    "-DTREESITTER_LIBRARY=" .. ctx.tree_sitter .. "/lib/libtree-sitter.a",
    "-DPYXIS_TREESITTER_C_SOURCE_DIR=" .. assert(ctx.extra.tree_sitter_c),
    "-DPYXIS_TREESITTER_LUA_SOURCE_DIR=" .. assert(ctx.extra.tree_sitter_lua),
    "-DPYXIS_TREESITTER_VIM_SOURCE_DIR=" .. assert(ctx.extra.tree_sitter_vim),
    "-DPYXIS_TREESITTER_VIMDOC_SOURCE_DIR=" .. assert(ctx.extra.tree_sitter_vimdoc),
    "-DPYXIS_TREESITTER_QUERY_SOURCE_DIR=" .. assert(ctx.extra.tree_sitter_query),
    "-DPYXIS_TREESITTER_MARKDOWN_SOURCE_DIR=" .. assert(ctx.extra.tree_sitter_markdown),
    "-DICONV_INCLUDE_DIR=" .. ctx.sysroot .. "/usr/include", "-DICONV_LIBRARY=",
    "-DLUA_PRG=" .. lua, "-DLUA_GEN_PRG=" .. lua, "-DNLUA0_HOST_PRG=" .. host_module,
    "-DPREFER_LUA=ON", "-DCOMPILE_LUA=OFF", "-DENABLE_LTO=OFF",
    "-DENABLE_LIBINTL=OFF", "-DENABLE_TRANSLATIONS=OFF", "-DENABLE_UNIBILIUM=OFF",
    "-DENABLE_WASMTIME=OFF", "-DENABLE_COMPILER_SUGGESTIONS=OFF",
    "-DFETCHCONTENT_FULLY_DISCONNECTED=ON" })
  ctx.run({ "cmake", "--build", ctx.build, "--target", "nvim_bin", "--parallel", "16" })

  local runtime = ctx.stage .. "/bin/nvim.pxb/app/share/nvim/runtime"
  ctx.run({ "mkdir", "-p", runtime .. "/syntax/vim" })
  ctx.run({ "cp", "-R", ctx.source .. "/runtime/.", runtime })
  -- Run upstream generators with native host Lua, without executing the editor.
  ctx.run({ lua, ctx.source .. "/src/gen/preload_nlua.lua", ctx.source, host_module,
    ctx.build, ctx.source .. "/src/gen/gen_vimvim.lua", runtime .. "/syntax/vim/generated.vim",
    ctx.build .. "/funcs_data.mpack", ctx.source .. "/src/nvim/options.lua",
    ctx.source .. "/src/nvim/auevents.lua", ctx.source .. "/src/nvim/ex_cmds.lua",
    ctx.source .. "/src/nvim/vvars.lua" })
  ctx.run({ lua, ctx.recipe .. "/host/helptags.lua", ctx.source,
    runtime .. "/doc/tags", runtime .. "/doc" })
  for _, name in ipairs({ "matchit", "netrw" }) do
    local doc = runtime .. "/pack/dist/opt/" .. name .. "/doc"
    ctx.run({ lua, ctx.recipe .. "/host/helptags.lua", ctx.source, doc .. "/tags", doc })
  end
  local outputs = ctx.metadata.outputs
  local function install(mode, source, target)
    ctx.run({ "install", "-D", "-m", mode, source, ctx.stage .. "/" .. target })
  end
  install("755", ctx.build .. "/bin/nvim.pxe", outputs.executable)
  install("644", ctx.recipe .. "/manifest.json", outputs.manifest)
  install("644", ctx.recipe .. "/runtime/sysinit.vim", outputs.sysinit)
  install("644", ctx.recipe .. "/runtime/colors/pyxis.vim", outputs.colorscheme)
  install("644", ctx.source .. "/LICENSE.txt", outputs.license)
  install("644", ctx.recipe .. "/PORT-NOTICE.md", outputs.port_notice)
  install("644", ctx.source .. "/src/xdiff/COPYING", outputs.xdiff_license)
  install("644", ctx.source .. "/src/mpack/LICENSE-MIT", outputs.mpack_license)
  install("644", ctx.source .. "/src/nvim/vterm/LICENSE", outputs.vterm_license)
  local notices = assert(io.open(ctx.stage .. "/" .. outputs.vendor_notices, "w"))
  for _, path in ipairs({ "src/bit.c", "src/klib/kvec.h", "src/cjson/lua_cjson.c",
    "src/cjson/fpconv.c", "src/cjson/strbuf.h", "src/nvim/fuzzy.c",
    "src/nvim/marktree.c", "src/nvim/tui/terminfo.c" }) do
    local source = assert(io.open(ctx.source .. "/" .. path, "r"))
    local text = assert(source:read("a"))
    assert(source:close())
    local notice
    if path == "src/nvim/tui/terminfo.c" then
      notice = text:match("(// Copyright %(c%) 2009.-)\n// nvim modifications:")
    elseif path == "src/klib/kvec.h" then
      notice = text:match("^(.-)\n// An example:")
    elseif path == "src/nvim/fuzzy.c" or path == "src/nvim/marktree.c" then
      notice = text:match("^(.-)#include")
    else
      local blocks = {}
      for block in text:gmatch("/%*.-%*/") do
        if block:lower():find("copyright", 1, true) then
          table.insert(blocks, block)
        end
      end
      notice = next(blocks) and table.concat(blocks, "\n")
    end
    assert(notices:write(path, "\n", assert(notice), "\n\n"))
  end
  assert(notices:close())
  install("644", ctx.extra.host_lua .. "/COPYRIGHT", outputs.host_lua_license)
  install("644", host_lpeg .. "/lpeg.html", outputs.host_lpeg_license)
  install("644", ctx.extra.tree_sitter_c .. "/LICENSE", outputs.tree_sitter_c_license)
  install("644", ctx.extra.tree_sitter_lua .. "/LICENSE.md", outputs.tree_sitter_lua_license)
  install("644", ctx.extra.tree_sitter_vim .. "/LICENSE", outputs.tree_sitter_vim_license)
  install("644", ctx.extra.tree_sitter_vimdoc .. "/LICENSE", outputs.tree_sitter_vimdoc_license)
  install("644", ctx.extra.tree_sitter_query .. "/LICENSE", outputs.tree_sitter_query_license)
  install("644", ctx.extra.tree_sitter_markdown .. "/LICENSE", outputs.tree_sitter_markdown_license)
  for _, dependency in ipairs({
    { prefix = ctx.lua51, name = "lua51", notices = {
      "COPYRIGHT", "lpeg.html", "luv-LICENSE.txt", "compat53-LICENSE", "PORT-NOTICE.md" } },
    { prefix = ctx.libuv, name = "libuv", notices = { "LICENSE", "LICENSE-extra", "PORT-NOTICE" } },
    { prefix = ctx.utf8proc, name = "utf8proc", notices = { "LICENSE.md" } },
    { prefix = ctx.tree_sitter, name = "tree-sitter", notices = {
      "LICENSE", "unicode-LICENSE", "PORT-NOTICE" } },
  }) do
    local target = "bin/nvim.pxb/app/metadata/licenses/" .. dependency.name
    for _, name in ipairs(dependency.notices) do
      install("644", dependency.prefix .. "/share/licenses/" .. dependency.name .. "/" .. name,
        target .. "/" .. name)
    end
    install("644", dependency.prefix .. "/share/" .. dependency.name .. "/source.txt",
      target .. "/source.txt")
  end
  install("644", ctx.recipe .. "/../LICENSE", outputs.pyxis_license)
  for _, name in ipairs({ "tlsf.h", "tlsf-upstream.md", "musl-COPYRIGHT",
    "musl-TRE-COPYRIGHT", "musl-upstream.md" }) do
    install("644", ctx.sdk .. "/share/licenses/" .. name,
      "bin/nvim.pxb/app/metadata/licenses/libc/" .. name)
  end
  for _, name in ipairs({ "LICENSE.TXT", "llvm-revision" }) do
    install("644", ctx.sdk .. "/share/toolchain/" .. name,
      "bin/nvim.pxb/app/metadata/licenses/toolchain/" .. name)
  end
  local provenance = assert(io.open(ctx.stage .. "/" .. outputs.provenance, "w"))
  assert(provenance:write("source=" .. ctx.metadata.source.archive.url .. "\n",
    "commit=" .. ctx.metadata.source.commit .. "\n",
    "sha256=" .. ctx.metadata.source.archive.sha256 .. "\n",
    "version=" .. ctx.metadata.version .. "\n",
    "license=" .. ctx.metadata.license .. "\n",
    "patches=" .. table.concat(ctx.metadata.patches, " ") .. "\n",
    "profile=native terminal client/server, PUC Lua 5.1, synchronous filesystem, 16 colours\n",
    "static_parsers=c lua vim vimdoc query markdown markdown_inline\n",
    "omitted=workers, jobs, terminal emulation, LSP, watches, sockets, signals, dynamic modules, swap/backup recovery\n"))
  for _, entry in ipairs(ctx.metadata.source.extra) do
    assert(provenance:write(entry.name .. "=" .. entry.archive.url .. "\n",
      entry.name .. "_mirror=" .. entry.archive.mirror .. "\n",
      entry.name .. "_sha256=" .. entry.archive.sha256 .. "\n"))
    if entry.version then
      assert(provenance:write(entry.name .. "_version=" .. entry.version .. "\n",
        entry.name .. "_license=" .. entry.license .. "\n"))
    end
  end
  assert(provenance:close())
end
