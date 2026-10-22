include $(SDK)/share/pyxis.mk

LIB_SOURCES := argvsplit audit buffer bufferset bytecode bsdstubs cache client \
  dependency fileio fragment license output parser path personality pkg queue \
  tuple variable version
CLI_SOURCES := main core getopt_long renderer-msvc
OBJECTS := $(addprefix $(BUILD)/libpkgconf/,$(addsuffix .o,$(LIB_SOURCES))) \
  $(addprefix $(BUILD)/cli/,$(addsuffix .o,$(CLI_SOURCES)))

.PHONY: all
all: $(BUILD)/pkgconf.pxe

$(BUILD)/libpkgconf/config.h: $(RECIPE)/config.h
	@mkdir -p $(@D)
	install -m 644 $< $@

$(BUILD)/%.o: $(SOURCE)/%.c $(BUILD)/libpkgconf/config.h
	@mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(BUILD) -I$(SOURCE) \
	  -DPKGCONFIG_IS_STATIC -c $< -o $@

$(BUILD)/pkgconf.pxe: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

-include $(OBJECTS:.o=.d)
