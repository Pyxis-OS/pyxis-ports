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
	  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF \
	  -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_PNG=ON \
	  -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BROTLI=ON \
	  -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_INCLUDEDIR=include
	$(MAKE) -C $(BUILD) -j16
