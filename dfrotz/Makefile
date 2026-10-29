include $(SDK)/share/pyxis.mk

UPSTREAM_FLAGS := $(filter-out -std=gnu23,$(PYXIS_CFLAGS)) -std=gnu17

.PHONY: all
all:
	$(MAKE) -C $(BUILD)/upstream -j16 dumb \
	  CC="$(CC)" AR="$(AR)" RANLIB="$(shell $(CC) -print-prog-name=llvm-ranlib)" \
	  CFLAGS="$(PYXIS_CPPFLAGS) $(UPSTREAM_FLAGS)" \
	  LDFLAGS="$(PYXIS_LDFLAGS) $(PYXIS_START) $(PYXIS_LDLIBS)" \
	  UNAME_S=Pyxis SOUND_TYPE=none NO_PKGCONF=yes \
	  GIT_DIR=$(BUILD)/no-git \
	  GIT_HASH=acf205585a9472d27c07c0fe62da4b8bc89d1ec7 GIT_HASH_SHORT=acf20558 \
	  GIT_DATE="2025-02-01 14:36:37 -0800" \
	  DFROTZ_BIN=$(BUILD)/dfrotz.pxe
