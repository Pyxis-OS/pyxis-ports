include $(SDK)/share/pyxis.mk

S := $(SOURCE)
INCLUDE := $(BUILD)/include/SDL2

# Each upstream directory is either used whole or only its dummy backend.
UPSTREAM_SOURCES := $(wildcard $(S)/src/*.c) \
  $(wildcard $(S)/src/atomic/*.c) \
  $(wildcard $(S)/src/audio/*.c) \
  $(wildcard $(S)/src/cpuinfo/*.c) \
  $(wildcard $(S)/src/dynapi/*.c) \
  $(wildcard $(S)/src/events/*.c) \
  $(wildcard $(S)/src/file/*.c) \
  $(wildcard $(S)/src/haptic/*.c) $(S)/src/haptic/dummy/SDL_syshaptic.c \
  $(wildcard $(S)/src/hidapi/*.c) \
  $(wildcard $(S)/src/joystick/*.c) $(S)/src/joystick/dummy/SDL_sysjoystick.c \
  $(wildcard $(S)/src/libm/*.c) \
  $(S)/src/loadso/dummy/SDL_sysloadso.c \
  $(wildcard $(S)/src/locale/*.c) $(S)/src/locale/dummy/SDL_syslocale.c \
  $(wildcard $(S)/src/misc/*.c) $(S)/src/misc/dummy/SDL_sysurl.c \
  $(wildcard $(S)/src/power/*.c) \
  $(wildcard $(S)/src/render/*.c) $(wildcard $(S)/src/render/software/*.c) \
  $(wildcard $(S)/src/sensor/*.c) $(S)/src/sensor/dummy/SDL_dummysensor.c \
  $(wildcard $(S)/src/stdlib/*.c) \
  $(wildcard $(S)/src/thread/*.c) $(wildcard $(S)/src/thread/generic/*.c) \
  $(wildcard $(S)/src/timer/*.c) \
  $(wildcard $(S)/src/video/*.c) $(wildcard $(S)/src/video/yuv2rgb/*.c)
BACKEND_SOURCES := $(wildcard $(RECIPE)/pyxis/*.c)

UPSTREAM_OBJECTS := $(patsubst $(S)/%.c,$(BUILD)/upstream/%.o,$(UPSTREAM_SOURCES))
BACKEND_OBJECTS := $(patsubst $(RECIPE)/pyxis/%.c,$(BUILD)/pyxis/%.o,$(BACKEND_SOURCES))
OBJECTS := $(UPSTREAM_OBJECTS) $(BACKEND_OBJECTS)

# The installed tree replaces upstream's platform-dispatching SDL_config.h, so
# the library and its consumers use one configuration.
PUBLIC_HEADERS := $(filter-out $(S)/include/SDL_config%,$(wildcard $(S)/include/*.h))
HEADER_STAMP := $(BUILD)/include.stamp

CPPFLAGS := -I$(INCLUDE) -I$(S)/src $(PYXIS_CPPFLAGS)
# Upstream builds with -Wall, without strict aliasing and with the compiler's
# default C standard; its warnings stay visible. The backend uses SDK C23.
UPSTREAM_CFLAGS := $(filter-out -std=gnu23 -Wextra,$(PYXIS_CFLAGS)) -std=gnu17 -fno-strict-aliasing
BACKEND_CFLAGS := $(PYXIS_CFLAGS) -fno-strict-aliasing

.PHONY: all
all: $(BUILD)/libSDL2.a

$(HEADER_STAMP): $(PUBLIC_HEADERS) $(RECIPE)/SDL_config.h $(RECIPE)/Makefile
	rm -rf $(INCLUDE)
	mkdir -p $(INCLUDE)
	cp $(PUBLIC_HEADERS) $(INCLUDE)/
	cp $(RECIPE)/SDL_config.h $(INCLUDE)/SDL_config.h
	touch $@

$(BUILD)/upstream/%.o: $(S)/%.c $(HEADER_STAMP)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(UPSTREAM_CFLAGS) -c $< -o $@

$(BUILD)/pyxis/%.o: $(RECIPE)/pyxis/%.c $(HEADER_STAMP)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(BACKEND_CFLAGS) -c $< -o $@

$(BUILD)/libSDL2.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(OBJECTS)

-include $(OBJECTS:.o=.d)
