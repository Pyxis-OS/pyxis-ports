MANUAL_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(SDK)/share/pyxis.mk

BUILD ?= build/manual-sdl-gamepad
SDL2_STAGE ?= build/sdl2/stage
MESA_STAGE ?= build/mesa/stage/dev
PROFILE ?= ordinary
PROFILE_BUILD := $(BUILD)/$(PROFILE)

ifeq ($(PROFILE),ordinary)
SDL_ARCHIVE := $(SDL2_STAGE)/dev/lib/libSDL2.a
LINK := $(CC)
PROFILE_LIBS := $(PYXIS_LDLIBS)
else ifeq ($(PROFILE),osmesa)
SDL_ARCHIVE := $(SDL2_STAGE)/gl-dev/lib/libSDL2-osmesa.a
MESA_ARCHIVE := $(MESA_STAGE)/lib/libOSMesa.a
LINK := $(CXX)
PROFILE_LIBS := $(PYXIS_CXX_LDLIBS)
else
$(error PROFILE must be ordinary or osmesa)
endif

.PHONY: all
all: $(PROFILE_BUILD)/app/bin/sdl-gamepad.pxe $(PROFILE_BUILD)/manifest.json

$(PROFILE_BUILD)/main.o: $(MANUAL_DIR)main.c
	mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SDL2_STAGE)/dev/include/SDL2 \
	  -DSDL_VIEWER_PROFILE='"$(PROFILE)"' -c $< -o $@

$(PROFILE_BUILD)/app/bin/sdl-gamepad.pxe: $(PROFILE_BUILD)/main.o $(SDL_ARCHIVE) $(MESA_ARCHIVE)
	mkdir -p $(@D)
	$(LINK) $(PYXIS_LDFLAGS) $(PYXIS_START) $< $(SDL_ARCHIVE) $(MESA_ARCHIVE) \
	  $(PROFILE_LIBS) -o $@

$(PROFILE_BUILD)/manifest.json: $(MANUAL_DIR)manifest.json
	mkdir -p $(@D)
	cp $< $@

-include $(PROFILE_BUILD)/main.d
