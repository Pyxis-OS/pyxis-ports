include $(SDK)/share/pyxis.mk

# config.h replaces configure's output, which cannot run against the SDK.
# include/ holds declarations for Unix interfaces Links compiles but this port
# never reaches; pyxis.c is the platform layer, as dos.c is for DOS.
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(RECIPE) -I$(RECIPE)/include -DHAVE_CONFIG_H
SOURCES := $(filter-out $(SOURCE)/dos.c,$(wildcard $(SOURCE)/*.c))
OBJECTS := $(patsubst $(SOURCE)/%.c,$(BUILD)/%.o,$(SOURCES)) \
           $(BUILD)/pyxis.o $(BUILD)/pyxis_console.o $(BUILD)/network.o
HEADERS := $(RECIPE)/config.h $(wildcard $(RECIPE)/include/*.h $(RECIPE)/include/*/*.h)

.PHONY: all
all: $(BUILD)/links.pxe

# Links is written in C89 style. Its sources leave parameters and, depending
# on the configuration, some variables, values and helpers unused.
LINKS_CFLAGS := -std=gnu17 -Wno-unused-parameter -Wno-unused-variable \
                -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-function

$(BUILD)/%.o: $(SOURCE)/%.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(LINKS_CFLAGS) -c $< -o $@

$(BUILD)/pyxis.o: $(RECIPE)/pyxis.c $(RECIPE)/pyxis_console.h $(HEADERS)
	$(CC) $(CPPFLAGS) -I$(SOURCE) $(PYXIS_CFLAGS) $(LINKS_CFLAGS) -c $< -o $@

$(BUILD)/pyxis_console.o: $(RECIPE)/pyxis_console.c $(RECIPE)/pyxis_console.h
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/network.o: $(RECIPE)/network.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -Wno-unused-parameter -c $< -o $@

$(BUILD)/links.pxe: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

-include $(OBJECTS:.o=.d)
