# CMake package for the Pyxis GLU port, installed as lib/cmake/GLU. GLU::GLU
# links the static library with Mesa's OSMesa and the C++ runtime its NURBS
# code needs. Paths resolve from this file, so the development prefix can move.

get_filename_component(_glu_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

if(NOT TARGET GLU::GLU)
  find_path(_glu_gl_include NAMES GL/gl.h REQUIRED)
  find_library(_glu_mesa NAMES OSMesa REQUIRED)
  find_library(_glu_cxx NAMES c++ REQUIRED)
  find_library(_glu_cxxabi NAMES c++abi REQUIRED)
  find_library(_glu_unwind NAMES unwind REQUIRED)
  add_library(GLU::GLU STATIC IMPORTED)
  set_target_properties(GLU::GLU PROPERTIES
    IMPORTED_LOCATION "${_glu_prefix}/lib/libGLU.a"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
    INTERFACE_INCLUDE_DIRECTORIES "${_glu_prefix}/include;${_glu_gl_include}"
    INTERFACE_LINK_LIBRARIES "${_glu_mesa};${_glu_cxx};${_glu_cxxabi};${_glu_unwind}")
endif()

set(GLU_FOUND TRUE)
unset(_glu_prefix)
