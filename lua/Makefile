include $(SDK)/share/pyxis.mk
include $(MBEDTLS)/share/mbedtls.mk

SOURCES := lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c \
           llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c \
           ltable.c ltm.c lundump.c lvm.c lzio.c lauxlib.c lbaselib.c \
           lcorolib.c ltablib.c lstrlib.c lutf8lib.c liolib.c loslib.c loadlib.c
OBJECTS := $(addprefix $(BUILD)/,$(SOURCES:.c=.o))

.PHONY: all
all: $(BUILD)/lua.pxe $(BUILD)/liblua.a

$(BUILD)/%.o: $(SOURCE)/%.c $(RECIPE)/config.h $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -include $(RECIPE)/config.h -c $< -o $@

$(BUILD)/main.o: $(RECIPE)/main.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE) -c $< -o $@

$(BUILD)/pyxis.o: $(RECIPE)/pyxis.c $(RECIPE)/pyxis.h $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(MBEDTLS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE) -c $< -o $@

$(BUILD)/liblua.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

$(BUILD)/lua.pxe: $(BUILD)/main.o $(BUILD)/pyxis.o $(BUILD)/liblua.a $(MBEDTLS_LIBRARIES) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(BUILD)/main.o $(BUILD)/pyxis.o $(BUILD)/liblua.a $(MBEDTLS_LIBRARIES) $(PYXIS_LDLIBS)

-include $(OBJECTS:.o=.d) $(BUILD)/main.d $(BUILD)/pyxis.d
