return {
  source = {
    file = {
      url = "https://curl.se/ca/cacert-2026-09-25.pem",
      mirror = "https://repo.internal/repository/raw-curl/ca/cacert-2026-09-25.pem",
      sha256 = "a41b5d356aea97a529fe27e0f7316d2f9d946d75927476cf9cf1b90637d00505",
      name = "cacert.pem",
    },
  },
  version = "2026-09-25",
  license = "MPL-2.0",
  dependencies = {
    host = { "curl", "sha256sum", "install" },
    pyxis = {},
  },
  patches = {},
  outputs = {
    bundle = "share/ca-certificates/cacert.pem",
    checksum = "share/ca-certificates/cacert.pem.sha256",
    provenance = "share/ca-certificates/source.txt",
    license = "share/licenses/ca-certificates/LICENSE",
    notice = "share/licenses/ca-certificates/NOTICE",
  },
}
