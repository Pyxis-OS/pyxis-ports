include $(SDK)/share/pyxis.mk

UPSTREAM_FLAGS := $(filter-out -std=gnu23 -MMD -MP,$(PYXIS_CFLAGS)) -std=gnu17 \
  -Werror=implicit-function-declaration -Werror=incompatible-pointer-types

.PHONY: all
all: $(BUILD)/ncurses-demo.pxe

$(BUILD)/configured: $(RECIPE)/Makefile $(RECIPE)/pyxis.ti
	mkdir -p $(BUILD)
	cd $(BUILD) && CC=$(CC) AR="$(AR)" RANLIB="$(CROSS_COMPILE)ranlib" \
	  CPPFLAGS="$(PYXIS_CPPFLAGS)" CFLAGS="$(UPSTREAM_FLAGS)" \
	  LDFLAGS="--sysroot=$(PYXIS_SYSROOT) -static -no-pie" \
	  $(SOURCE)/configure --host="$$( $(CC) -dumpmachine )" --build="$$( $(SOURCE)/config.guess )" \
	  --prefix=/usr --with-build-cc="$(HOSTCC)" \
	  --without-cxx --without-cxx-binding --without-ada --without-debug \
	  --without-progs --without-tests --without-manpages --disable-db-install \
	  --disable-home-terminfo --disable-sigwinch --enable-term-driver \
	  --disable-ext-colors --disable-database --disable-termcap --with-fallbacks=pyxis
	touch $@

$(BUILD)/lib/libncursesw.a: $(BUILD)/configured
	$(MAKE) -C $(BUILD)/include
	$(MAKE) -C $(BUILD)/ncurses libs CFLAGS="$(UPSTREAM_FLAGS)" TERMINFO_SRC=$(RECIPE)/pyxis.ti

$(BUILD)/ncurses-demo.pxe: $(BUILD)/lib/libncursesw.a $(RECIPE)/demo.c
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(BUILD)/include $(RECIPE)/demo.c \
	  $(PYXIS_LDFLAGS) $(PYXIS_START) $(BUILD)/lib/libncursesw.a $(PYXIS_LDLIBS) -o $@
