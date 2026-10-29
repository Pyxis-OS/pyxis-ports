include $(SDK)/share/pyxis.mk

UPSTREAM_FLAGS := $(filter-out -std=gnu23 -MMD -MP,$(PYXIS_CFLAGS)) -std=gnu17 \
  -Werror=implicit-function-declaration -Werror=incompatible-pointer-types

.PHONY: all
all:
	$(MAKE) -j16 -C $(BUILD)/source/src CC="$(CC)" \
	  CPPFLAGS="$(PYXIS_CPPFLAGS) -I$(NCURSESW)/include -DHISCORE_FILENAME='\"home://.vitetris-hiscores\"'" \
	  CFLAGS="$(UPSTREAM_FLAGS)" \
	  LDFLAGS="$(PYXIS_LDFLAGS) $(PYXIS_START)" \
	  LDLIBS="$(NCURSESW)/lib/libncursesw.a $(PYXIS_LDLIBS)"
