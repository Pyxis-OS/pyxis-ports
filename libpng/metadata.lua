return {
  source = {
    url = "https://www.libpng.org/pub/png/libpng.html",
    archive = {
      url = "https://prdownloads.sourceforge.net/libpng/libpng-1.6.59.tar.gz",
      mirror = "https://repo.internal/repository/raw-sourceforge/libpng/libpng-1.6.59.tar.gz",
      sha256 = "86a3e4b501f7f50e392c4e456ea158893e9a595b1c63eb88c7bb9f6cf8772dad",
    },
  },
  version = "1.6.59",
  license = "libpng-2.0",
  dependencies = {
    host = { "make", "awk", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    library = "dev/lib/libpng.a",
    header = "dev/include/png.h",
    platform = "dev/include/pngconf.h",
    configuration = "dev/include/pnglibconf.h",
    license = "share/licenses/libpng/LICENSE",
    notice = "share/licenses/libpng/PORT-NOTICE",
    provenance = "share/libpng/source.txt",
  },
}
