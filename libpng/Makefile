include $(SDK)/share/pyxis.mk

CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(BUILD) -I$(SOURCE) -I$(ZLIB)/include \
            -DPNG_INTEL_SSE_OPT=0
SOURCES := png.c pngerror.c pngget.c pngmem.c pngpread.c pngread.c \
           pngrio.c pngrtran.c pngrutil.c pngset.c pngtrans.c \
           pngwio.c pngwrite.c pngwtran.c pngwutil.c
OBJECTS := $(addprefix $(BUILD)/,$(SOURCES:.c=.o))
HEADERS := $(wildcard $(SOURCE)/*.h)
SDK_HEADERS := $(wildcard $(PYXIS_SYSROOT)/usr/include/*.h)
CONFIG_INPUTS := $(SOURCE)/scripts/pnglibconf.mak $(SOURCE)/scripts/pnglibconf.dfa \
                 $(SOURCE)/scripts/options.awk $(SOURCE)/scripts/dfn.awk \
                 $(SOURCE)/pngconf.h $(SOURCE)/pngusr.dfa $(RECIPE)/pyxis.dfa \
                 $(RECIPE)/Makefile $(SDK)/share/pyxis.mk $(SDK_HEADERS) \
                 $(ZLIB)/include/zlib.h $(ZLIB)/include/zconf.h

.PHONY: all
all: $(BUILD)/libpng.a

# The upstream generator runs awk and the target preprocessor, never target code.
# Force its intermediates to follow all wrapper inputs, including dfn.awk.
$(BUILD)/pnglibconf.h: $(CONFIG_INPUTS)
	rm -f $@ $(BUILD)/pnglibconf.dfn
	$(MAKE) -C $(BUILD) -f $(SOURCE)/scripts/pnglibconf.mak pnglibconf.h \
	  srcdir=$(SOURCE) AWK=awk CC="$(CC)" CPP="$(CC) -E" \
	  CPPFLAGS="$(CPPFLAGS)" CFLAGS="$(filter-out -MMD -MP,$(PYXIS_CFLAGS))" \
	  DFA_XTRA=$(RECIPE)/pyxis.dfa

$(BUILD)/%.o: $(SOURCE)/%.c $(HEADERS) $(BUILD)/pnglibconf.h $(RECIPE)/Makefile
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/libpng.a: $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

-include $(OBJECTS:.o=.d)
