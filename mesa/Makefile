include $(SDK)/share/pyxis.mk

STRIP := $(shell $(CC) -print-prog-name=llvm-strip)
LIBRARY := $(BUILD)/libOSMesa.a

.PHONY: all
all: $(BUILD)/mesa/build.ninja
	ninja -C $(BUILD)/mesa -j8 src/gallium/targets/osmesa/libOSMesa.a
	python3 $(RECIPE)/configure.py merge --build $(BUILD) --ar '$(AR)' \
	  --llvm-development $(LLVM_DEVELOPMENT)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE)/include \
	  -c $(RECIPE)/manual/triangle.c -o $(BUILD)/triangle.o
	$(CXX) $(PYXIS_LDFLAGS) $(PYXIS_START) $(BUILD)/triangle.o $(LIBRARY) \
	  $(PYXIS_CXX_LDLIBS) -o $(BUILD)/mesa-triangle.pxe

$(BUILD)/mesa/build.ninja:
	python3 $(RECIPE)/configure.py configure --source $(SOURCE) --build $(BUILD) \
	  --cc '$(CC)' --cxx '$(CXX)' --ar '$(AR)' \
	  --c-flags '$(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS)' \
	  --cxx-flags '$(PYXIS_CXX_CPPFLAGS) $(PYXIS_CXXFLAGS)' \
	  --link-flags='--sysroot=$(PYXIS_SYSROOT)' \
	  --llvm-development $(LLVM_DEVELOPMENT)

.PHONY: strip-library
strip-library:
	$(STRIP) --strip-debug $(STAGED_LIBRARY)
