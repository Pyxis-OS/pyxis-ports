#include "runtime.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

extern "C" lua_State* bee_lua_newstate();
extern "C" int luaopen_lpeglabel(lua_State* state);

#ifndef LUALS_VERSION
#error "Set LUALS_VERSION from the pinned recipe metadata"
#endif

namespace pyxis_luals {
namespace {
std::string instance;

void string_global(lua_State* state, const char* name, const std::string& value)
{
    lua_pushlstring(state, value.data(), value.size());
    lua_setglobal(state, name);
}

int initialize(lua_State* state)
{
    luaL_openlibs(state);
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
    lua_getglobal(state, "package");
    std::string path = runtime_root() + "/script/?.lua;" + runtime_root() + "/script/?/init.lua";
    lua_pushlstring(state, path.data(), path.size());
    lua_setfield(state, -2, "path");
    lua_pushliteral(state, "");
    lua_setfield(state, -2, "cpath");
    lua_pop(state, 1);
    if (luaL_dostring(state, "ROOT = require('bee.filesystem').path(LUALS_ROOT)") != LUA_OK) {
        return lua_error(state);
    }
    return 0;
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

struct invocation {
    int argc;
    char** argv;
};

int run(lua_State* state)
{
    auto* call = static_cast<invocation*>(lua_touserdata(state, lua_upvalueindex(1)));
    lua_createtable(state, call->argc, 1);
    string_global(state, "LUALS_ROOT", runtime_root());
    std::string main = runtime_root() + "/main.lua";
    lua_pushlstring(state, main.data(), main.size());
    lua_rawseti(state, -2, 0);
    for (int i = 1; i < call->argc; ++i) {
        lua_pushstring(state, call->argv[i]);
        lua_rawseti(state, -2, i);
    }
    lua_setglobal(state, "arg");
    if (luaL_loadfile(state, main.c_str()) != LUA_OK) {
        return lua_error(state);
    }
    lua_call(state, 0, 0);
    return 0;
}
}

const std::string& runtime_root()
{
    static const std::string value = "app://share/lua-language-server";
    return value;
}

const std::string& state_root() { return instance; }

bool close_state(lua_State* state, bool normal)
{
    if (!state) return true;
    lua_sethook(state, nullptr, 0, 0);
    int failed = 0;
    lua_pyxis_io_shutdown(state, normal ? 1 : 0, &failed);
    lua_close(state);
    return failed == 0;
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
}

int main(int argc, char** argv)
{
    using namespace pyxis_luals;
    if (argc == 2 && std::strcmp(argv[1], "--version") == 0) {
        return std::printf("%s\n", LUALS_VERSION) < 0 ? 1 : 0;
    }
    if (argc == 2 && std::strcmp(argv[1], "--help") == 0) {
        return std::puts("lua-language-server [--stdio] [--locale=ID] [--loglevel=LEVEL]\n"
            "Pyxis: byte-pipe stdio LSP, JSON/JSONC settings, per-instance state.") < 0 ? 1 : 0;
    }

    lua_State* state = nullptr;
    try {
        instance = create_state_directory();
        state = new_state();
        invocation call{argc, argv};
        lua_pushlightuserdata(state, &call);
        lua_pushcclosure(state, protected_native<run>, 1);
        int status = lua_pcall(state, 0, 0, 0);
        if (status != LUA_OK && !is_stop_error(state, -1)) {
            const char* message = lua_tostring(state, -1);
            std::fprintf(stderr, "lua-language-server: %s\n", message ? message : "Lua execution failed");
            request_stop(1);
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "lua-language-server: %s\n", error.what());
        request_stop(1);
    }
    stop_and_join();
    if (!close_state(state, requested_exit_status() == 0)) {
        request_stop(1);
    }
    if (!instance.empty()) {
        std::string error;
        if (!remove_state_directory(instance, error)) {
            std::fprintf(stderr, "lua-language-server: state cleanup: %s\n", error.c_str());
            request_stop(1);
        }
    }
    return requested_exit_status();
}
