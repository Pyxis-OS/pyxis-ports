include $(SDK)/share/pyxis.mk
# CMake writes its own dependency files; the SDK's -MMD -MP would duplicate
# them, and Clang reports the unused -MD that CMake adds.
CMAKE_PYXIS_CXXFLAGS := $(filter-out -MMD -MP,$(PYXIS_CXXFLAGS))

# Only the formatting library: fmt's os.h file and process helpers need POSIX
# descriptors, so FMT_OS is off.
.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" \
	  -DCMAKE_TOOLCHAIN_FILE=$(RECIPE)/toolchain.cmake \
	  -DCMAKE_CXX_COMPILER=$(shell command -v $(CXX)) \
	  -DCMAKE_AR=$(shell command -v $(AR)) \
	  -DCMAKE_SYSROOT=$(PYXIS_SYSROOT) \
	  -DCMAKE_CXX_FLAGS="$(PYXIS_CXX_CPPFLAGS) $(CMAKE_PYXIS_CXXFLAGS)" \
	  -DBUILD_SHARED_LIBS=OFF -DFMT_OS=OFF \
	  -DFMT_DOC=OFF -DFMT_TEST=OFF -DFMT_INSTALL=OFF
	$(MAKE) -C $(BUILD)
