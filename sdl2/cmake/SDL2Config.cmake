# CMake package for the Pyxis SDL2 port, installed as lib/cmake/SDL2. It
# provides upstream's static target name; Pyxis has no shared SDL2::SDL2.
# Paths resolve from this file, so the development prefix can move.

get_filename_component(_sdl2_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT TARGET SDL2::SDL2-static)
  add_library(SDL2::SDL2-static STATIC IMPORTED)
  set_target_properties(SDL2::SDL2-static PROPERTIES
    IMPORTED_LOCATION "${_sdl2_prefix}/lib/libSDL2.a"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    INTERFACE_INCLUDE_DIRECTORIES "${_sdl2_prefix}/include;${_sdl2_prefix}/include/SDL2")
endif()

set(SDL2_PREFIX "${_sdl2_prefix}")
set(SDL2_INCLUDE_DIR "${_sdl2_prefix}/include/SDL2")
set(SDL2_INCLUDE_DIRS "${_sdl2_prefix}/include;${_sdl2_prefix}/include/SDL2")
set(SDL2_LIBDIR "${_sdl2_prefix}/lib")
set(SDL2_STATIC_LIBRARIES SDL2::SDL2-static)
set(SDL2_LIBRARIES SDL2::SDL2-static)
set(SDL2_FOUND TRUE)
unset(_sdl2_prefix)
