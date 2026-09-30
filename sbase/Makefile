include $(SDK)/share/pyxis.mk

CKSUM_OBJECTS := $(BUILD)/cksum.o $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/fshut.o
TEE_OBJECTS := $(BUILD)/tee.o $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/ealloc.o \
               $(BUILD)/libutil/writeall.o
UNIQ_OBJECTS := $(BUILD)/uniq.o $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/ealloc.o \
                $(BUILD)/libutil/fshut.o $(BUILD)/libutil/strtonum.o
SHA256SUM_OBJECTS := $(BUILD)/sha256sum.o $(BUILD)/libutil/crypt.o $(BUILD)/libutil/sha256.o \
                     $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/fshut.o

.PHONY: all
all: $(BUILD)/cksum.pxe $(BUILD)/tee.pxe $(BUILD)/uniq.pxe $(BUILD)/sha256sum.pxe

$(BUILD)/%.o: $(SOURCE)/%.c $(SOURCE)/util.h $(SOURCE)/arg.h $(SOURCE)/compat.h
	@mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/cksum.elf: $(CKSUM_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(CKSUM_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/tee.elf: $(TEE_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(TEE_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/uniq.elf: $(UNIQ_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(UNIQ_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/sha256sum.elf: $(SHA256SUM_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(SHA256SUM_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/%.pxe: $(BUILD)/%.elf
	$(PYXIS_ELF2PXE) --format p1f -o $@ $<

-include $(sort $(CKSUM_OBJECTS:.o=.d) $(TEE_OBJECTS:.o=.d) $(UNIQ_OBJECTS:.o=.d) \
  $(SHA256SUM_OBJECTS:.o=.d))
