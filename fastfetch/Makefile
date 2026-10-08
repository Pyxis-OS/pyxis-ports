include $(SDK)/share/pyxis.mk
# CMake writes its own dependency files; the SDK's -MMD -MP would duplicate
# them, and Clang reports the unused -MD that CMake adds.
CMAKE_PYXIS_CFLAGS := $(filter-out -MMD -MP,$(PYXIS_CFLAGS))
# The link flags below replace the driver's startup and libraries, which the
# Pyxis patch adds to fastfetch itself; CMake's test programs cannot link with
# them, so its checks only compile.

.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" \
	  -DCMAKE_TOOLCHAIN_FILE=$(PYXIS_SDK)/share/pyxis.cmake \
	  -DPYXIS_CROSS_COMPILE=$(CROSS_COMPILE) \
	  -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
	  -DPYXIS_RUNTIME_LIBRARY=$(PYXIS_RUNTIME_LIBRARY) \
	  -DCMAKE_C_FLAGS="$(CMAKE_PYXIS_CFLAGS)" \
	  -DCMAKE_C_FLAGS_RELEASE="-DNDEBUG" \
	  -DCMAKE_C_FLAGS_RELWITHDEBINFO="-DNDEBUG" \
	  -DCMAKE_C_FLAGS_DEBUG="-O0 -g3" \
	  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	  -DCMAKE_EXE_LINKER_FLAGS="$(PYXIS_LDFLAGS)"
	$(MAKE) -C $(BUILD) -j16
