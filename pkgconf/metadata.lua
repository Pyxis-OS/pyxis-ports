return {
  source = {
    url = "https://github.com/pkgconf/pkgconf/releases/tag/pkgconf-3.0.10",
    archive = {
      url = "https://github.com/pkgconf/pkgconf/archive/refs/tags/pkgconf-3.0.10.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/pkgconf/pkgconf/archive/refs/tags/pkgconf-3.0.10.tar.gz",
      sha256 = "11818a25f2730132f3b258ac8be4e82f83b9db03a1a99a6c29cd01b94fc9d310",
    },
  },
  version = "3.0.10",
  license = "ISC AND BSD-4-Clause AND BSD-2-Clause AND LicenseRef-scancode-sudo",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install" },
    pyxis = { "libc" },
  },
  patches = { "patches/0001-native-pyxis-paths-and-static-default.patch" },
  outputs = {
    executable = "bin/pkgconf.pxe",
    alias = "bin/pkg-config.pxe",
    license = "share/licenses/pkgconf/COPYING",
    authors = "share/licenses/pkgconf/AUTHORS",
    getopt_source = "share/licenses/pkgconf/getopt_long.c",
    getopt_header = "share/licenses/pkgconf/getopt_long.h",
    notice = "share/licenses/pkgconf/PORT-NOTICE",
    provenance = "share/pkgconf/source.txt",
  },
}
