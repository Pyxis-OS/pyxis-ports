include $(SDK)/share/pyxis.mk

UPSTREAM_FLAGS := $(filter-out -std=gnu23 -MMD -MP,$(PYXIS_CFLAGS)) -std=gnu17 \
  -Werror=implicit-function-declaration -Werror=incompatible-pointer-types
SOURCES := main sudoku utils
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SOURCES)))

.PHONY: all
all: $(BUILD)/nudoku.pxe

$(BUILD)/%.o: $(SOURCE)/src/%.c $(RECIPE)/config.h
	mkdir -p $(BUILD)
	$(CC) $(PYXIS_CPPFLAGS) $(UPSTREAM_FLAGS) -include $(RECIPE)/config.h \
	  -I$(RECIPE) -I$(SOURCE)/src -I$(NCURSESW)/include -c $< -o $@

$(BUILD)/nudoku.pxe: $(OBJECTS) $(NCURSESW)/lib/libncursesw.a
	$(CC) $(PYXIS_LDFLAGS) $(PYXIS_START) $(OBJECTS) \
	  $(NCURSESW)/lib/libncursesw.a $(PYXIS_LDLIBS) -o $@
