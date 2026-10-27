#include "runtime.hpp"

#include <cstdio>
#include <memory>
#include <stdexcept>

extern "C" lua_State* bee_lua_newstate();
extern "C" int luaopen_lpeglabel(lua_State* state);

namespace pyxis_luals {
namespace {
std::string instance;

void string_global(lua_State* state, const char* name, const std::string& value)
{
    lua_pushlstring(state, value.data(), value.size());
    lua_setglobal(state, name);
}

int initialize_platform(lua_State* state)
{
    register_platform(state);
    register_concurrency(state);
    luaL_getsubtable(state, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    lua_pushcfunction(state, luaopen_lpeglabel);
    lua_setfield(state, -2, "lpeglabel");
    lua_pop(state, 1);

    string_global(state, "LUALS_ROOT", runtime_root());
    string_global(state, "LUALS_STATE_ROOT", state_root());
    string_global(state, "LOGPATH", state_root() + "/log");
    string_global(state, "METAPATH", state_root() + "/meta");
    if (luaL_dostring(state, "ROOT = require('bee.filesystem').path(LUALS_ROOT)") != LUA_OK) {
        return lua_error(state);
    }
    return 0;
}

int initialize(lua_State* state)
{
    luaL_openlibs(state);
    return initialize_platform(state);
}

template<int (*Function)(lua_State*)>
int protected_native(lua_State* state)
{
    try {
        return Function(state);
    } catch (const std::exception& error) {
        lua_pushstring(state, error.what());
    }
    return lua_error(state);
}


}

const std::string& runtime_root()
{
    static const std::string value = "app://share/lua-language-server";
    return value;
}

const std::string& state_root() { return instance; }

void close_state(lua_State* state, bool normal)
{
    if (!state) return;
    lua_sethook(state, nullptr, 0, 0);
    lua_pyxis_io_shutdown(state, normal ? 1 : 0);
    lua_close(state);
}

lua_State* new_state()
{
    lua_State* state = bee_lua_newstate();
    if (!state) throw std::runtime_error("Lua state allocation failed");
    auto abandon = [](lua_State* value) { close_state(value, false); };
    std::unique_ptr<lua_State, decltype(abandon)> owned(state, abandon);
    lua_pushcfunction(state, protected_native<initialize>);
    if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
        const char* message = lua_tostring(state, -1);
        std::string error = message ? message : "Lua state initialization failed";
        throw std::runtime_error(error);
    }
    enable_stop_hook(state);
    return owned.release();
}

void initialize_instance()
{
    instance = create_state_directory();
}

void initialize_main(lua_State* state)
{
    lua_pushcfunction(state, protected_native<initialize_platform>);
    lua_call(state, 0, 0);
    enable_stop_hook(state);
}

bool cleanup_instance()
{
    if (instance.empty()) return true;
    std::string error;
    if (remove_state_directory(instance, error)) return true;
    std::fprintf(stderr, "lua-language-server: state cleanup: %s\n", error.c_str());
    return false;
}

}
