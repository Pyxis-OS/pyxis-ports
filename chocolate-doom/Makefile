include $(SDK)/share/pyxis.mk

# Only the chocolate-doom target: no Heretic, Hexen, Strife, setup tool or
# server. SDL2_mixer and SDL2_net are upstream options, so the program builds
# without sound, music or network play. SDL2 is the Pyxis development library;
# libsamplerate, libpng and FluidSynth are not offered, so CMake leaves them out.
.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" -Wno-dev \
	  -DCMAKE_TOOLCHAIN_FILE=$(PYXIS_SDK)/share/pyxis.cmake \
	  -DPYXIS_CROSS_COMPILE=$(CROSS_COMPILE) \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_FIND_ROOT_PATH=$(SDL2) \
	  -DENABLE_SDL2_MIXER=OFF -DENABLE_SDL2_NET=OFF
	$(MAKE) -C $(BUILD) chocolate-doom
