include $(SDK)/share/pyxis.mk

CPPFLAGS := $(PYXIS_CXX_CPPFLAGS) -I$(SOURCE) -I$(SOURCE)/backends -I$(SDL2)/include/SDL2 -I$(MESA)/include
CXXFLAGS := $(PYXIS_CXXFLAGS) -ffunction-sections -fdata-sections
CORE := imgui imgui_draw imgui_tables imgui_widgets imgui_demo
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(CORE)))
GLFLAGS := -DIMGUI_IMPL_OPENGL_LOADER_CUSTOM -DGL_GLEXT_PROTOTYPES=1 -include SDL_opengl.h

.PHONY: all
all: $(BUILD)/libimgui.a $(BUILD)/libimgui-sdl2.a $(BUILD)/libimgui-opengl3.a $(BUILD)/imgui-demo.pxe

$(BUILD)/%.o: $(SOURCE)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/imgui_impl_sdl2.o: $(SOURCE)/backends/imgui_impl_sdl2.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Official custom-loader mode: symbols resolve to the selected static GL closure.
$(BUILD)/imgui_impl_opengl3.o: $(SOURCE)/backends/imgui_impl_opengl3.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GLFLAGS) -c $< -o $@

$(BUILD)/libimgui.a: $(OBJECTS)
	$(AR) rcs $@ $^

$(BUILD)/libimgui-sdl2.a: $(BUILD)/imgui_impl_sdl2.o
	$(AR) rcs $@ $^

$(BUILD)/libimgui-opengl3.a: $(BUILD)/imgui_impl_opengl3.o
	$(AR) rcs $@ $^

$(BUILD)/demo.o: $(RECIPE)/demo.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGL_GLEXT_PROTOTYPES=1 -c $< -o $@

$(BUILD)/imgui-demo.pxe: $(BUILD)/demo.o $(BUILD)/libimgui.a $(BUILD)/libimgui-sdl2.a $(BUILD)/libimgui-opengl3.a $(SDL2_OSMESA)/lib/libSDL2-osmesa.a $(MESA)/lib/libOSMesa.a $(PYXIS_CXX_LIBRARIES) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CXX) $(PYXIS_LDFLAGS) -Wl,--gc-sections $(PYXIS_START) $< \
	  $(BUILD)/libimgui-sdl2.a $(BUILD)/libimgui-opengl3.a $(BUILD)/libimgui.a \
	  $(SDL2_OSMESA)/lib/libSDL2-osmesa.a $(MESA)/lib/libOSMesa.a $(PYXIS_CXX_LDLIBS) -o $@

-include $(OBJECTS:.o=.d) $(BUILD)/imgui_impl_sdl2.d $(BUILD)/imgui_impl_opengl3.d $(BUILD)/demo.d
