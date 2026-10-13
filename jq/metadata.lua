return {
  source = {
    url = "https://github.com/jqlang/jq/releases/tag/jq-1.7.1",
    archive = {
      url = "https://github.com/jqlang/jq/releases/download/jq-1.7.1/jq-1.7.1.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/jqlang/jq/releases/download/jq-1.7.1/jq-1.7.1.tar.gz",
      sha256 = "478c9ca129fd2e3443fe27314b455e211e0d8c60bc8ff7df703873deeee580c2",
    },
  },
  version = "1.7.1",
  -- COPYING covers jq (MIT), David Gay's dtoa (a permissive Lucent notice, as
  -- LicenseRef-Gay-dtoa) and decNumber (ICU). Its BSD-3-Clause notice belongs
  -- to Windows-only code that this build does not compile.
  license = "MIT AND ICU AND BSD-3-Clause AND LicenseRef-Gay-dtoa",
  dependencies = {
    host = { "make", "curl", "sha256sum", "tar", "gzip", "sed", "install" },
    pyxis = { "libc" },
  },
  patches = {
    "patches/0001-environment-snapshot.patch",
    "patches/0002-zero-size-allocations.patch",
    "patches/0003-buffer-stdout.patch",
  },
  outputs = {
    executable = "bin/jq.pxe",
    license = "share/licenses/jq/COPYING",
    notice = "share/licenses/jq/PORT-NOTICE",
    provenance = "share/jq/source.txt",
  },
}
