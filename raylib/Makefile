include $(SDK)/share/pyxis.mk

SOURCES := rcore rshapes rtextures rtext rmodels
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SOURCES)))
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(SOURCE)/src -I$(SDL2)/include -include $(RECIPE)/pyxis_config.h
CFLAGS := $(PYXIS_CFLAGS) -ffunction-sections -fdata-sections

.PHONY: all
all: $(BUILD)/libraylib.a

$(BUILD)/%.o: $(SOURCE)/src/%.c $(RECIPE)/pyxis_config.h $(RECIPE)/Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/libraylib.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(OBJECTS)

-include $(OBJECTS:.o=.d)
