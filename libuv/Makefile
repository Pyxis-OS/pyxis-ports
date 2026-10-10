include $(SDK)/share/pyxis.mk

UPSTREAM := timer version uv-data-getter-setters strscpy
NATIVE := core stream process unsupported fs thread watchers
OBJECTS := $(addprefix $(BUILD)/upstream-,$(addsuffix .o,$(UPSTREAM))) \
           $(addprefix $(BUILD)/pyxis-,$(addsuffix .o,$(NATIVE)))
INCLUDES := -I$(RECIPE)/include -I$(RECIPE)/pyxis -I$(SOURCE)/include -I$(SOURCE)/src
HEADERS := $(wildcard $(RECIPE)/include/uv/*.h $(RECIPE)/pyxis/*.h \
                     $(SOURCE)/include/*.h $(SOURCE)/include/uv/*.h $(SOURCE)/src/*.h)

.PHONY: all
all: $(BUILD)/libuv.a $(BUILD)/uv-relay.pxe

$(BUILD)/upstream-%.o: $(SOURCE)/src/%.c $(HEADERS) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/pyxis-%.o: $(RECIPE)/pyxis/%.c $(HEADERS) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/libuv.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

$(BUILD)/relay.o: $(RECIPE)/examples/relay.c $(HEADERS) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD)/uv-relay.pxe: $(BUILD)/relay.o $(BUILD)/libuv.a $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(BUILD)/relay.o $(BUILD)/libuv.a $(PYXIS_LDLIBS)

-include $(OBJECTS:.o=.d) $(BUILD)/relay.d
