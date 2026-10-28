include $(SDK)/share/pyxis.mk

# Applets are selected separately; no default BusyBox configuration is built.
# Recipe headers select the applet profile and declare the native adapters.
# getopt32, list/numeric/string helpers and tar traversal use upstream sources.
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(RECIPE) -I$(SOURCE)/include
GETOPT_OBJECTS := $(BUILD)/getopt32.o $(BUILD)/llist.o $(BUILD)/xatonum.o $(BUILD)/printf_number.o $(BUILD)/tar_string.o $(BUILD)/tar_compare.o $(BUILD)/last_char_is.o $(BUILD)/skip_whitespace.o
READ_KEY_OBJECTS := $(BUILD)/read_key.o $(BUILD)/read.o $(BUILD)/safe_poll.o $(BUILD)/poll_with_signals.o $(BUILD)/signals.o $(BUILD)/tar_perror.o $(BUILD)/console_verror.o
OBJECTS := $(BUILD)/vi.o $(BUILD)/busybox_pyxis.o $(GETOPT_OBJECTS) $(READ_KEY_OBJECTS)

.PHONY: all
all: $(BUILD)/vi.pxe $(BUILD)/less.pxe $(BUILD)/tar.pxe $(BUILD)/printf.pxe

# Upstream relies on unused parameters, signed/unsigned comparisons and
# intentional case fallthrough that BusyBox's own flags do not report.
VI_WARNINGS := -Wno-unused-parameter -Wno-sign-compare -Wno-implicit-fallthrough

$(addprefix $(BUILD)/,$(addsuffix .o,read_key read safe_poll poll_with_signals signals)): $(BUILD)/%.o: $(SOURCE)/libbb/%.c $(RECIPE)/libbb.h $(RECIPE)/vi_config.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

# The console adapter retains its fatal wrappers and output-flush policy.
$(BUILD)/console_verror.o: $(SOURCE)/libbb/verror_msg.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -DPYXIS_CONSOLE_APPLET -c $< -o $@

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

$(BUILD)/less.pxe: $(BUILD)/less.o $(BUILD)/less_pyxis.o $(BUILD)/realloc_vector.o $(GETOPT_OBJECTS) $(READ_KEY_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(BUILD)/less.o $(BUILD)/less_pyxis.o $(BUILD)/realloc_vector.o $(GETOPT_OBJECTS) $(READ_KEY_OBJECTS) $(PYXIS_LDLIBS)

-include $(BUILD)/less.d $(BUILD)/less_pyxis.d

TAR_HELPER_NAMES := init_handle filter_accept_all filter_accept_reject_list find_list_entry header_skip header_list header_verbose_list data_skip data_align data_extract_to_stdout data_extract_all seek_by_jump seek_by_read unsafe_symlink_target
TAR_HELPERS := $(addprefix $(BUILD)/archive_,$(addsuffix .o,$(TAR_HELPER_NAMES))) $(BUILD)/tar_recursive.o $(BUILD)/tar_subpath.o $(BUILD)/tar_path.o $(BUILD)/tar_printable.o $(BUILD)/tar_autostring.o $(BUILD)/tar_modes.o $(BUILD)/tar_copyfd.o $(BUILD)/tar_read.o $(BUILD)/tar_full_write.o $(BUILD)/tar_safe_write.o $(BUILD)/tar_directory.o $(BUILD)/tar_libbb.o $(BUILD)/tar_verror.o $(BUILD)/tar_perror.o $(BUILD)/tar_die.o $(BUILD)/tar_errorret.o
TAR_OBJECTS := $(BUILD)/tar.o $(BUILD)/get_header_tar.o $(BUILD)/tar_header.o $(BUILD)/tar_prefix.o $(BUILD)/tar_pyxis.o $(GETOPT_OBJECTS) $(TAR_HELPERS)

$(BUILD)/tar.o: $(SOURCE)/archival/tar.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/get_header_tar.o: $(SOURCE)/archival/libarchive/get_header_tar.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/tar_header.o: $(SOURCE)/archival/chksum_and_xwrite_tar_header.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/tar_prefix.o: $(SOURCE)/archival/libarchive/unsafe_prefix.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/tar_string.o: $(SOURCE)/libbb/safe_strncpy.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/tar_pyxis.o: $(RECIPE)/tar_pyxis.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/tar.pxe: $(TAR_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(TAR_OBJECTS) $(PYXIS_LDLIBS)

-include $(TAR_OBJECTS:.o=.d)

PRINTF_OBJECTS := $(BUILD)/printf.o $(BUILD)/printf_escape.o \
                  $(BUILD)/printf_number.o $(BUILD)/printf_pyxis.o

$(BUILD)/printf.o: $(SOURCE)/coreutils/printf.c $(RECIPE)/printf_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/printf_escape.o: $(SOURCE)/libbb/process_escape_sequence.c $(RECIPE)/printf_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/printf_number.o: $(SOURCE)/libbb/bb_strtonum.c $(RECIPE)/printf_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/printf_pyxis.o: $(RECIPE)/printf_pyxis.c $(RECIPE)/printf_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/printf.pxe: $(PRINTF_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(PRINTF_OBJECTS) $(PYXIS_LDLIBS)

-include $(PRINTF_OBJECTS:.o=.d)

$(BUILD)/getopt32.o: $(SOURCE)/libbb/getopt32.c $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/llist.o: $(SOURCE)/libbb/llist.c $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/xatonum.o: $(SOURCE)/libbb/xatonum.c $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/archive_%.o: $(SOURCE)/archival/libarchive/%.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@

$(BUILD)/tar_recursive.o: $(SOURCE)/libbb/recursive_action.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_subpath.o: $(SOURCE)/libbb/concat_subpath_file.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_compare.o: $(SOURCE)/libbb/compare_string_array.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_printable.o: $(SOURCE)/libbb/printable_string.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_autostring.o: $(SOURCE)/libbb/auto_string.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_modes.o: $(SOURCE)/libbb/mode_string.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@

$(BUILD)/last_char_is.o: $(SOURCE)/libbb/last_char_is.c $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@
$(BUILD)/skip_whitespace.o: $(SOURCE)/libbb/skip_whitespace.c $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@
$(BUILD)/tar_path.o: $(SOURCE)/libbb/concat_path_file.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@

$(BUILD)/realloc_vector.o: $(SOURCE)/libbb/xrealloc_vector.c $(RECIPE)/libbb.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -c $< -o $@

$(BUILD)/tar_libbb.o: $(RECIPE)/tar_libbb.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@
$(BUILD)/tar_copyfd.o: $(SOURCE)/libbb/copyfd.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_read.o: $(SOURCE)/libbb/read.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_full_write.o: $(SOURCE)/libbb/full_write.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_safe_write.o: $(SOURCE)/libbb/safe_write.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_directory.o: $(SOURCE)/libbb/make_directory.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@

$(BUILD)/tar_verror.o: $(SOURCE)/libbb/verror_msg.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_perror.o: $(SOURCE)/libbb/perror_msg.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_die.o: $(SOURCE)/libbb/xfunc_die.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
$(BUILD)/tar_errorret.o: $(SOURCE)/libbb/default_error_retval.c $(RECIPE)/tar_support.h
	$(CC) $(CPPFLAGS) $(PYXIS_CFLAGS) $(VI_WARNINGS) -DPYXIS_APPLET_TAR -c $< -o $@
