include $(SDK)/share/pyxis.mk

OBJECTS := $(BUILD)/bitwise.o $(BUILD)/framing.o
.PHONY: all
all: $(BUILD)/libogg.a

$(BUILD)/include/ogg/config_types.h: $(RECIPE)/config_types.h
	mkdir -p $(dir $@)
	cp $< $@

$(BUILD)/%.o: $(SOURCE)/src/%.c $(BUILD)/include/ogg/config_types.h $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(BUILD)/include -I$(SOURCE)/include -c $< -o $@

$(BUILD)/libogg.a: $(OBJECTS)
	$(AR) rcs $@ $^

-include $(OBJECTS:.o=.d)
