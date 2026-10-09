MANUAL_SOURCE := $(dir $(lastword $(MAKEFILE_LIST)))main.c

include $(SDK)/share/pyxis.mk

BUILD ?= build/manual-sdl-clipboard
SDL2_STAGE ?= build/sdl2/stage

.PHONY: all
all: $(BUILD)/sdl-clipboard.pxe

$(BUILD)/sdl-clipboard.pxe: $(MANUAL_SOURCE) $(SDL2_STAGE)/dev/lib/libSDL2.a
	mkdir -p $(BUILD)
	$(CC) $(PYXIS_CPPFLAGS) -I$(SDL2_STAGE)/dev/include/SDL2 $(PYXIS_CFLAGS) \
	  $(PYXIS_LDFLAGS) $(PYXIS_START) $< $(SDL2_STAGE)/dev/lib/libSDL2.a \
	  $(PYXIS_LDLIBS) -o $@

-include $(BUILD)/sdl-clipboard.d
