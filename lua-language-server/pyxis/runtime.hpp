#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <string>

namespace pyxis_luals {
// Program-private glue; this is not an SDK or kernel interface.
lua_State* new_state();
bool close_state(lua_State* state, bool normal);
bool is_stop_error(lua_State* state, int index);
void enable_stop_hook(lua_State* state);
void register_platform(lua_State* state);
void register_concurrency(lua_State* state);
const std::string& runtime_root();
const std::string& state_root();
std::string create_state_directory();
bool remove_state_directory(const std::string& path, std::string& error);
bool stopping();
void request_stop(int status);
int requested_exit_status();
void stop_and_join();
}
