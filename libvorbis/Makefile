include $(SDK)/share/pyxis.mk

CORE := mdct smallft block envelope window lsp lpc analysis synthesis psy info \
        floor1 floor0 res0 mapping0 registry codebook sharedbook lookup bitrate
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(CORE)))
ALL_OBJECTS := $(OBJECTS) $(BUILD)/vorbisenc.o $(BUILD)/vorbisfile.o

.PHONY: all
all: $(BUILD)/libvorbis.a $(BUILD)/libvorbisenc.a $(BUILD)/libvorbisfile.a

$(BUILD)/%.o: $(SOURCE)/lib/%.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -DHAVE_ALLOCA_H -I$(OGG)/include -I$(SOURCE)/include -I$(SOURCE)/lib -c $< -o $@

$(BUILD)/libvorbis.a: $(OBJECTS)
	$(AR) rcs $@ $^

$(BUILD)/libvorbisenc.a: $(BUILD)/vorbisenc.o
	$(AR) rcs $@ $^

$(BUILD)/libvorbisfile.a: $(BUILD)/vorbisfile.o
	$(AR) rcs $@ $^

-include $(ALL_OBJECTS:.o=.d)
