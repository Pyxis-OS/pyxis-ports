local recipe = assert(debug.getinfo(1, "S").source:match("^@(.+)/metadata.lua$"))
local bundle = "bin/lua-language-server.pxb/"
local app = bundle .. "app/"
local outputs = {
  executable = app .. "bin/lua-language-server.pxe",
  manifest = bundle .. "manifest.json",
  runtime_manifest = app .. "metadata/runtime-files.txt",
  provenance = app .. "metadata/source.txt",
  notice = app .. "metadata/PORT-NOTICE",
  license = app .. "metadata/licenses/LuaLS-LICENSE",
  bee_license = app .. "metadata/licenses/bee-LICENSE",
  lua_license = app .. "metadata/licenses/Lua-LICENSE",
  parser_license = app .. "metadata/licenses/LPegLabel-LICENSE",
  json_license = app .. "metadata/licenses/json-LICENSE",
  serialization_license = app .. "metadata/licenses/lua-serialize-LICENSE",
  pyxis_license = app .. "metadata/licenses/MPL-2.0",
  licensing = app .. "metadata/licenses/LICENSING.md",
}
for index, path in ipairs({
  "libcxx-LICENSE.TXT",
  "libcxxabi-LICENSE.TXT",
  "libunwind-LICENSE.TXT",
  "compiler-rt-LICENSE.TXT",
  "compiler-rt-CREDITS.TXT",
  "libcxx-CREDITS.TXT",
  "libcxxabi-CREDITS.TXT",
  "runtime-SOURCE-NOTICES.txt",
  "lpeglabel-SOURCE-NOTICES.txt",
  "ORIGIN.md",
}) do
  outputs["runtime_notice_" .. index] = app .. "metadata/licenses/runtime/" .. path
end
for index, path in ipairs({ "musl-COPYRIGHT", "musl-TRE-COPYRIGHT",
    "musl-upstream.md", "tlsf.h", "tlsf-upstream.md", "musl-fmaf.c",
    "npfs/LICENSE", "npfs/LICENSING.md", "fat32/LICENSE", "fat32/LICENSING.md" }) do
  outputs["sdk_notice_" .. index] = app .. "metadata/licenses/sdk/" .. path
end
outputs.sdk_manifest = app .. "metadata/sdk-manifest.txt"
outputs.toolchain_license = app .. "metadata/licenses/sdk/toolchain-LICENSE"
outputs.toolchain_revision = app .. "metadata/toolchain-revision"
for index, path in ipairs(dofile(recipe .. "/runtime-files.lua")) do
  outputs["runtime_" .. index] = app .. "share/lua-language-server/" .. path
end
for index, path in ipairs({ "Makefile", "metadata.lua", "build.lua", "stage.py",
    "runtime-files.lua", "manifest.json", "README.md", "PORT-NOTICE",
    "pyxis/runtime.hpp", "pyxis/binding.hpp", "pyxis/main.cpp", "pyxis/filesystem.cpp",
    "pyxis/platform.cpp", "pyxis/concurrency.cpp", "pyxis/concurrency.md",
    "pyxis/lua-config.h", "pyxis/lua-core.cpp", "pyxis/lua-libraries.cpp",
    "patches/0001-native-workers.patch", "patches/0002-pyxis-profile.patch",
    "patches/0003-pyxis-lua-libraries.patch" }) do
  outputs["pyxis_" .. index] = app .. "metadata/pyxis/" .. path
end

return {
  source = {
    url = "https://github.com/LuaLS/lua-language-server.git",
    commit = "b144c872f081373a893b55979142fed651e4425a",
    archive = {
      url = "https://github.com/LuaLS/lua-language-server/archive/b144c872f081373a893b55979142fed651e4425a.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/LuaLS/lua-language-server/archive/b144c872f081373a893b55979142fed651e4425a.tar.gz",
      sha256 = "844ed9f36ea5cd9bd1310861fcae73b9859996aee7bf06f3b048805507a63a2c",
    },
    extra = {
      { name = "bee", url = "https://github.com/actboy168/bee.lua.git",
        commit = "f55b6988e8d9564816a555118b0e76564ef9a283",
        archive = {
          url = "https://github.com/actboy168/bee.lua/archive/f55b6988e8d9564816a555118b0e76564ef9a283.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/actboy168/bee.lua/archive/f55b6988e8d9564816a555118b0e76564ef9a283.tar.gz",
          sha256 = "61f25750910fce54f8e4dc1e3118e831a479bb1f654573e8cee57de4403bb2f4",
        }, patches = { "patches/0003-pyxis-lua-libraries.patch" } },
      { name = "lpeglabel", url = "https://github.com/sqmedeiros/lpeglabel.git",
        commit = "912b0b9e8641074408ffc2259e069b188e0c717b",
        archive = {
          url = "https://github.com/sqmedeiros/lpeglabel/archive/912b0b9e8641074408ffc2259e069b188e0c717b.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/sqmedeiros/lpeglabel/archive/912b0b9e8641074408ffc2259e069b188e0c717b.tar.gz",
          sha256 = "f30f8039b89a571bdf450d1b82727ddbb3905e58b205ddd0421fad50983b991f",
        } },
      { name = "json", url = "https://github.com/actboy168/json.lua.git",
        commit = "08095fd2cdfde8f49a5f5b54b35ec7b3d4906a36",
        archive = {
          url = "https://github.com/actboy168/json.lua/archive/08095fd2cdfde8f49a5f5b54b35ec7b3d4906a36.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/actboy168/json.lua/archive/08095fd2cdfde8f49a5f5b54b35ec7b3d4906a36.tar.gz",
          sha256 = "625fc92798e27d8c6597b59b1f8d014744496dc2e1c5117baa5e1a4d286c7b7e",
        } },
      { name = "serialize_notice", url = "https://github.com/cloudwu/lua-serialize.git",
        commit = "d523513c0612cf4442d97116038afd36cf1168be",
        archive = {
          url = "https://github.com/cloudwu/lua-serialize/archive/d523513c0612cf4442d97116038afd36cf1168be.tar.gz",
          mirror = "https://repo.internal/repository/raw-github/cloudwu/lua-serialize/archive/d523513c0612cf4442d97116038afd36cf1168be.tar.gz",
          sha256 = "cec6ecbf32852cc940839657a058f33cf7985fa2e84395b392a51442b4a81f45",
        } },
    },
  },
  version = "3.19.2",
  license = "MIT AND MPL-2.0",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "git", "python3", "install" },
    pyxis = { "libc", "libpyxis", "libc++", "libc++abi", "libunwind" },
  },
  patches = { "patches/0001-native-workers.patch", "patches/0002-pyxis-profile.patch" },
  outputs = outputs,
}
