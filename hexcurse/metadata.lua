return {
  source = {
    url = "https://github.com/LonnyGomes/hexcurse",
    commit = "3ce605db4276310297f1cf871a38365650099f5d",
    archive = {
      url = "https://github.com/LonnyGomes/hexcurse/archive/refs/tags/v1.60.0.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/LonnyGomes/hexcurse/archive/refs/tags/v1.60.0.tar.gz",
      sha256 = "f6919e4a824ee354f003f0c42e4c4cef98a93aa7e3aa449caedd13f9a2db5530",
    },
  },
  version = "1.60.0",
  license = "GPL-2.0-or-later",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "git", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = { "patches/0001-pyxis-terminal-and-file-saves.patch" },
  outputs = {
    executable = "bin/hexcurse.pxb/app/bin/hexcurse.pxe",
    manifest = "bin/hexcurse.pxb/manifest.json",
    license = "bin/hexcurse.pxb/app/metadata/hexcurse/COPYING",
    source = "bin/hexcurse.pxb/app/metadata/hexcurse-source.tar",
    provenance = "bin/hexcurse.pxb/app/metadata/source.txt",
  },
}
