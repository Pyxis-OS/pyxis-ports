# Select this target instead of SDL2::SDL2-static. Shared headers and covered
# backend sources come from sdl2-dev; Mesa and its C++ runtime are dependencies.
include(CMakeFindDependencyMacro)
find_dependency(SDL2 CONFIG)
get_filename_component(_sdl2_osmesa_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT TARGET SDL2::SDL2-osmesa-static)
  find_path(_sdl2_osmesa_include NAMES GL/osmesa.h REQUIRED)
  find_library(_sdl2_osmesa_mesa NAMES OSMesa REQUIRED)
  find_library(_sdl2_osmesa_cxx NAMES c++ REQUIRED)
  find_library(_sdl2_osmesa_cxxabi NAMES c++abi REQUIRED)
  find_library(_sdl2_osmesa_unwind NAMES unwind REQUIRED)
  add_library(SDL2::SDL2-osmesa-static STATIC IMPORTED)
  set_target_properties(SDL2::SDL2-osmesa-static PROPERTIES
    IMPORTED_LOCATION "${_sdl2_osmesa_prefix}/lib/libSDL2-osmesa.a"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS};${_sdl2_osmesa_include}"
    INTERFACE_LINK_LIBRARIES "${_sdl2_osmesa_mesa};${_sdl2_osmesa_cxx};${_sdl2_osmesa_cxxabi};${_sdl2_osmesa_unwind}")
endif()

set(SDL2OSMesa_FOUND TRUE)
unset(_sdl2_osmesa_prefix)
