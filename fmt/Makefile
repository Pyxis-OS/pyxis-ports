include $(SDK)/share/pyxis.mk
# CMake writes its own dependency files; the SDK's -MMD -MP would duplicate
# them, and Clang reports the unused -MD that CMake adds.
CMAKE_PYXIS_CXXFLAGS := $(filter-out -MMD -MP,$(PYXIS_CXXFLAGS))

# Only the formatting library: fmt's os.h file and process helpers need POSIX
# descriptors, so FMT_OS is off.
.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" \
	  -DCMAKE_TOOLCHAIN_FILE=$(PYXIS_SDK)/share/pyxis.cmake \
	  -DPYXIS_CROSS_COMPILE=$(CROSS_COMPILE) \
	  -DCMAKE_CXX_FLAGS="$(CMAKE_PYXIS_CXXFLAGS)" \
	  -DBUILD_SHARED_LIBS=OFF -DFMT_OS=OFF \
	  -DFMT_DOC=OFF -DFMT_TEST=OFF -DFMT_INSTALL=ON \
	  -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_INSTALL_INCLUDEDIR=include
	$(MAKE) -C $(BUILD)
