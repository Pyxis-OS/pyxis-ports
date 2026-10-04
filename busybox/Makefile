include $(SDK)/share/pyxis.mk

# Only editors/vi.c is built. The recipe's libbb.h and vi_config.h replace
# BusyBox's libbb and Kconfig output, and busybox_pyxis.c supplies main.
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(RECIPE)
OBJECTS := $(BUILD)/vi.o $(BUILD)/busybox_pyxis.o

.PHONY: all
all: $(BUILD)/vi.pxe

# Upstream relies on unused parameters, signed/unsigned comparisons and
# intentional case fallthrough that BusyBox's own flags do not report.
VI_WARNINGS := -Wno-unused-parameter -Wno-sign-compare -Wno-implicit-fallthrough

$(BUILD)/vi.o: $(SOURCE)/editors/vi.c $(RECIPE)/libbb.h $(RECIPE)/vi_config.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/busybox_pyxis.o: $(RECIPE)/busybox_pyxis.c $(RECIPE)/libbb.h $(RECIPE)/vi_config.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/vi.elf: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/vi.pxe: $(BUILD)/vi.elf
	$(PYXIS_ELF2PXE) --format p1f -o $@ $<

-include $(OBJECTS:.o=.d)
