include $(SDK)/share/pyxis.mk

# Lua 5.1.5 core and standard libraries; lua.c is the interpreter only.
LUA_CORE := lapi lcode ldebug ldo ldump lfunc lgc llex lmem lobject lopcodes \
            lparser lstate lstring ltable ltm lundump lvm lzio lauxlib lbaselib \
            ldblib liolib lmathlib loslib ltablib lstrlib loadlib linit
LPEG_SOURCES := lpcap lpcode lpcset lpprint lptree lpvm
LUA_OBJECTS := $(addprefix $(BUILD)/lua-,$(addsuffix .o,$(LUA_CORE)))
LPEG_OBJECTS := $(addprefix $(BUILD)/lpeg-,$(addsuffix .o,$(LPEG_SOURCES)))
LUV_OBJECTS := $(BUILD)/luv.o $(BUILD)/compat-5.3.o
LUA_INCLUDES := -I$(SOURCE)/src
LUV_INCLUDES := $(LUA_INCLUDES) -I$(COMPAT53)/c-api -I$(LUV)/src -I$(LIBUV)/include

.PHONY: all
all: $(BUILD)/liblua5.1.a $(BUILD)/liblpeg.a $(BUILD)/libluv.a $(BUILD)/lua5.1.pxe

$(BUILD)/lua-%.o: $(SOURCE)/src/%.c $(wildcard $(SOURCE)/src/*.h) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(LUA_INCLUDES) -c $< -o $@

$(BUILD)/lpeg-%.o: $(LPEG)/%.c $(wildcard $(LPEG)/*.h $(SOURCE)/src/*.h) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(LUA_INCLUDES) -c $< -o $@

# luv.c includes every other luv source file.
$(BUILD)/luv.o: $(LUV)/src/luv.c $(wildcard $(LUV)/src/*.c $(LUV)/src/*.h) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(LUV_INCLUDES) -c $< -o $@

$(BUILD)/compat-5.3.o: $(COMPAT53)/c-api/compat-5.3.c $(COMPAT53)/c-api/compat-5.3.h $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(LUA_INCLUDES) -c $< -o $@

$(BUILD)/liblua5.1.a: $(LUA_OBJECTS)
	$(AR) rcs $@ $(LUA_OBJECTS)

$(BUILD)/liblpeg.a: $(LPEG_OBJECTS)
	$(AR) rcs $@ $(LPEG_OBJECTS)

$(BUILD)/libluv.a: $(LUV_OBJECTS)
	$(AR) rcs $@ $(LUV_OBJECTS)

$(BUILD)/lua.o: $(SOURCE)/src/lua.c $(wildcard $(SOURCE)/src/*.h) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(LUA_INCLUDES) -c $< -o $@

$(BUILD)/lua5.1.pxe: $(BUILD)/lua.o $(BUILD)/libluv.a $(BUILD)/liblpeg.a $(BUILD)/liblua5.1.a \
                     $(LIBUV)/lib/libuv.a $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(BUILD)/lua.o $(BUILD)/libluv.a \
	  $(BUILD)/liblpeg.a $(BUILD)/liblua5.1.a $(LIBUV)/lib/libuv.a $(PYXIS_LDLIBS)

-include $(LUA_OBJECTS:.o=.d) $(LPEG_OBJECTS:.o=.d) $(LUV_OBJECTS:.o=.d) $(BUILD)/lua.d
