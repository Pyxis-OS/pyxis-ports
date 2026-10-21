#ifndef PYXIS_WPA_RSN_CONFIG_H
#define PYXIS_WPA_RSN_CONFIG_H

/* Shared by the library and its native consumer: these affect upstream headers. */
#define CONFIG_NO_TKIP
#define CONFIG_NO_RC4
#define CONFIG_NO_STDOUT_DEBUG
#define CONFIG_NO_WPA_MSG
#define CONFIG_NO_HOSTAPD_LOGGER
#define CONFIG_NO_RANDOM_POOL
#define CONFIG_NO_CONFIG_WRITE
#define CONFIG_NO_CONFIG_BLOBS
#define CONFIG_NO_ROAMING
#define CONFIG_NO_SCAN_PROCESSING
#define CONFIG_CRYPTO_INTERNAL
#define CONFIG_INTERNAL_AES
#define CONFIG_INTERNAL_SHA1
#define CONFIG_INTERNAL_SHA256
#define CONFIG_SHA256

#define __BYTE_ORDER __BYTE_ORDER__
#define __LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#define __BIG_ENDIAN __ORDER_BIG_ENDIAN__
#define bswap_16(value) __builtin_bswap16(value)
#define bswap_32(value) __builtin_bswap32(value)
#define bswap_64(value) __builtin_bswap64(value)

#endif
