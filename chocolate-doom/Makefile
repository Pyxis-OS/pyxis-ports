include $(SDK)/share/pyxis.mk

# Only the chocolate-doom target: no Heretic, Hexen, Strife, setup tool or
# server. SDL2_mixer supplies digital sound and OPL music output. SDL2_net,
# libsamplerate, libpng and FluidSynth are not selected.
.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" -Wno-dev \
	  -DCMAKE_TOOLCHAIN_FILE=$(PYXIS_SDK)/share/pyxis.cmake \
	  -DPYXIS_CROSS_COMPILE=$(CROSS_COMPILE) \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_FIND_ROOT_PATH="$(SDL2);$(SDL2_MIXER)" \
	  -DCMAKE_PREFIX_PATH="$(SDL2);$(SDL2_MIXER)" \
	  -DENABLE_SDL2_MIXER=ON -DENABLE_SDL2_NET=OFF
	$(MAKE) -C $(BUILD) chocolate-doom
