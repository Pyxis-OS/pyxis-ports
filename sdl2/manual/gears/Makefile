MANUAL_SOURCE := $(dir $(lastword $(MAKEFILE_LIST)))main.c
include $(SDK)/share/pyxis.mk

BUILD ?= build/manual-sdl-gears
SDL2_STAGE ?= build/sdl2/stage
MESA_STAGE ?= build/mesa/stage/dev

.PHONY: all
all: $(BUILD)/sdl-gears.pxe

$(BUILD)/sdl-gears.pxe: $(MANUAL_SOURCE) $(SDL2_STAGE)/gl-dev/lib/libSDL2-osmesa.a $(MESA_STAGE)/lib/libOSMesa.a
	mkdir -p $(BUILD)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SDL2_STAGE)/dev/include/SDL2 \
	  -I$(MESA_STAGE)/include -c $< -o $(BUILD)/gears.o
	$(CXX) $(PYXIS_LDFLAGS) $(PYXIS_START) $(BUILD)/gears.o \
	  $(SDL2_STAGE)/gl-dev/lib/libSDL2-osmesa.a $(MESA_STAGE)/lib/libOSMesa.a \
	  $(PYXIS_CXX_LDLIBS) -o $@

-include $(BUILD)/gears.d
