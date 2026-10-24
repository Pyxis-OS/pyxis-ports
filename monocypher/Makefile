include $(SDK)/share/pyxis.mk

OBJECTS := $(BUILD)/monocypher.o $(BUILD)/monocypher-ed25519.o

.PHONY: all
all: $(BUILD)/libmonocypher.a

$(BUILD)/monocypher.o: $(SOURCE)/src/monocypher.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE)/src -c $< -o $@

$(BUILD)/monocypher-ed25519.o: $(SOURCE)/src/optional/monocypher-ed25519.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE)/src -c $< -o $@

$(BUILD)/libmonocypher.a: $(OBJECTS)
	$(AR) rcs $@ $^

-include $(OBJECTS:.o=.d)
