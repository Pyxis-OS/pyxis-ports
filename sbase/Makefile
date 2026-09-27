include $(SDK)/share/pyxis.mk

OBJECTS := $(BUILD)/cksum.o $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/fshut.o

.PHONY: all
all: $(BUILD)/cksum.pxe

$(BUILD)/%.o: $(SOURCE)/%.c $(SOURCE)/util.h $(SOURCE)/arg.h
	@mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/cksum.elf: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/cksum.pxe: $(BUILD)/cksum.elf
	$(PYXIS_ELF2PXE) --format p1f -o $@ $<

-include $(OBJECTS:.o=.d)
