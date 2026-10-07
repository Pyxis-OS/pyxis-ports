include $(SDK)/share/pyxis.mk

# Applets are selected separately; no default BusyBox configuration is built.
# Recipe-local support/config headers replace the selected libbb interfaces.
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(RECIPE)
OBJECTS := $(BUILD)/vi.o $(BUILD)/busybox_pyxis.o

.PHONY: all
all: $(BUILD)/vi.pxe $(BUILD)/less.pxe $(BUILD)/tar.pxe

# Upstream relies on unused parameters, signed/unsigned comparisons and
# intentional case fallthrough that BusyBox's own flags do not report.
VI_WARNINGS := -Wno-unused-parameter -Wno-sign-compare -Wno-implicit-fallthrough

$(BUILD)/vi.o: $(SOURCE)/editors/vi.c $(RECIPE)/libbb.h $(RECIPE)/vi_config.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/busybox_pyxis.o: $(RECIPE)/busybox_pyxis.c $(RECIPE)/libbb.h $(RECIPE)/vi_config.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/vi.pxe: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

-include $(OBJECTS:.o=.d)

$(BUILD)/less.o: $(SOURCE)/miscutils/less.c $(RECIPE)/less_support.h $(RECIPE)/less_config.h $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/less_pyxis.o: $(RECIPE)/busybox_pyxis.c $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -DPYXIS_APPLET_LESS -c $< -o $@

$(BUILD)/less.pxe: $(BUILD)/less.o $(BUILD)/less_pyxis.o $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(BUILD)/less.o $(BUILD)/less_pyxis.o $(PYXIS_LDLIBS)

-include $(BUILD)/less.d $(BUILD)/less_pyxis.d

TAR_OBJECTS := $(BUILD)/tar.o $(BUILD)/get_header_tar.o $(BUILD)/tar_header.o $(BUILD)/tar_pyxis.o

$(BUILD)/tar.o: $(SOURCE)/archival/tar.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/get_header_tar.o: $(SOURCE)/archival/libarchive/get_header_tar.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/tar_header.o: $(SOURCE)/archival/chksum_and_xwrite_tar_header.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/tar_pyxis.o: $(RECIPE)/tar_pyxis.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/tar.pxe: $(TAR_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(TAR_OBJECTS) $(PYXIS_LDLIBS)

-include $(TAR_OBJECTS:.o=.d)
