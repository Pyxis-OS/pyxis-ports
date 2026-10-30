# CMake package for the Pyxis SDL3 port, installed as lib/cmake/SDL3. It
# provides upstream's static target and SDL3::SDL3 linking it; Pyxis has no
# shared SDL3. Paths resolve from this file, so the development prefix can move.

get_filename_component(_sdl3_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT TARGET SDL3::SDL3-static)
  add_library(SDL3::SDL3-static STATIC IMPORTED)
  set_target_properties(SDL3::SDL3-static PROPERTIES
    IMPORTED_LOCATION "${_sdl3_prefix}/lib/libSDL3.a"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    INTERFACE_INCLUDE_DIRECTORIES "${_sdl3_prefix}/include")
endif()
if(NOT TARGET SDL3::SDL3)
  add_library(SDL3::SDL3 INTERFACE IMPORTED)
  set_target_properties(SDL3::SDL3 PROPERTIES INTERFACE_LINK_LIBRARIES SDL3::SDL3-static)
endif()

set(SDL3_FOUND TRUE)
unset(_sdl3_prefix)
