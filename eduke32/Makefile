include $(SDK)/share/pyxis.mk

# EDuke32's own GNU Make build, on the PYXIS platform from 0001: the game only,
# with the classic renderer and none of OpenGL, Polymer, libvpx, Vorbis, FLAC,
# XMP, network play, mimalloc or the startup window. Release flags are
# upstream's, without LTO; the output is not stripped.
#
# The SDK's headers are system directories, searched after EDuke32's own,
# because EDuke32's log.h, keyboard.h and others share names with Pyxis
# headers. LIBS replaces EDuke32's host libraries with SDL2 and the SDK's.
EDUKE32_OPTIONS := PLATFORM=PYXIS SDLCONFIG= LTO=0 STRIP= \
  USE_OPENGL=0 POLYMER=0 USE_LIBVPX=0 HAVE_VORBIS=0 HAVE_FLAC=0 HAVE_XMP=0 \
  NETCODE=0 USE_MIMALLOC=0 STARTUP_WINDOW=0 HAVE_GTK2=0
EDUKE32_CFLAGS := --sysroot=$(PYXIS_SYSROOT) -nostdinc \
  -isystem $(shell $(CC) -print-file-name=include) \
  -isystem $(PYXIS_SYSROOT)/usr/include \
  -isystem $(SDL2)/include -isystem $(SDL2)/include/SDL2 \
  -fno-stack-protector -fno-pic -fno-pie -mno-red-zone -march=x86-64
EDUKE32_CXXFLAGS := -isystem $(PYXIS_SYSROOT)/usr/include/c++/v1

.PHONY: all
all:
	$(MAKE) -C $(SOURCE) eduke32 $(EDUKE32_OPTIONS) obj=$(BUILD)/obj \
	  CC=$(CC) AR=$(AR) RANLIB=$(shell $(CC) -print-prog-name=llvm-ranlib) \
	  CFLAGS="$(EDUKE32_CFLAGS)" CXXFLAGS="$(EDUKE32_CXXFLAGS)" \
	  LDFLAGS="$(PYXIS_LDFLAGS) $(PYXIS_START)" LIBDIRS="-L$(SDL2)/lib" \
	  LIBS="-lSDL2 $(PYXIS_CXX_LDLIBS)"
