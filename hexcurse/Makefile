include $(SDK)/share/pyxis.mk

UPSTREAM_FLAGS := $(filter-out -std=gnu23 -MMD -MP,$(PYXIS_CFLAGS)) -std=gnu17 \
  -Werror=implicit-function-declaration -Werror=incompatible-pointer-types
SOURCES := file llist screen hexcurse stack getopt acceptch color
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SOURCES)))

.PHONY: all
all: $(BUILD)/hexcurse.pxe

$(BUILD)/%.o: $(SOURCE)/src/%.c $(SOURCE)/include/hex.h $(RECIPE)/config.h
	mkdir -p $(BUILD)
	$(CC) $(PYXIS_CPPFLAGS) $(UPSTREAM_FLAGS) -I$(RECIPE) -I$(SOURCE)/include \
	  -I$(NCURSESW)/include -c $< -o $@

$(BUILD)/hexcurse.pxe: $(OBJECTS) $(NCURSESW)/lib/libncursesw.a
	$(CC) $(PYXIS_LDFLAGS) $(PYXIS_START) $(OBJECTS) \
	  $(NCURSESW)/lib/libncursesw.a $(PYXIS_LDLIBS) -o $@
