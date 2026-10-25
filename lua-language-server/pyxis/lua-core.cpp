// Keep the Lua C API linkage, with its supported C++ exception unwinding so
// Lua errors cannot longjmp past native binding owners.
#include <lua.hpp>
#define MAKE_LIB
#include <onelua.c>
