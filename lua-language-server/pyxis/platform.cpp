#include "binding.hpp"

#include <climits>
#include <ctime>

#define PYXIS_STRINGIFY_INNER(value) #value
#define PYXIS_STRINGIFY(value) PYXIS_STRINGIFY_INNER(value)

namespace pyxis_luals {
namespace {
int unsupported(lua_State*)
{
    throw std::runtime_error("operation is unavailable in the Pyxis LuaLS profile");
}

int open_sys(lua_State* state)
{
    static const luaL_Reg methods[] = {
        {"exe_path", binding<unsupported>}, {"dll_path", binding<unsupported>},
        {"fullpath", binding<unsupported>}, {nullptr, nullptr},
    };
    luaL_newlib(state, methods);
    return 1;
}

void string_field(lua_State* state, const char* name, const char* value)
{
    lua_pushstring(state, value);
    lua_setfield(state, -2, name);
}

int open_platform(lua_State* state)
{
    lua_newtable(state);
    string_field(state, "os", "pyxis");
#if defined(__x86_64__)
    string_field(state, "Arch", "x86_64");
#else
#error "LuaLS architecture identification needs the actual Pyxis target"
#endif
    string_field(state, "Compiler", "clang");
    string_field(state, "CompilerVersion", "Clang " PYXIS_STRINGIFY(__clang_major__) "."
        PYXIS_STRINGIFY(__clang_minor__) "." PYXIS_STRINGIFY(__clang_patchlevel__));
    string_field(state, "CRT", "libc++");
    string_field(state, "CRTVersion", "libc++ " PYXIS_STRINGIFY(_LIBCPP_VERSION));
#if defined(NDEBUG)
    lua_pushboolean(state, false);
#else
    lua_pushboolean(state, true);
#endif
    lua_setfield(state, -2, "DEBUG");
    // No numeric OS version is synthesized. The bounded runtime gates its uses.
    return 1;
}

int clock_milliseconds(lua_State* state, clockid_t clock)
{
    struct timespec now;
    if (clock_gettime(clock, &now) != 0) system_error("clock_gettime");
    constexpr lua_Integer milliseconds = 1000;
    if (now.tv_nsec < 0 || now.tv_nsec >= 1000000000) {
        throw std::runtime_error("invalid native clock fraction");
    }
    if (now.tv_sec > LUA_MAXINTEGER / milliseconds || now.tv_sec < LUA_MININTEGER / milliseconds) {
        throw std::runtime_error("clock value exceeds Lua integer milliseconds");
    }
    lua_Integer value = now.tv_sec * milliseconds;
    lua_Integer fraction = now.tv_nsec / 1000000;
    if (fraction < 0 || fraction >= milliseconds || value > LUA_MAXINTEGER - fraction) {
        throw std::runtime_error("invalid or overflowing native clock value");
    }
    lua_pushinteger(state, value + fraction);
    return 1;
}

int wall_time(lua_State* state) { return clock_milliseconds(state, CLOCK_REALTIME); }
int monotonic_time(lua_State* state) { return clock_milliseconds(state, CLOCK_MONOTONIC); }

int open_time(lua_State* state)
{
    static const luaL_Reg methods[] = {
        {"time", binding<wall_time>}, {"monotonic", binding<monotonic_time>},
        {"thread", binding<unsupported>}, {nullptr, nullptr},
    };
    luaL_newlib(state, methods);
    return 1;
}
}

void register_platform(lua_State* state)
{
    luaL_getsubtable(state, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    lua_pushcfunction(state, binding<open_filesystem>);
    lua_setfield(state, -2, "bee.filesystem");
    lua_pushcfunction(state, binding<open_sys>);
    lua_setfield(state, -2, "bee.sys");
    lua_pushcfunction(state, binding<open_platform>);
    lua_setfield(state, -2, "bee.platform");
    lua_pushcfunction(state, binding<open_time>);
    lua_setfield(state, -2, "bee.time");
    lua_pop(state, 1);
}
}
