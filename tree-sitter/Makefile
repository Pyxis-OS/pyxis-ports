include $(SDK)/share/pyxis.mk

# The upstream amalgamation includes the Wasm-disabled API implementation.
.PHONY: all
all: $(BUILD)/libtree-sitter.a

$(BUILD)/tree-sitter.o: $(SOURCE)/lib/src/lib.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE)/lib/include \
	  -I$(SOURCE)/lib/src -c $< -o $@

$(BUILD)/libtree-sitter.a: $(BUILD)/tree-sitter.o
	$(AR) rcs $@ $<

-include $(BUILD)/tree-sitter.d
