# Select this target instead of SDL3::SDL3-static. Shared headers and covered
# backend sources come from sdl3-dev; Mesa and its C++ runtime are dependencies.
include(CMakeFindDependencyMacro)
find_dependency(SDL3 CONFIG)
get_filename_component(_sdl3_osmesa_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT TARGET SDL3::SDL3-osmesa-static)
  find_path(_sdl3_osmesa_include NAMES GL/osmesa.h REQUIRED)
  find_library(_sdl3_osmesa_mesa NAMES OSMesa REQUIRED)
  find_library(_sdl3_osmesa_cxx NAMES c++ REQUIRED)
  find_library(_sdl3_osmesa_cxxabi NAMES c++abi REQUIRED)
  find_library(_sdl3_osmesa_unwind NAMES unwind REQUIRED)
  add_library(SDL3::SDL3-osmesa-static STATIC IMPORTED)
  set_target_properties(SDL3::SDL3-osmesa-static PROPERTIES
    IMPORTED_LOCATION "${_sdl3_osmesa_prefix}/lib/libSDL3-osmesa.a"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    INTERFACE_INCLUDE_DIRECTORIES "${_sdl3_osmesa_include};$<TARGET_PROPERTY:SDL3::SDL3-static,INTERFACE_INCLUDE_DIRECTORIES>"
    INTERFACE_LINK_LIBRARIES "${_sdl3_osmesa_mesa};${_sdl3_osmesa_cxx};${_sdl3_osmesa_cxxabi};${_sdl3_osmesa_unwind}")
endif()

set(SDL3OSMesa_FOUND TRUE)
unset(_sdl3_osmesa_prefix)
