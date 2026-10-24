include $(SDK)/share/pyxis.mk
.PHONY: all
all:
	cmake -S $(SOURCE) -B $(BUILD) -G "Unix Makefiles" \
	  -DCMAKE_TOOLCHAIN_FILE=$(PYXIS_SDK)/share/pyxis.cmake \
	  -DPYXIS_CROSS_COMPILE=$(CROSS_COMPILE) \
	  -DJSON_BuildTests=OFF -DJSON_Install=ON -DJSON_MultipleHeaders=ON
