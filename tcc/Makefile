SDK ?= build/sdk
SDK := $(abspath $(SDK))
SOURCE := $(abspath $(SOURCE))
BUILD := $(abspath $(BUILD))
STAGE := $(abspath $(STAGE))
HOST_CC ?= cc
.DEFAULT_GOAL := all
RECIPE_MAKEFILE := $(lastword $(MAKEFILE_LIST))
include $(SDK)/share/pyxis.mk

SUPPORT_SOURCES := libtcc1.c va_list.c builtin.c
SUPPORT_OBJECTS := $(addprefix $(BUILD)/runtime/,$(SUPPORT_SOURCES:.c=.o))

.PHONY: all host
all: host $(BUILD)/libtcc1.a

# This compiler runs on the host. Only its output and support archive target Pyxis.
$(BUILD)/host/config.mak: $(SOURCE)/configure $(RECIPE_MAKEFILE)
	mkdir -p $(@D)
	cd $(@D) && $(SOURCE)/configure --cc=$(HOST_CC) \
	  --config-backtrace=no --config-bcheck=no --config-semlock=no \
	  --config-pie=no --config-pic=no \
	  --sysroot=$(PYXIS_SYSROOT) --tccdir=$(STAGE)/lib/tcc

host: $(BUILD)/host/config.mak
	$(MAKE) -C $(BUILD)/host x86_64-pyxis-tcc

$(BUILD)/runtime/%.o: $(SOURCE)/lib/%.c $(SDK)/share/pyxis.mk $(RECIPE_MAKEFILE)
	mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -frounding-math -c $< -o $@

$(BUILD)/libtcc1.a: $(SUPPORT_OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^

-include $(SUPPORT_OBJECTS:.o=.d)
