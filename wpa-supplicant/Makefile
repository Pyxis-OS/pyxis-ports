include $(SDK)/share/pyxis.mk

ifeq ($(strip $(SOURCES)),)
$(error SOURCES is supplied by this recipe's build.lua)
endif

OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SOURCES)))
INCLUDES := -I$(RECIPE)/include -I$(SOURCE)/src -I$(SOURCE)/src/utils

.PHONY: all
all: $(BUILD)/libwpa-rsn.a

$(BUILD)/%.o: $(SOURCE)/src/%.c $(RECIPE)/Makefile $(RECIPE)/include/wpa-rsn-config.h
	mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -Wno-unused-parameter -ffunction-sections -fdata-sections $(INCLUDES) -c $< -o $@

$(BUILD)/libwpa-rsn.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

-include $(OBJECTS:.o=.d)
