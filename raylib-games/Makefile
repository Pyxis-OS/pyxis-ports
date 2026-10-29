include $(SDK)/share/pyxis.mk

CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(RAYLIB)/include
CFLAGS := $(PYXIS_CFLAGS) -ffunction-sections -fdata-sections

.PHONY: all
all: $(addprefix $(BUILD)/,$(addsuffix .pxe,$(GAMES)))

$(BUILD)/%.o: $(SOURCE)/classics/src/%.c $(RECIPE)/Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/%.pxe: $(BUILD)/%.o $(RAYLIB)/lib/libraylib.a $(SDL2)/lib/libSDL2.a $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -Wl,--gc-sections $(PYXIS_START) $< $(RAYLIB)/lib/libraylib.a $(SDL2)/lib/libSDL2.a $(PYXIS_LDLIBS) -o $@

-include $(addprefix $(BUILD)/,$(addsuffix .d,$(GAMES)))
