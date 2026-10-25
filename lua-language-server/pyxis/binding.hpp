#pragma once

#include "runtime.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace pyxis_luals {
int open_filesystem(lua_State* state);

inline void system_error(const char* operation)
{
    int error = errno;
    throw std::runtime_error(std::string(operation) + ": " + std::strerror(error));
}

template<int (*Function)(lua_State*)>
int binding(lua_State* state)
{
    try {
        return Function(state);
    } catch (const std::exception& error) {
        lua_pushstring(state, error.what());
    }
    // No C++ owners remain across Lua's error transfer.
    return lua_error(state);
}
}
