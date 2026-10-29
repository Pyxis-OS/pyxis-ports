return {
  source = {
    url = "https://gitlab.com/DavidGriffith/frotz",
    commit = "acf205585a9472d27c07c0fe62da4b8bc89d1ec7",
    archive = {
      url = "https://gitlab.com/DavidGriffith/frotz/-/archive/2.55/frotz-2.55.tar.gz",
      mirror = "https://repo.internal/repository/raw-gitlab/DavidGriffith/frotz/-/archive/2.55/frotz-2.55.tar.gz",
      sha256 = "a8c4c4d79a2aa9b39e0efbbd3e0803f1dc5ef36f75f2c2945e295d190575eb09",
    },
  },
  version = "2.55",
  license = "GPL-2.0-or-later; dumb frontend permissive notices retained",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "install", "cp" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = {},
  outputs = {
    executable = "bin/dfrotz.pxb/app/bin/dfrotz.pxe",
    manifest = "bin/dfrotz.pxb/manifest.json",
    license = "bin/dfrotz.pxb/app/metadata/frotz/COPYING",
    source = "bin/dfrotz.pxb/app/metadata/frotz-source.tar",
    provenance = "bin/dfrotz.pxb/app/metadata/source.txt",
  },
}
