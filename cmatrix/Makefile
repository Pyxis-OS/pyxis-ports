include $(SDK)/share/pyxis.mk

UPSTREAM_FLAGS := $(filter-out -std=gnu23 -MMD -MP,$(PYXIS_CFLAGS)) -std=gnu17 \
  -Werror=implicit-function-declaration -Werror=incompatible-pointer-types
SOURCES := cmatrix
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SOURCES)))

.PHONY: all
all: $(BUILD)/cmatrix.pxe

$(BUILD)/%.o: $(SOURCE)/%.c $(RECIPE)/config.h
	mkdir -p $(BUILD)
	$(CC) $(PYXIS_CPPFLAGS) $(UPSTREAM_FLAGS) -include $(RECIPE)/config.h \
	  -I$(RECIPE) -I$(SOURCE) -I$(NCURSESW)/include -c $< -o $@

$(BUILD)/cmatrix.pxe: $(OBJECTS) $(NCURSESW)/lib/libncursesw.a
	$(CC) $(PYXIS_LDFLAGS) $(PYXIS_START) $(OBJECTS) \
	  $(NCURSESW)/lib/libncursesw.a $(PYXIS_LDLIBS) -o $@
