include $(SDK)/share/pyxis.mk

# Keep the core and buffer convenience APIs; gz* file I/O is outside this profile.
SOURCES := adler32.c crc32.c deflate.c infback.c inffast.c inflate.c \
           inftrees.c trees.c zutil.c compress.c uncompr.c
OBJECTS := $(addprefix $(BUILD)/,$(SOURCES:.c=.o))
HEADERS := $(wildcard $(SOURCE)/*.h)

.PHONY: all
all: $(BUILD)/libz.a

$(BUILD)/%.o: $(SOURCE)/%.c $(HEADERS) $(RECIPE)/Makefile
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/libz.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

-include $(OBJECTS:.o=.d)
