include $(SDK)/share/pyxis.mk

.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" \
	  -DCMAKE_TOOLCHAIN_FILE=$(RECIPE)/toolchain.cmake \
	  -DCMAKE_C_COMPILER=$(shell command -v $(CC)) \
	  -DCMAKE_AR=$(shell command -v $(AR)) \
	  -DCMAKE_SYSROOT=$(PYXIS_SYSROOT) \
	  -DCMAKE_C_FLAGS="$(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS)" \
	  -DMBEDTLS_USER_CONFIG_FILE=$(RECIPE)/pyxis_tls_config.h \
	  -DTF_PSA_CRYPTO_USER_CONFIG_FILE=$(RECIPE)/pyxis_crypto_config.h \
	  -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DGEN_FILES=OFF \
	  -DUSE_STATIC_MBEDTLS_LIBRARY=ON -DUSE_SHARED_MBEDTLS_LIBRARY=OFF
	$(MAKE) -C $(BUILD) -j16
