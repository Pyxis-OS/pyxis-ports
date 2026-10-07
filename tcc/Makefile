SDK ?= build/sdk
SDK := $(abspath $(SDK))
SOURCE := $(abspath $(SOURCE))
BUILD := $(abspath $(BUILD))
STAGE := $(abspath $(STAGE))
HOST_CC ?= cc
# Read-only SDK layout used by the Pyxis boot archive.
GUEST_SYSROOT ?= boot://sdk
GUEST_TCCDIR ?= $(GUEST_SYSROOT)/lib/tcc
.DEFAULT_GOAL := all
RECIPE_MAKEFILE := $(lastword $(MAKEFILE_LIST))
include $(SDK)/share/pyxis.mk

SUPPORT_SOURCES := libtcc1.c va_list.c builtin.c
SUPPORT_OBJECTS := $(addprefix $(BUILD)/runtime/,$(SUPPORT_SOURCES:.c=.o))

.PHONY: all host
all: host $(BUILD)/libtcc1.a $(BUILD)/guest/tcc.pxe

# This compiler runs on the host. Only its output and support archive target Pyxis.
$(BUILD)/host/config.mak: $(SOURCE)/configure $(RECIPE_MAKEFILE)
	mkdir -p $(@D)
	cd $(@D) && $(SOURCE)/configure --cc=$(HOST_CC) \
	  --config-backtrace=no --config-bcheck=no --config-semlock=no \
	  --config-pie=no --config-pic=no \
	  --sysroot=$(PYXIS_SYSROOT) --tccdir=$(STAGE)/lib/tcc

host: $(BUILD)/host/config.mak
	$(MAKE) -C $(BUILD)/host x86_64-pyxis-tcc \
	  CPPFLAGS="-iquote $(PYXIS_SYSROOT)/usr/include -Wno-unused-function" \
	  PYXIS_P1F_HEADER=$(PYXIS_SYSROOT)/usr/include/pxe/p1f.h \
	  PYXIS_RUNTIME_LIBRARY=$(PYXIS_RUNTIME_LIBRARY)

GUEST_SOURCES := tcc.c libtcc.c tccpp.c tccgen.c tccdbg.c tccasm.c tccelf.c \
                 tccrun.c tccp1f.c x86_64-gen.c x86_64-link.c i386-asm.c
GUEST_DEFINES := -DTCC_TARGET_X86_64 -DTCC_TARGET_PYXIS \
                 -DCONFIG_TCC_BACKTRACE=0 -DCONFIG_TCC_BCHECK=0 \
                 -DCONFIG_TCC_SEMLOCK=0 -DCONFIG_DWARF_VERSION=5 \
                 -DCONFIG_SYSROOT='"$(GUEST_SYSROOT)"' \
                 -DCONFIG_TCCDIR='"$(GUEST_TCCDIR)"' \
                 -DCONFIG_TCC_CROSSPREFIX='""' \
                 -DPYXIS_RUNTIME_LIBRARY='"$(PYXIS_RUNTIME_LIBRARY)"'

# Reuse upstream's generated version/predefines, but compile and link entirely
# with the target SDK. The host target generates tccdefs_.h before this starts.
$(BUILD)/guest/tcc.o: $(addprefix $(SOURCE)/,$(GUEST_SOURCES)) \
                    $(wildcard $(SOURCE)/*.h) $(SDK)/share/pyxis.mk $(RECIPE_MAKEFILE) | host
	mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) $(GUEST_DEFINES) \
	  -I$(BUILD)/host -I$(SOURCE) -frounding-math -fstack-usage \
	  -Wno-unused-parameter -Wno-sign-compare -Wno-missing-field-initializers \
	  -Wno-implicit-fallthrough -Wno-unused-function \
	  -c $(SOURCE)/tcc.c -o $@

$(BUILD)/guest/tcc.elf: $(BUILD)/guest/tcc.o $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $< $(PYXIS_LDLIBS)

$(BUILD)/guest/tcc.pxe: $(BUILD)/guest/tcc.elf $(PYXIS_ELF2PXE)
	$(PYXIS_ELF2PXE) --format p1f -o $@ $<

$(BUILD)/runtime/%.o: $(SOURCE)/lib/%.c $(SDK)/share/pyxis.mk $(RECIPE_MAKEFILE)
	mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -frounding-math -c $< -o $@

$(BUILD)/libtcc1.a: $(SUPPORT_OBJECTS)
	rm -f $@
	$(AR) rcs $@ $^

-include $(SUPPORT_OBJECTS:.o=.d)
-include $(BUILD)/guest/tcc.d
