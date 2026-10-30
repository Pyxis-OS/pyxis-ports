include $(SDK)/share/pyxis.mk

S := $(SOURCE)

# Each upstream directory is used whole, or only its dummy or selected backend
# files. Renderer and driver sources compile only under their configuration.
UPSTREAM_SOURCES := $(wildcard $(S)/src/*.c) \
  $(wildcard $(S)/src/atomic/*.c) \
  $(wildcard $(S)/src/audio/*.c) \
  $(wildcard $(S)/src/camera/*.c) $(S)/src/camera/dummy/SDL_camera_dummy.c \
  $(wildcard $(S)/src/core/*.c) \
  $(wildcard $(S)/src/cpuinfo/*.c) \
  $(wildcard $(S)/src/dialog/*.c) $(S)/src/dialog/dummy/SDL_dummydialog.c \
  $(wildcard $(S)/src/dynapi/*.c) \
  $(wildcard $(S)/src/events/*.c) \
  $(wildcard $(S)/src/filesystem/*.c) \
  $(S)/src/filesystem/posix/SDL_sysfsops.c \
  $(wildcard $(S)/src/gpu/*.c) \
  $(wildcard $(S)/src/haptic/*.c) $(S)/src/haptic/dummy/SDL_syshaptic.c \
  $(wildcard $(S)/src/hidapi/*.c) \
  $(wildcard $(S)/src/io/*.c) $(wildcard $(S)/src/io/generic/*.c) \
  $(wildcard $(S)/src/joystick/*.c) $(S)/src/joystick/dummy/SDL_sysjoystick.c \
  $(wildcard $(S)/src/libm/*.c) \
  $(S)/src/loadso/dummy/SDL_sysloadso.c \
  $(wildcard $(S)/src/locale/*.c) $(S)/src/locale/dummy/SDL_syslocale.c \
  $(wildcard $(S)/src/main/*.c) $(S)/src/main/generic/SDL_sysmain_callbacks.c \
  $(wildcard $(S)/src/misc/*.c) $(S)/src/misc/dummy/SDL_sysurl.c \
  $(wildcard $(S)/src/power/*.c) \
  $(wildcard $(S)/src/process/*.c) $(S)/src/process/dummy/SDL_dummyprocess.c \
  $(wildcard $(S)/src/render/*.c) $(wildcard $(S)/src/render/*/*.c) \
  $(wildcard $(S)/src/sensor/*.c) $(S)/src/sensor/dummy/SDL_dummysensor.c \
  $(wildcard $(S)/src/stdlib/*.c) \
  $(wildcard $(S)/src/storage/*.c) $(S)/src/storage/generic/SDL_genericstorage.c \
  $(wildcard $(S)/src/thread/*.c) $(S)/src/thread/generic/SDL_syssem.c \
  $(S)/src/thread/pthread/SDL_syscond.c $(S)/src/thread/pthread/SDL_sysmutex.c \
  $(S)/src/thread/pthread/SDL_sysrwlock.c $(S)/src/thread/pthread/SDL_systls.c \
  $(wildcard $(S)/src/time/*.c) $(S)/src/time/unix/SDL_systime.c \
  $(wildcard $(S)/src/timer/*.c) $(S)/src/timer/unix/SDL_systimer.c \
  $(wildcard $(S)/src/tray/*.c) $(S)/src/tray/dummy/SDL_tray.c \
  $(wildcard $(S)/src/video/*.c) $(wildcard $(S)/src/video/yuv2rgb/*.c)
BACKEND_SOURCES := $(filter-out $(RECIPE)/pyxis/SDL_pyxisopengl.c,$(wildcard $(RECIPE)/pyxis/*.c))
GL_BACKEND_SOURCES := $(BACKEND_SOURCES) $(RECIPE)/pyxis/SDL_pyxisopengl.c

UPSTREAM_OBJECTS := $(patsubst $(S)/%.c,$(BUILD)/upstream/%.o,$(UPSTREAM_SOURCES))
BACKEND_OBJECTS := $(patsubst $(RECIPE)/pyxis/%.c,$(BUILD)/pyxis/%.o,$(BACKEND_SOURCES))
OBJECTS := $(UPSTREAM_OBJECTS) $(BACKEND_OBJECTS)
GL_BACKEND_OBJECTS := $(patsubst $(RECIPE)/pyxis/%.c,$(BUILD)/osmesa/pyxis/%.o,$(GL_BACKEND_SOURCES))
GL_OBJECTS := $(UPSTREAM_OBJECTS) $(GL_BACKEND_OBJECTS)

# The recipe's SDL_build_config.h is found before upstream's dispatcher.
CPPFLAGS := -I$(RECIPE) -I$(S)/include -I$(S)/src -I$(RECIPE)/pyxis $(PYXIS_CPPFLAGS)
# Upstream builds with -Wall, without strict aliasing and with the compiler's
# default C standard; its warnings stay visible. The backend uses SDK C23.
UPSTREAM_CFLAGS := $(filter-out -std=gnu23 -Wextra,$(PYXIS_CFLAGS)) -std=gnu17 -fno-strict-aliasing
BACKEND_CFLAGS := $(PYXIS_CFLAGS) -fno-strict-aliasing

.PHONY: all osmesa
all: $(BUILD)/libSDL3.a

osmesa: $(BUILD)/libSDL3-osmesa.a

$(BUILD)/upstream/%.o: $(S)/%.c $(RECIPE)/SDL_build_config.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(UPSTREAM_CFLAGS) -c $< -o $@

$(BUILD)/pyxis/%.o: $(RECIPE)/pyxis/%.c $(RECIPE)/SDL_build_config.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(BACKEND_CFLAGS) -c $< -o $@

$(BUILD)/osmesa/pyxis/%.o: $(RECIPE)/pyxis/%.c $(RECIPE)/SDL_build_config.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -I$(MESA)/include -DPYXIS_USE_OSMESA=1 $(BACKEND_CFLAGS) -c $< -o $@

$(BUILD)/libSDL3.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(OBJECTS)

$(BUILD)/libSDL3-osmesa.a: $(GL_OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(GL_OBJECTS)

-include $(OBJECTS:.o=.d) $(GL_BACKEND_OBJECTS:.o=.d)
