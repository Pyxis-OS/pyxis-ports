return {
  source = {
    url = "https://github.com/abishekvashok/cmatrix",
    commit = "adfdf1656f23e5ab3b52c7d7edf91249a4477e8d",
    archive = {
      url = "https://github.com/abishekvashok/cmatrix/archive/refs/tags/v2.0.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/abishekvashok/cmatrix/archive/refs/tags/v2.0.tar.gz",
      sha256 = "ad93ba39acd383696ab6a9ebbed1259ecf2d3cf9f49d6b97038c66f80749e99a",
    },
  },
  version = "2.0",
  license = "GPL-3.0-or-later",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "git", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = { "patches/0001-pyxis-terminal.patch" },
  outputs = {
    executable = "bin/cmatrix.pxb/app/bin/cmatrix.pxe",
    manifest = "bin/cmatrix.pxb/manifest.json",
    license = "bin/cmatrix.pxb/app/metadata/cmatrix/LICENSE",
    source = "bin/cmatrix.pxb/app/metadata/cmatrix-source.tar",
    provenance = "bin/cmatrix.pxb/app/metadata/source.txt",
  },
}
