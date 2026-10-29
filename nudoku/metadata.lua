return {
  source = {
    url = "https://github.com/jubalh/nudoku",
    commit = "96638addc1b5ec45293ed122ffb6b68248851115",
    archive = {
      url = "https://github.com/jubalh/nudoku/archive/refs/tags/8.0.1.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/jubalh/nudoku/archive/refs/tags/8.0.1.tar.gz",
      sha256 = "4e8a35950b7b7ce1e49f9457a8aceffbd21fb2b34aa8386847a7a158a2cab551",
    },
  },
  version = "8.0.1",
  license = "GPL-3.0-or-later",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "git", "install" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {},
  outputs = {
    executable = "bin/nudoku.pxb/app/bin/nudoku.pxe",
    manifest = "bin/nudoku.pxb/manifest.json",
    license = "bin/nudoku.pxb/app/metadata/nudoku/LICENSE",
    source = "bin/nudoku.pxb/app/metadata/nudoku-source.tar",
    provenance = "bin/nudoku.pxb/app/metadata/source.txt",
  },
}
