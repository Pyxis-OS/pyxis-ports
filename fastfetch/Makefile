include $(SDK)/share/pyxis.mk
# CMake writes its own dependency files; the SDK's -MMD -MP would duplicate
# them, and Clang reports the unused -MD that CMake adds.
CMAKE_PYXIS_CFLAGS := $(filter-out -MMD -MP,$(PYXIS_CFLAGS))

.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" \
	  -DCMAKE_TOOLCHAIN_FILE=$(RECIPE)/toolchain.cmake \
	  -DCMAKE_C_COMPILER=$(shell command -v $(CC)) \
	  -DCMAKE_AR=$(shell command -v $(AR)) \
	  -DCMAKE_SYSROOT=$(PYXIS_SYSROOT) -DPYXIS_SDK=$(PYXIS_SDK) \
	  -DCMAKE_C_FLAGS="$(PYXIS_CPPFLAGS) $(CMAKE_PYXIS_CFLAGS)" \
	  -DCMAKE_C_FLAGS_RELEASE="-DNDEBUG" \
	  -DCMAKE_C_FLAGS_RELWITHDEBINFO="-DNDEBUG" \
	  -DCMAKE_C_FLAGS_DEBUG="-O0 -g3" \
	  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	  -DCMAKE_EXE_LINKER_FLAGS="$(PYXIS_LDFLAGS)"
	$(MAKE) -C $(BUILD) -j16
