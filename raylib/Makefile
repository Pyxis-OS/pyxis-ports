include $(SDK)/share/pyxis.mk

SOURCES := rcore rshapes rtextures rtext rmodels raudio
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SOURCES)))
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(SOURCE)/src -I$(RECIPE) -I$(SDL2)/include -include $(RECIPE)/pyxis_config.h
CFLAGS := $(PYXIS_CFLAGS) -ffunction-sections -fdata-sections

.PHONY: all
all: $(BUILD)/libraylib.a $(BUILD)/raylib-shapes.pxe

$(BUILD)/%.o: $(SOURCE)/src/%.c $(RECIPE)/pyxis_config.h $(RECIPE)/pyxis_miniaudio.h $(RECIPE)/Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/libraylib.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(OBJECTS)

-include $(OBJECTS:.o=.d)

$(BUILD)/raylib-shapes.pxe: $(RECIPE)/examples/shapes.c $(BUILD)/libraylib.a $(SDL2)/lib/libSDL2.a $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PYXIS_LDFLAGS) -Wl,--gc-sections $(PYXIS_START) $< $(BUILD)/libraylib.a $(SDL2)/lib/libSDL2.a $(PYXIS_LDLIBS) -o $@
