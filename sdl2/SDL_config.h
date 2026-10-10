/* SDL 2.32 configuration for Pyxis. It replaces upstream's SDL_config.h
 * platform dispatcher in the installed headers, so the library and its
 * consumers see the same settings. */
#ifndef SDL_config_h_
#define SDL_config_h_

#include "SDL_platform.h"

#define SIZEOF_VOIDP 8
#define HAVE_GCC_ATOMICS 1

/* libc headers and functions; SDL supplies its own versions of the rest.
 * SDL_iconv stays SDL's own: libc's iconv lacks the UCS-2, UCS-4 and UTF-32
 * forms SDL converts text through. */
#define HAVE_LIBC 1
#define HAVE_CTYPE_H 1
#define HAVE_FLOAT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MATH_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_WCHAR_H 1

#define HAVE_MALLOC 1
#define HAVE_CALLOC 1
#define HAVE_REALLOC 1
#define HAVE_FREE 1
#define HAVE_GETENV 1
#define HAVE_SETENV 1
#define HAVE_QSORT 1
#define HAVE_ABS 1
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
#define HAVE_WCSLEN 1
#define HAVE_STRLEN 1
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_STRTOK_R 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_STRCASESTR 1
#define HAVE_SSCANF 1
#define HAVE_VSSCANF 1
#define HAVE_VSNPRINTF 1
#define HAVE_ATAN 1
#define HAVE_ATAN2 1
#define HAVE_CEIL 1
#define HAVE_CEILF 1
#define HAVE_COS 1
#define HAVE_FABS 1
#define HAVE_FLOOR 1
#define HAVE_FMOD 1
#define HAVE_POW 1
#define HAVE_ROUND 1
#define HAVE_ROUNDF 1
#define HAVE_SCALBN 1
#define HAVE_SIN 1
#define HAVE_SQRT 1
#define HAVE_SQRTF 1
#define HAVE_TAN 1
#define HAVE_TRUNC 1
#define HAVE_FSEEKO 1
#define HAVE_SETJMP 1
#define HAVE__EXIT 1

/* Facilities Pyxis does not provide yet; SDL reports them as unsupported. */
#define SDL_AUDIO_DISABLED 1
#define SDL_HAPTIC_DISABLED 1
#define SDL_HIDAPI_DISABLED 1
#define SDL_SENSOR_DISABLED 1
#define SDL_LOADSO_DISABLED 1
#define SDL_THREADS_DISABLED 1
#define SDL_POWER_DISABLED 1

/* Joysticks initialize and report zero devices. */
#define SDL_JOYSTICK_DUMMY 1

#define SDL_VIDEO_DRIVER_PYXIS 1
#define SDL_VIDEO_RENDER_SW 1
#define SDL_TIMER_PYXIS 1
#define SDL_FILESYSTEM_PYXIS 1

#endif /* SDL_config_h_ */
