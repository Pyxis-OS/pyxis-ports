include $(SDK)/share/pyxis.mk

# The 0001 patch removes SDL2_net, the UDP driver and the Vorbis, MP3 and FLAC
# codecs from the CMake project, so the program builds against SDL2 alone.
# SDL2 is the Pyxis development library.
.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" -Wno-dev \
	  -DCMAKE_TOOLCHAIN_FILE=$(PYXIS_SDK)/share/pyxis.cmake \
	  -DPYXIS_CROSS_COMPILE=$(CROSS_COMPILE) \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_FIND_ROOT_PATH=$(SDL2)
	$(MAKE) -C $(BUILD) chocolate-quake
