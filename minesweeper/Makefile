include $(SDK)/share/pyxis.mk

CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(RAYLIB)/include
CFLAGS := $(PYXIS_CFLAGS) -ffunction-sections -fdata-sections

.PHONY: all
all: $(BUILD)/minesweeper.pxe

$(BUILD)/main.o: $(SOURCE)/main.c $(RECIPE)/Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/minesweeper.pxe: $(BUILD)/main.o $(RAYLIB)/lib/libraylib.a $(SDL2)/lib/libSDL2.a $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -Wl,--gc-sections $(PYXIS_START) $< $(RAYLIB)/lib/libraylib.a $(SDL2)/lib/libSDL2.a $(PYXIS_LDLIBS) -o $@

-include $(BUILD)/main.d
