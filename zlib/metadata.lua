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
    library = "dev/lib/libz.a",
    header = "dev/include/zlib.h",
    configuration = "dev/include/zconf.h",
    license = "share/licenses/zlib/LICENSE",
    notice = "share/licenses/zlib/PORT-NOTICE",
    provenance = "share/zlib/source.txt",
  },
}
