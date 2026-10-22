include(CMakeFindDependencyMacro)
find_dependency(SDL2 CONFIG)
get_filename_component(_raylib_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(NOT TARGET raylib)
  add_library(raylib STATIC IMPORTED)
  set_target_properties(raylib PROPERTIES
    IMPORTED_LOCATION "${_raylib_prefix}/lib/libraylib.a"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    INTERFACE_INCLUDE_DIRECTORIES "${_raylib_prefix}/include"
    INTERFACE_LINK_LIBRARIES SDL2::SDL2-static)
endif()
unset(_raylib_prefix)
