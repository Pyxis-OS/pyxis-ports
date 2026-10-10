include $(SDK)/share/pyxis.mk

CKSUM_OBJECTS := $(BUILD)/cksum.o $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/fshut.o
TEE_OBJECTS := $(BUILD)/tee.o $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/ealloc.o \
               $(BUILD)/libutil/writeall.o
UNIQ_OBJECTS := $(BUILD)/uniq.o $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/ealloc.o \
                $(BUILD)/libutil/fshut.o $(BUILD)/libutil/strtonum.o
SHA256SUM_OBJECTS := $(BUILD)/sha256sum.o $(BUILD)/libutil/crypt.o $(BUILD)/libutil/sha256.o \
                     $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/fshut.o

UTF_COMMON := $(BUILD)/libutf/rune.o $(BUILD)/libutf/utf.o $(BUILD)/libutf/runetype.o
WC_OBJECTS := $(BUILD)/wc.o $(BUILD)/libutf/fgetrune.o $(BUILD)/libutf/isspacerune.o \
              $(UTF_COMMON) $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/fshut.o
TAIL_OBJECTS := $(BUILD)/tail.o $(BUILD)/libutil/concat.o $(BUILD)/libutil/ealloc.o \
                $(BUILD)/libutil/eprintf.o $(BUILD)/libutil/strtonum.o \
                $(BUILD)/libutil/writeall.o $(UTF_COMMON)
SORT_OBJECTS := $(BUILD)/sort.o $(BUILD)/libutil/ealloc.o $(BUILD)/libutil/eprintf.o \
                $(BUILD)/libutil/fshut.o $(BUILD)/libutil/getlines.o \
                $(BUILD)/libutil/linecmp.o $(BUILD)/libutil/memmem.o \
                $(BUILD)/libutil/reallocarray.o $(BUILD)/libutil/unescape.o \
                $(BUILD)/libutf/isalnumrune.o $(BUILD)/libutf/isalpharune.o \
                $(BUILD)/libutf/isblankrune.o $(BUILD)/libutf/iscntrlrune.o \
                $(BUILD)/libutf/isdigitrune.o $(BUILD)/libutf/isprintrune.o \
                $(BUILD)/libutf/lowerrune.o $(UTF_COMMON)
GREP_OBJECTS := $(BUILD)/grep.o $(BUILD)/libutil/ealloc.o $(BUILD)/libutil/eprintf.o \
                $(BUILD)/libutil/eregcomp.o $(BUILD)/libutil/fshut.o \
                $(BUILD)/libutil/reallocarray.o

.PHONY: all
all: $(BUILD)/cksum.pxe $(BUILD)/tee.pxe $(BUILD)/uniq.pxe $(BUILD)/sha256sum.pxe \
     $(BUILD)/wc.pxe $(BUILD)/tail.pxe $(BUILD)/sort.pxe $(BUILD)/grep.pxe

$(BUILD)/%.o: $(SOURCE)/%.c $(SOURCE)/util.h $(SOURCE)/arg.h $(SOURCE)/compat.h \
              $(SOURCE)/utf.h $(SOURCE)/text.h $(SOURCE)/queue.h
	@mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -c $< -o $@

$(BUILD)/cksum.pxe: $(CKSUM_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(CKSUM_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/tee.pxe: $(TEE_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(TEE_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/uniq.pxe: $(UNIQ_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(UNIQ_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/sha256sum.pxe: $(SHA256SUM_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(SHA256SUM_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/wc.pxe: $(WC_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(WC_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/tail.pxe: $(TAIL_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(TAIL_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/sort.pxe: $(SORT_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(SORT_OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/grep.pxe: $(GREP_OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(GREP_OBJECTS) $(PYXIS_LDLIBS)

-include $(sort $(CKSUM_OBJECTS:.o=.d) $(TEE_OBJECTS:.o=.d) $(UNIQ_OBJECTS:.o=.d) \
  $(SHA256SUM_OBJECTS:.o=.d) $(WC_OBJECTS:.o=.d) $(TAIL_OBJECTS:.o=.d) \
  $(SORT_OBJECTS:.o=.d) $(GREP_OBJECTS:.o=.d))
