include $(SDK)/share/pyxis.mk
JSON ?= $(abspath ../../../build/ports/json/stage/dev)
BUILD ?= build
.PHONY: all
all: $(BUILD)/json-check.pxe
$(BUILD)/json-check.pxe: check.cpp
	mkdir -p $(BUILD)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS) -I$(JSON)/include \
	  $< $(PYXIS_START) $(PYXIS_LDFLAGS) $(PYXIS_CXX_LDLIBS) -o $@
