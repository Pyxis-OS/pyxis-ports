include $(SDK)/share/pyxis.mk

.PHONY: all
all: $(BUILD)/libutf8proc.a

$(BUILD)/utf8proc.o: $(SOURCE)/utf8proc.c $(SOURCE)/utf8proc.h \
                     $(SOURCE)/utf8proc_data.c $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -DUTF8PROC_STATIC -DUTF8PROC_EXPORTS -c $< -o $@

$(BUILD)/libutf8proc.a: $(BUILD)/utf8proc.o
	$(AR) rcs $@ $<

-include $(BUILD)/utf8proc.d
