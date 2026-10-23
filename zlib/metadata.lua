return {
  source = {
    url = "https://zlib.net/",
    archive = {
      url = "https://zlib.net/zlib-1.3.2.tar.gz",
      mirror = "https://repo.internal/repository/raw-zlib/zlib-1.3.2.tar.gz",
      sha256 = "bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16",
    },
  },
  version = "1.3.2",
  license = "Zlib",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    dev_pyxis_license = "dev/share/licenses/zlib/MPL-2.0",
    dev_licensing = "dev/share/licenses/zlib/LICENSING.md",
    pc_zlib = "dev/lib/pkgconfig/zlib.pc",

    library = "dev/lib/libz.a",
    header = "dev/include/zlib.h",
    configuration = "dev/include/zconf.h",
    license = "share/licenses/zlib/LICENSE",
    notice = "share/licenses/zlib/PORT-NOTICE",
    provenance = "share/zlib/source.txt",
    dev_license = "dev/share/licenses/zlib/LICENSE",
    dev_notice = "dev/share/licenses/zlib/PORT-NOTICE",
    dev_provenance = "dev/share/zlib/source.txt",
  },
}
