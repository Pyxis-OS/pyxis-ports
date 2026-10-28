include $(SDK)/share/pyxis.mk

LUA := $(BEE)/3rd/lua55
INCLUDES := -I$(BEE) -I$(RECIPE)/pyxis -I$(LUA) -I$(BEE)/3rd/lua-patch -I$(LPEGLABEL)
NATIVE_SOURCES := runtime.cpp filesystem.cpp platform.cpp concurrency.cpp
NATIVE_OBJECTS := $(addprefix $(BUILD)/,$(NATIVE_SOURCES:.cpp=.o))
BEE_OBJECTS := $(BUILD)/bee-filesystem.o $(BUILD)/bee-error.o
LPEG_SOURCES := lplcap.c lplcode.c lplprint.c lpltree.c lplvm.c
LPEG_OBJECTS := $(addprefix $(BUILD)/lpeg-,$(LPEG_SOURCES:.c=.o))
LUA_INPUTS := $(wildcard $(LUA)/*.h $(LUA)/*.c $(BEE)/3rd/lua-patch/*.h $(BEE)/3rd/lua-patch/*.c)

.PHONY: all
all: $(BUILD)/lua-language-server.pxe

$(BUILD)/lua-core.o: $(RECIPE)/pyxis/lua-core.cpp $(RECIPE)/pyxis/lua-config.h $(LUA_INPUTS)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) -std=gnu++17 $(INCLUDES) -include $(RECIPE)/pyxis/lua-config.h -c $< -o $@

$(BUILD)/lua-libraries.o: $(RECIPE)/pyxis/lua-libraries.cpp $(RECIPE)/pyxis/lua-config.h $(LUA_INPUTS)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) -std=gnu++17 $(INCLUDES) -include $(RECIPE)/pyxis/lua-config.h -c $< -o $@

$(BUILD)/bee-filesystem.o: $(BEE)/binding/lua_filesystem.cpp $(wildcard $(BEE)/bee/lua/*.h $(BEE)/bee/nonstd/*.h) $(LUA_INPUTS)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) $(INCLUDES) -DBEE_STATIC -c $< -o $@

$(BUILD)/bee-error.o: $(BEE)/bee/lua/error.cpp $(wildcard $(BEE)/bee/nonstd/*.h) $(LUA_INPUTS)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/bee-newstate.o: $(BEE)/3rd/lua-patch/bee_newstate.c $(LUA_INPUTS)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/lua-seri.o: $(BEE)/3rd/lua-seri/lua-seri.cpp $(LUA_INPUTS)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) -std=gnu++17 $(INCLUDES) -I$(BEE)/3rd/lua-seri -DLUALS_VERSION='"$(VERSION)"' -c $< -o $@

$(BUILD)/lpeg-%.o: $(LPEGLABEL)/%.c $(wildcard $(LPEGLABEL)/*.h) $(LUA_INPUTS)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(INCLUDES) -DMAXRECLEVEL=1000 -c $< -o $@

$(BUILD)/%.o: $(RECIPE)/pyxis/%.cpp $(wildcard $(RECIPE)/pyxis/*.hpp) $(LUA_INPUTS)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) -std=gnu++17 $(INCLUDES) -I$(BEE)/3rd/lua-seri -DLUALS_VERSION='"$(VERSION)"' -c $< -o $@

$(BUILD)/bootstrap.o: $(BEE)/bootstrap/main.cpp $(wildcard $(RECIPE)/pyxis/*.hpp) $(LUA_INPUTS)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) -std=gnu++17 $(INCLUDES) -c $< -o $@

OBJECTS := $(BUILD)/bootstrap.o $(NATIVE_OBJECTS) $(BEE_OBJECTS) $(LPEG_OBJECTS) $(BUILD)/lua-core.o \
  $(BUILD)/lua-libraries.o $(BUILD)/bee-newstate.o $(BUILD)/lua-seri.o

$(BUILD)/lua-language-server.pxe: $(OBJECTS) $(PYXIS_START) $(PYXIS_CXX_LIBRARIES) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CXX) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_CXX_LDLIBS)

-include $(OBJECTS:.o=.d)
