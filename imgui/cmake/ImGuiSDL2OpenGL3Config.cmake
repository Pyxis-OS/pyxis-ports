include(CMakeFindDependencyMacro)
find_dependency(ImGui CONFIG)
find_dependency(SDL2OSMesa CONFIG)
get_filename_component(_imgui_backend_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT TARGET ImGui::SDL2OpenGL3)
  add_library(ImGui::SDL2OpenGL3 INTERFACE IMPORTED)
  set_target_properties(ImGui::SDL2OpenGL3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_imgui_backend_prefix}/include"
    INTERFACE_LINK_LIBRARIES "${_imgui_backend_prefix}/lib/libimgui-sdl2.a;${_imgui_backend_prefix}/lib/libimgui-opengl3.a;ImGui::ImGui;SDL2::SDL2-osmesa-static")
endif()
set(ImGuiSDL2OpenGL3_FOUND TRUE)
unset(_imgui_backend_prefix)
