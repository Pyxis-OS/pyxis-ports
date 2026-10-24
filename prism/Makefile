include $(SDK)/share/pyxis.mk

PRISM_CXXFLAGS := $(filter-out -std=%,$(PYXIS_CXXFLAGS)) -std=c++20
PRISM_CFLAGS := $(filter-out -std=%,$(PYXIS_CFLAGS)) -std=c11
OBJECTS := $(BUILD)/ast.o $(BUILD)/lexer.o $(BUILD)/processor.o \
           $(BUILD)/gv.o $(BUILD)/invoke.o

.PHONY: all
all: $(BUILD)/libprism.a

$(BUILD)/%.o: $(SOURCE)/src/prism/%.cpp $(RECIPE)/Makefile
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PRISM_CXXFLAGS) -I$(SPDLOG)/include -DSPDLOG_COMPILED_LIB -c $< -o $@

$(BUILD)/gv.o: $(SOURCE)/src/prism/utils/gv.cpp $(RECIPE)/Makefile
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(PRISM_CXXFLAGS) -c $< -o $@

# Upstream's unspecified-argument bridge needs C11, not C23 or C++ prototypes.
$(BUILD)/invoke.o: $(SOURCE)/src/prism/utils/invoke.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PRISM_CFLAGS) -c $< -o $@

$(BUILD)/libprism.a: $(OBJECTS)
	$(AR) rcs $@ $^

-include $(OBJECTS:.o=.d)
