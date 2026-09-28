# Include this for every translation unit consuming these configured headers.
MBEDTLS_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
MBEDTLS_CPPFLAGS := -I$(MBEDTLS_ROOT)/include \
  -DMBEDTLS_USER_CONFIG_FILE='"mbedtls/pyxis_tls_config.h"' \
  -DTF_PSA_CRYPTO_USER_CONFIG_FILE='"mbedtls/pyxis_crypto_config.h"'
MBEDTLS_LIBRARIES := $(MBEDTLS_ROOT)/lib/libmbedtls.a \
  $(MBEDTLS_ROOT)/lib/libmbedx509.a $(MBEDTLS_ROOT)/lib/libtfpsacrypto.a
