include $(SDK)/share/pyxis.mk

OBJECTS := $(addprefix $(BUILD)/,info.o internal.o opusfile.o stream.o)
.PHONY: all
all: $(BUILD)/libopusfile.a

$(BUILD)/%.o: $(SOURCE)/src/%.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -DOP_HAVE_LRINTF -I$(SOURCE)/include -I$(OGG)/include -I$(OPUS)/include/opus -c $< -o $@

$(BUILD)/libopusfile.a: $(OBJECTS)
	$(AR) rcs $@ $^

-include $(OBJECTS:.o=.d)
