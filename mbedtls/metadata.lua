return {
  source = {
    url = "https://github.com/Mbed-TLS/mbedtls.git",
    commit = "0a8fda272a5a0abef3b47c91bed37185d5a726b1",
    archive = {
      url = "https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-4.1.1/mbedtls-4.1.1.tar.bz2",
      mirror = "https://repo.internal/repository/raw-github/Mbed-TLS/mbedtls/releases/download/mbedtls-4.1.1/mbedtls-4.1.1.tar.bz2",
      sha256 = "3359a349e23db3d5536fcee032ae7b2ecbfc08972fab643089b5cbf2a375c98c",
    },
  },
  bundled_crypto = {
    version = "1.1.1",
    url = "https://github.com/Mbed-TLS/TF-PSA-Crypto.git",
    commit = "a0632be94d883daa0295ac1eabf70359ad94f91b",
  },
  license = "Apache-2.0",
  dependencies = {
    host = { "make", "cmake", "curl", "sha256sum", "tar", "bzip2" },
    pyxis = { "libc" },
  },
  patches = {},
  outputs = {
    tls_library = "dev/lib/libmbedtls.a",
    x509_library = "dev/lib/libmbedx509.a",
    crypto_library = "dev/lib/libtfpsacrypto.a",
    tls_header = "dev/include/mbedtls/ssl.h",
    crypto_header = "dev/include/psa/crypto.h",
    driver_header = "dev/include/mbedtls/private_access.h",
    tls_config = "dev/include/mbedtls/pyxis_tls_config.h",
    crypto_config = "dev/include/mbedtls/pyxis_crypto_config.h",
    make_config = "dev/share/mbedtls.mk",
    license = "share/licenses/mbedtls/LICENSE",
    crypto_license = "share/licenses/tf-psa-crypto/LICENSE",
    notice = "share/licenses/mbedtls/PORT-NOTICE",
  },
}
