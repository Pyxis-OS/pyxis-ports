include $(SDK)/share/pyxis.mk
# CMake writes its own dependency files; the SDK's -MMD -MP would duplicate
# them, and Clang reports the unused -MD that CMake adds.
CMAKE_PYXIS_CFLAGS := $(filter-out -MMD -MP,$(PYXIS_CFLAGS))

.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" \
	  -DCMAKE_TOOLCHAIN_FILE=$(PYXIS_SDK)/share/pyxis.cmake \
	  -DPYXIS_CROSS_COMPILE=$(CROSS_COMPILE) \
	  -DCMAKE_C_FLAGS="$(CMAKE_PYXIS_CFLAGS)" \
	  -DMBEDTLS_USER_CONFIG_FILE=$(RECIPE)/pyxis_tls_config.h \
	  -DTF_PSA_CRYPTO_USER_CONFIG_FILE=$(RECIPE)/pyxis_crypto_config.h \
	  -DENABLE_PROGRAMS=OFF -DENABLE_TESTING=OFF -DGEN_FILES=OFF \
	  -DUSE_STATIC_MBEDTLS_LIBRARY=ON -DUSE_SHARED_MBEDTLS_LIBRARY=OFF
	$(MAKE) -C $(BUILD) -j16
