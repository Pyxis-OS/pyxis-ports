/* Links configuration for Pyxis, written by hand: configure cannot run
 * against the freestanding SDK. Only facilities Pyxis libc provides are
 * listed; everything else takes Links' fallback or the Pyxis platform block.
 *
 * Part of the Pyxis port of Links; distributed under the GPL like Links
 * (see COPYING in the Links source). */
#define PACKAGE "links"
#define VERSION "2.30"
#define DEBUGLEVEL 0
#define ENABLE_UTF8 1

#define STDC_HEADERS 1
#define HAVE_STDLIB_H_X 1
#define HAVE_STDARG_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_LIMITS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_FCNTL_H 1
#define HAVE_TIME_H 1
#define HAVE_MATH_H 1
#define HAVE_SETJMP_H 1
#define HAVE_DIRENT_H 1

#define SIZEOF_UNSIGNED 4
#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_UNSIGNED_SHORT 2
#define HAVE_LONG_LONG 1
#define HAVE_VOLATILE 1
#define HAVE_RESTRICT 1
#define HAVE___RESTRICT 1
#define HAVE_ERRNO 1
#define C_LITTLE_ENDIAN 1
#define HAVE_GCC_ASSEMBLER 1
#define HAVE___BUILTIN_ADD_OVERFLOW 1
#define HAVE___BUILTIN_CLZ 1
#define RENAME_OVER_EXISTING_FILES 1

#define HAVE_CALLOC 1
#define HAVE_GETCWD 1
#define HAVE_GMTIME 1
#define HAVE_MEMCHR 1
#define HAVE_MEMCMP 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMRCHR 1
#define HAVE_MEMSET 1
#define HAVE_SNPRINTF 1
#define HAVE_VPRINTF 1
#define HAVE_STRCHR 1
#define HAVE_STRCMP 1
#define HAVE_STRCPY 1
#define HAVE_STRCSPN 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRLEN 1
#define HAVE_STRNCMP 1
#define HAVE_STRNCPY 1
#define HAVE_STRNLEN 1
#define HAVE_STRRCHR 1
#define HAVE_STRSPN 1
#define HAVE_STRSTR 1
#define HAVE_STRTOD 1
#define HAVE_STRTOL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOUL 1

/* A port-local header under include/ supplies fd_set for pyxis_select. */
#define HAVE_SYS_SELECT_H 1
/* Port-local socket declarations (include/sys/socket.h); calls fail. */
#define HAVE_SOCKLEN_T 1
#define HAVE_GETHOSTBYNAME 1
