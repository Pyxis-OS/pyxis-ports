local recipe = debug.getinfo(1, "S").source:sub(2):match("^(.*)/")
local outputs = {
  pkgconfig = "dev/lib/pkgconfig/wpa-rsn.pc",
  library = "dev/lib/libwpa-rsn.a",
  header = "dev/include/wpa-rsn/wpa-rsn.h",
  config = "dev/include/wpa-rsn/wpa-rsn-config.h",
  makefile = "dev/share/wpa-rsn.mk",
  license = "share/licenses/wpa-rsn/README",
  copying = "share/licenses/wpa-rsn/COPYING",
  notice = "share/licenses/wpa-rsn/PORT-NOTICE",
  upstream_notices = "share/licenses/wpa-rsn/SOURCE-NOTICES",
  provenance = "share/wpa-rsn/source.txt",
  dev_license = "dev/share/licenses/wpa-rsn/README",
  dev_copying = "dev/share/licenses/wpa-rsn/COPYING",
  dev_notice = "dev/share/licenses/wpa-rsn/PORT-NOTICE",
  dev_upstream_notices = "dev/share/licenses/wpa-rsn/SOURCE-NOTICES",
  dev_provenance = "dev/share/wpa-rsn/source.txt",
}
for _, header in ipairs(dofile(recipe .. "/headers.lua")) do
  outputs["header_" .. header:gsub("[^%w]", "_")] = "dev/include/wpa-rsn/" .. header
end
return {
  source = {
    url = "https://w1.fi/wpa_supplicant/",
    archive = {
      url = "https://w1.fi/releases/wpa_supplicant-2.12.tar.gz",
      mirror = "https://repo.internal/repository/raw-w1/releases/wpa_supplicant-2.12.tar.gz",
      sha256 = "08e23937e16d0155e55cab2b51f51fbe10d80a1aa91c4e15442645059b737ef6",
    },
  },
  version = "2.12",
  license = "BSD-3-Clause with public-domain internal crypto portions",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install", "patch" },
    pyxis = { "libc" },
  },
  patches = {
    "patches/0001-pyxis-native-library.patch",
    "patches/0002-native-secret-erasure.patch",
  },
  outputs = outputs,
}
