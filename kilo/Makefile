include $(SDK)/share/pyxis.mk

.PHONY: all
all: $(BUILD)/kilo.pxe

$(BUILD)/kilo.o: $(SOURCE)/kilo.c
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/kilo.pxe: $(BUILD)/kilo.o $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $< $(PYXIS_LDLIBS)
