return {
  source = {
    file = {
      url = "https://curl.se/ca/cacert-2026-10-20.pem",
      mirror = "https://repo.internal/repository/raw-curl/ca/cacert-2026-10-20.pem",
      sha256 = "796f1389bfea13c0ed78f663b765baaff54a508e5483786bd6ece73c5e3e3cbe",
      name = "cacert.pem",
    },
  },
  version = "2026-10-20",
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
