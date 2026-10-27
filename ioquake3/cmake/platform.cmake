# This profile uses the native SDK platform; it does not claim Unix facilities.
if(NOT PYXIS_PORT_DIR)
    message(FATAL_ERROR "PYXIS_PORT_DIR must name the ioquake3 recipe")
endif()
if(USE_RENDERER_DLOPEN OR BUILD_GAME_LIBRARIES OR BUILD_RENDERER_GL2 OR
   BUILD_SERVER OR USE_HTTP OR USE_OPENAL OR USE_VOIP OR USE_MUMBLE OR USE_CODEC_OPUS)
    message(FATAL_ERROR "Unsupported feature enabled in the static Pyxis profile")
endif()

set(SYSTEM_PLATFORM_SOURCES
    ${PYXIS_PORT_DIR}/pyxis/sys_pyxis.c
    ${SOURCE_DIR}/sys/con_passive.c)
# The adapters use the engine's own platform declarations.
include_directories(${PYXIS_PORT_DIR} ${SOURCE_DIR}/sys ${SOURCE_DIR}/qcommon)
