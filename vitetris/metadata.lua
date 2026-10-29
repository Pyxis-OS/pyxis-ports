return {
  source = {
    url = "https://github.com/vicgeralds/vitetris",
    commit = "744b5090a6d1315cd7b818e5276ed90ba0f3df23",
    archive = {
      url = "https://github.com/vicgeralds/vitetris/archive/744b5090a6d1315cd7b818e5276ed90ba0f3df23.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/vicgeralds/vitetris/archive/744b5090a6d1315cd7b818e5276ed90ba0f3df23.tar.gz",
      sha256 = "ae8e0371ee3d6884f6d7420e8ff9ec11fcd013bb8fd7efb87b957181c0b6d58f",
    },
  },
  version = "0.59.1",
  license = "BSD-2-Clause",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "git", "install", "cp", "ar" },
    pyxis = { "libc", "libpyxis" },
  },
  patches = { "patches/0001-pyxis-home-path.patch" },
  outputs = {
    executable = "bin/vitetris.pxb/app/bin/vitetris.pxe",
    manifest = "bin/vitetris.pxb/manifest.json",
    license = "bin/vitetris.pxb/app/metadata/vitetris/LICENSE",
    source = "bin/vitetris.pxb/app/metadata/vitetris-source.tar",
    provenance = "bin/vitetris.pxb/app/metadata/source.txt",
  },
}
