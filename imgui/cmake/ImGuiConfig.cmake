get_filename_component(_imgui_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT TARGET ImGui::ImGui)
  find_library(_imgui_cxx NAMES c++ REQUIRED)
  find_library(_imgui_cxxabi NAMES c++abi REQUIRED)
  find_library(_imgui_unwind NAMES unwind REQUIRED)
  add_library(ImGui::ImGui STATIC IMPORTED)
  set_target_properties(ImGui::ImGui PROPERTIES
    IMPORTED_LOCATION "${_imgui_prefix}/lib/libimgui.a"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    INTERFACE_INCLUDE_DIRECTORIES "${_imgui_prefix}/include"
    INTERFACE_LINK_LIBRARIES "${_imgui_cxx};${_imgui_cxxabi};${_imgui_unwind}")
endif()
set(ImGui_VERSION "1.91.9b")
set(ImGui_FOUND TRUE)
unset(_imgui_prefix)
