include $(SDK)/share/pyxis.mk

.PHONY: all
all: $(BUILD)/libpicohttpparser.a

$(BUILD)/picohttpparser.o: $(SOURCE)/picohttpparser.c $(SOURCE)/picohttpparser.h $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/libpicohttpparser.a: $(BUILD)/picohttpparser.o
	$(AR) rcs $@ $<

-include $(BUILD)/picohttpparser.d
