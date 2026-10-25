#include "runtime.hpp"
#include <3rd/lua-seri/lua-seri.h>

extern "C" {
#include <abi/handle.h>
#include <abi/pipe.h>
#include <abi/wait.h>
#include <environment.h>
#include <handle.h>
#include <pipe.h>
#include <pyxis/descriptor.h>
#include <startup.h>
#include <wait.h>
#include <time.h>
#include <unistd.h>
}

#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace pyxis_luals {
namespace {
// Program-private provisional budgets, to be qualified on public fixtures.
constexpr size_t message_bytes = 16 * 1024 * 1024;
constexpr size_t channel_bytes = 64 * 1024 * 1024;
constexpr size_t channel_items = 256;
constexpr size_t total_bytes = 256 * 1024 * 1024;
constexpr size_t total_items = 4096;
constexpr size_t table_limit = 65536;
constexpr size_t value_limit = 1024 * 1024;
constexpr size_t frame_bytes = 4 * 1024 * 1024;
constexpr size_t worker_limit = 8;
constexpr size_t channel_limit = 32;
constexpr int read_event = 1;
const char* stop_error = "LuaLS native runtime stopped";
char stop_sentinel;
const char* channel_meta = "pyxis.luals.channel";
const char* selector_meta = "pyxis.luals.selector";
const char* worker_meta = "pyxis.luals.worker";

struct message {
    void* bytes = nullptr;
    size_t charge = 0;
};
struct channel {
    std::deque<message> queue;
    size_t bytes = 0;
    size_t items = 0; // Includes admitted in-flight serialization.
};
struct selector {
    std::vector<std::shared_ptr<channel>> channels;
};
struct worker {
    std::thread thread;
    bool joined = false;
};
struct runtime {
    std::mutex mutex;
    std::condition_variable changed;
    std::map<std::string, std::shared_ptr<channel>> channels;
    std::vector<std::shared_ptr<worker>> workers;
    std::deque<std::string> errors;
    size_t bytes = 0;
    size_t items = 0;
    std::atomic<bool> stop{false};
    std::atomic<int> exit_status{0};
    std::once_flag initialize;
    handle_t stop_reader = HANDLE_INVALID;
    handle_t stop_writer = HANDLE_INVALID;
    pyxis_descriptor_binding input{};
    pyxis_descriptor_binding output{};
};
// Lifetime extends through main's stop_and_join, before Lua and C++ teardown.
runtime state;

uint64_t monotonic_ns()
{
    timespec now{};
    if (clock_gettime(CLOCK_MONOTONIC, &now) || now.tv_sec < 0 ||
        static_cast<uint64_t>(now.tv_sec) > UINT64_MAX / 1000000000) {
        throw std::runtime_error("LuaLS monotonic clock unavailable");
    }
    return static_cast<uint64_t>(now.tv_sec) * 1000000000 + now.tv_nsec;
}

void initialize()
{
    (void)monotonic_ns();
    if (pyxis_descriptor_borrow(STDIN_FILENO, &state.input) ||
        pyxis_descriptor_borrow(STDOUT_FILENO, &state.output) ||
        state.input.info.kind != HANDLE_KIND_NATIVE ||
        state.output.info.kind != HANDLE_KIND_NATIVE ||
        state.input.info.protocol != PROTOCOL_PIPE ||
        state.output.info.protocol != PROTOCOL_PIPE ||
        !(state.input.access & PYXIS_DESCRIPTOR_READ) ||
        !(state.output.access & PYXIS_DESCRIPTOR_WRITE) || state.input.buffered_read) {
        throw std::runtime_error("LuaLS stdio requires native pipe input/output without buffered input");
    }
    handle_t service = HANDLE_INVALID;
    if (environment_capability(startup_environment_handle(), "pipe", &service) != CALL_OK) {
        throw std::runtime_error("LuaLS pipe creation authority unavailable");
    }
    pipe_create_reply ends{};
    auto result = pipe_create(service, &ends);
    handle_close(service);
    if (result != CALL_OK) {
        throw std::runtime_error("LuaLS shutdown wake pipe creation failed");
    }
    state.stop_reader = ends.reader;
    state.stop_writer = ends.writer;
}

void instruction_hook(lua_State* L, lua_Debug*)
{
    if (stopping()) {
        lua_sethook(L, nullptr, 0, 0);
        lua_pushlightuserdata(L, &stop_sentinel);
        lua_error(L);
    }
}

int fail(lua_State* L, const std::string& error)
{
    // The caller releases all C++ owners/locks before crossing Lua's error boundary.
    if (error == stop_error) {
        lua_sethook(L, nullptr, 0, 0);
        lua_pushlightuserdata(L, &stop_sentinel);
    } else {
        lua_pushlstring(L, error.data(), error.size());
    }
    return -1;
}

template<lua_CFunction Function>
int boundary(lua_State* L)
{
    int result;
    try {
        result = Function(L);
    } catch (const std::exception& error) {
        lua_pushstring(L, error.what());
        request_stop(1);
        result = -1;
    }
    // All program-owned C++ locks and values are gone before Lua longjmp.
    return result < 0 ? lua_error(L) : result;
}

struct preflight {
    size_t bytes = 4;
    size_t values = 0;
    std::unordered_set<const void*> tables;
    std::string error;

    bool add(size_t count)
    {
        if (count > message_bytes - bytes) {
            error = "LuaLS serialized message exceeds 16 MiB admission budget";
            return false;
        }
        bytes += count;
        return true;
    }
    bool value(lua_State* L, int index, unsigned depth)
    {
        if (++values > value_limit || depth > 128) {
            error = "LuaLS serialized message exceeds value/depth budget";
            return false;
        }
        index = lua_absindex(L, index);
        switch (lua_type(L, index)) {
        case LUA_TNIL:
        case LUA_TBOOLEAN: return add(1);
        case LUA_TNUMBER: return add(9);
        case LUA_TSTRING: return add(5) && add(lua_rawlen(L, index));
        case LUA_TTABLE: {
            auto pointer = lua_topointer(L, index);
            if (!tables.insert(pointer).second) {
                return add(10);
            }
            if (tables.size() > table_limit || lua_rawlen(L, index) > value_limit) {
                error = "LuaLS serialized table exceeds admission budget";
                return false;
            }
            // __pairs can execute arbitrary producer code or expand beyond the raw
            // graph measured here. Such messages are outside this private profile.
            if (luaL_getmetafield(L, index, "__pairs") != LUA_TNIL) {
                lua_pop(L, 1);
                error = "LuaLS channels do not admit __pairs serialization";
                return false;
            }
            // Array holes cost a byte each. Hash traversal conservatively charges
            // array keys/values too; shared/cyclic tables are counted once.
            if (!add(16) || !add(lua_rawlen(L, index))) {
                return false;
            }
            lua_pushnil(L);
            while (lua_next(L, index)) {
                bool ok = value(L, -2, depth + 1) && value(L, -1, depth + 1);
                lua_pop(L, 1);
                if (!ok) {
                    lua_pop(L, 1);
                    return false;
                }
            }
            return true;
        }
        default:
            error = "LuaLS channels admit only nil, boolean, number, string and raw table values";
            return false;
        }
    }
};

int pack_protected(lua_State* L)
{
    int size = 0;
    void* bytes;
    try {
        bytes = seri_pack(L, 0, &size);
    } catch (const std::bad_alloc&) {
        return luaL_error(L, "LuaLS message serialization memory unavailable");
    }
    lua_pushlightuserdata(L, bytes);
    lua_pushinteger(L, size);
    return 2;
}

bool pack(lua_State* L, int from, void*& data, size_t& size, std::string& error)
{
    int top = lua_gettop(L);
    lua_pushcfunction(L, pack_protected);
    for (int i = from; i <= top; ++i) {
        lua_pushvalue(L, i);
    }
    if (lua_pcall(L, top - from + 1, 2, 0) != LUA_OK) {
        const char* text = lua_tostring(L, -1);
        error = text ? text : "LuaLS message serialization failed";
        lua_pop(L, 1);
        return false;
    }
    data = lua_touserdata(L, -2);
    size = static_cast<size_t>(lua_tointeger(L, -1));
    lua_pop(L, 2);
    return true;
}

void release(channel& queue, size_t charge)
{
    queue.bytes -= charge;
    --queue.items;
    state.bytes -= charge;
    --state.items;
}

bool push(lua_State* L, const std::shared_ptr<channel>& queue, std::string& error)
{
    preflight check;
    for (int i = 2, n = lua_gettop(L); i <= n; ++i) {
        if (!check.value(L, i, 0)) {
            error = check.error;
            return false;
        }
    }
    // lua-seri builds linked scratch blocks and then flattens them. Reserve
    // three times the conservative encoded size, covering both copies and
    // block headers, before its first allocation/copy.
    size_t charge = check.bytes * 3;
    {
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait(lock, [&] {
            return stopping() || (queue->items < channel_items &&
                charge <= channel_bytes - queue->bytes && state.items < total_items &&
                charge <= total_bytes - state.bytes);
        });
        if (stopping()) {
            error = stop_error;
            return false;
        }
        ++queue->items;
        queue->bytes += charge;
        ++state.items;
        state.bytes += charge;
    }
    struct admission {
        std::shared_ptr<channel> queue;
        size_t charge;
        bool committed = false;
        ~admission()
        {
            if (!committed) {
                {
                    std::lock_guard<std::mutex> lock(state.mutex);
                    release(*queue, charge);
                }
                state.changed.notify_all();
            }
        }
    } reserved{queue, charge};
    // Reserve before copying, then serialize outside the shared queue lock.
    void* bytes = nullptr;
    size_t size = 0;
    bool packed = pack(L, 2, bytes, size, error);
    std::unique_ptr<void, decltype(&free)> payload(bytes, free);
    if (!packed) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (size > check.bytes || stopping()) {
            error = stopping() ? stop_error : "LuaLS serialization exceeded its reserved budget";
            return false;
        }
        queue->queue.push_back({bytes, size});
        queue->bytes -= charge - size;
        state.bytes -= charge - size;
        reserved.committed = true;
        payload.release();
    }
    state.changed.notify_all();
    return true;
}

using channel_ref = std::shared_ptr<channel>;
using worker_ref = std::shared_ptr<worker>;

int channel_gc(lua_State* L)
{
    static_cast<channel_ref*>(luaL_checkudata(L, 1, channel_meta))->~shared_ptr();
    return 0;
}
int channel_push(lua_State* L)
{
    auto* queue = static_cast<channel_ref*>(luaL_checkudata(L, 1, channel_meta));
    std::string error;
    try {
        push(L, *queue, error);
    } catch (const std::exception& e) {
        error = e.what();
    }
    if (!error.empty()) {
        if (error != stop_error) {
            request_stop(1);
        }
        return fail(L, error);
    }
    return 0;
}
int unpack_protected(lua_State* L)
{
    void* bytes = lua_touserdata(L, 1);
    lua_settop(L, 0);
    return seri_unpackptr(L, bytes); // Frees bytes even on a protected unpack error.
}
int channel_pop(lua_State* L)
{
    auto* queue = static_cast<channel_ref*>(luaL_checkudata(L, 1, channel_meta));
    message item;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (!(*queue)->queue.empty()) {
            item = (*queue)->queue.front();
            (*queue)->queue.pop_front();
        }
    }
    if (!item.bytes) {
        lua_pushboolean(L, false);
        return 1;
    }
    lua_settop(L, 1);
    lua_pushboolean(L, true);
    lua_pushcfunction(L, unpack_protected);
    lua_pushlightuserdata(L, item.bytes);
    int status = lua_pcall(L, 1, LUA_MULTRET, 0);
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        release(**queue, item.charge);
    }
    state.changed.notify_all();
    if (status != LUA_OK) {
        request_stop(1);
        return -1;
    }
    return lua_gettop(L) - 1;
}
int channel_fd(lua_State* L)
{
    luaL_checkudata(L, 1, channel_meta);
    lua_settop(L, 1);
    return 1; // Typed queue readiness object, never a fabricated descriptor.
}
int push_channel(lua_State* L, const channel_ref& value)
{
    void* slot = lua_newuserdatauv(L, sizeof(channel_ref), 0);
    new (slot) channel_ref(value);
    luaL_setmetatable(L, channel_meta);
    return 1;
}
int channel_create(lua_State* L)
{
    size_t length = 0;
    const char* name = luaL_checklstring(L, 1, &length);
    if (!length || length > 128) {
        return luaL_error(L, "LuaLS channel name must contain 1..128 bytes");
    }
    channel_ref value;
    std::string error;
    try {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (stopping()) {
            error = stop_error;
        } else if (state.channels.count(name) || state.channels.size() == channel_limit) {
            error = "LuaLS duplicate channel name or channel capacity exhausted";
        } else {
            value = std::make_shared<channel>();
            state.channels.emplace(name, value);
        }
    } catch (const std::exception& e) {
        error = e.what();
    }
    if (!error.empty()) {
        return fail(L, error);
    }
    return push_channel(L, value);
}
int channel_query(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    channel_ref value;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        auto it = state.channels.find(name);
        if (it != state.channels.end()) {
            value = it->second;
        }
    }
    if (!value) {
        lua_pushnil(L);
        lua_pushliteral(L, "LuaLS channel not found");
        return 2;
    }
    return push_channel(L, value);
}

int selector_gc(lua_State* L)
{
    static_cast<selector*>(luaL_checkudata(L, 1, selector_meta))->~selector();
    return 0;
}
int selector_close(lua_State* L)
{
    static_cast<selector*>(luaL_checkudata(L, 1, selector_meta))->channels.clear();
    lua_pushboolean(L, true);
    return 1;
}
int selector_add(lua_State* L)
{
    auto* self = static_cast<selector*>(luaL_checkudata(L, 1, selector_meta));
    auto* queue = static_cast<channel_ref*>(luaL_checkudata(L, 2, channel_meta));
    if (luaL_checkinteger(L, 3) != read_event || !lua_isnoneornil(L, 4)) {
        return luaL_error(L, "LuaLS selector supports typed channel read readiness only");
    }
    for (const auto& current : self->channels) {
        if (current == *queue) {
            lua_pushboolean(L, true);
            return 1;
        }
    }
    if (self->channels.size() == channel_limit) {
        return luaL_error(L, "LuaLS selector channel capacity exhausted");
    }
    try {
        self->channels.push_back(*queue);
    } catch (const std::exception& e) {
        return luaL_error(L, "%s", e.what());
    }
    lua_pushboolean(L, true);
    return 1;
}
int next_event(lua_State* L)
{
    auto index = lua_tointeger(L, lua_upvalueindex(2)) + 1;
    lua_pushinteger(L, index);
    lua_replace(L, lua_upvalueindex(2));
    lua_rawgeti(L, lua_upvalueindex(1), index);
    if (lua_isnil(L, -1)) {
        return 0;
    }
    lua_pushinteger(L, read_event);
    return 2;
}
int selector_wait(lua_State* L)
{
    auto* self = static_cast<selector*>(luaL_checkudata(L, 1, selector_meta));
    lua_Integer timeout = luaL_optinteger(L, 2, -1);
    if (timeout < -1 || timeout > 30000) {
        return luaL_error(L, "LuaLS selector timeout must be -1 or 0..30000 ms");
    }
    std::vector<channel_ref> ready;
    std::string error;
    try {
        std::unique_lock<std::mutex> lock(state.mutex);
        auto predicate = [&] {
            if (stopping()) {
                return true;
            }
            for (const auto& queue : self->channels) {
                if (!queue->queue.empty()) {
                    return true;
                }
            }
            return false;
        };
        if (timeout < 0) {
            if (self->channels.empty()) {
                error = "LuaLS cannot wait forever on an empty selector";
            } else {
                state.changed.wait(lock, predicate);
            }
        } else if (timeout) {
            (void)monotonic_ns();
            state.changed.wait_for(lock, std::chrono::milliseconds(timeout), predicate);
        }
        if (stopping()) {
            error = stop_error;
        }
        if (error.empty()) {
            for (const auto& queue : self->channels) {
                if (!queue->queue.empty()) {
                    ready.push_back(queue);
                }
            }
        }
    } catch (const std::exception& e) {
        error = e.what();
    }
    if (!error.empty()) {
        return fail(L, error);
    }
    lua_createtable(L, static_cast<int>(ready.size()), 0);
    for (size_t i = 0; i < ready.size(); ++i) {
        push_channel(L, ready[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_pushinteger(L, 0);
    lua_pushcclosure(L, next_event, 2);
    return 1;
}
int selector_create(lua_State* L)
{
    if (!lua_isnoneornil(L, 1)) {
        auto count = luaL_checkinteger(L, 1);
        if (count < 1 || count > static_cast<lua_Integer>(channel_limit)) {
            return luaL_error(L, "LuaLS selector event capacity must be 1..32");
        }
    }
    void* slot = lua_newuserdatauv(L, sizeof(selector), 0);
    new (slot) selector;
    luaL_setmetatable(L, selector_meta);
    return 1;
}

void worker_main(std::string source, void* parameters)
{
    lua_State* L = nullptr;
    std::string error;
    try {
        L = new_state();
        if (!L) {
            throw std::runtime_error("LuaLS worker Lua state allocation failed");
        }
        enable_stop_hook(L);
        int status = luaL_loadbuffer(L, source.data(), source.size(), "=(native worker)");
        if (status == LUA_OK) {
            if (parameters) {
                lua_pushcfunction(L, unpack_protected);
                lua_pushlightuserdata(L, parameters);
                parameters = nullptr;
                status = lua_pcall(L, 1, LUA_MULTRET, 0);
            }
            if (status == LUA_OK) {
                status = lua_pcall(L, lua_gettop(L) - 1, 0, 0);
            }
        }
        if (status != LUA_OK && !is_stop_error(L, -1)) {
            const char* text = lua_tostring(L, -1);
            error = text ? text : "LuaLS worker failed with a non-string error";
        }
    } catch (const std::exception& e) {
        error = e.what();
    }
    free(parameters);
    if (!error.empty()) {
        request_stop(1);
    }
    if (L && !close_state(L, stopping() && requested_exit_status() == 0)) {
        request_stop(1);
    }
    if (!error.empty()) {
        // A failing worker cannot continue supplying its assigned service.
        try {
            std::string diagnostic = "lua-language-server: worker failed: " + error.substr(0, 8192) + "\n";
            size_t written = 0;
            while (written < diagnostic.size()) {
                ssize_t count = pyxis_descriptor_try_write(STDERR_FILENO,
                    diagnostic.data() + written, diagnostic.size() - written);
                if (count <= 0) {
                    break; // Failure status remains visible even if stderr is unavailable.
                }
                written += count;
            }
            {
                std::lock_guard<std::mutex> lock(state.mutex);
                if (state.errors.size() < worker_limit) {
                    state.errors.push_back(error.substr(0, 8192));
                }
            }
        } catch (const std::exception&) {
            // Reporting allocation failure cannot turn a failed worker into
            // successful service or escape the native thread entry.
        }
        request_stop(1);
    }
}
int worker_gc(lua_State* L)
{
    static_cast<worker_ref*>(luaL_checkudata(L, 1, worker_meta))->~shared_ptr();
    return 0;
}
int thread_create(lua_State* L)
{
    size_t length = 0;
    const char* source = luaL_checklstring(L, 1, &length);
    if (length > 64 * 1024 || lua_gettop(L) != 1) {
        return luaL_error(L, "LuaLS private workers accept a source of at most 64 KiB and no arguments");
    }
    worker_ref created;
    std::string error;
    try {
        std::string script(source, length);
        std::lock_guard<std::mutex> lock(state.mutex);
        if (stopping() || state.workers.size() == worker_limit) {
            error = stopping() ? stop_error : "LuaLS worker capacity exhausted";
        } else {
            created = std::make_shared<worker>();
            state.workers.push_back(created);
            try {
                created->thread = std::thread(worker_main, std::move(script), nullptr);
            } catch (const std::exception&) {
                state.workers.pop_back();
                throw;
            }
        }
    } catch (const std::exception& e) {
        error = e.what();
    }
    if (!error.empty()) {
        if (error != stop_error) {
            request_stop(1);
        }
        return fail(L, error);
    }
    void* slot = lua_newuserdatauv(L, sizeof(worker_ref), 0);
    new (slot) worker_ref(created);
    luaL_setmetatable(L, worker_meta);
    return 1;
}
int thread_errors(lua_State* L)
{
    std::string text;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.errors.empty()) {
            return 0;
        }
        text = std::move(state.errors.front());
        state.errors.pop_front();
    }
    lua_pushlstring(L, text.data(), text.size());
    return 1;
}
int thread_sleep(lua_State* L)
{
    lua_Integer millis = luaL_checkinteger(L, 1);
    if (millis < 0 || millis > 30000) {
        return luaL_error(L, "LuaLS sleep duration must be 0..30000 ms");
    }
    std::string error;
    try {
        (void)monotonic_ns();
        std::unique_lock<std::mutex> lock(state.mutex);
        state.changed.wait_for(lock, std::chrono::milliseconds(millis), [] { return stopping(); });
        if (stopping()) {
            error = stop_error;
        }
    } catch (const std::exception& e) {
        error = e.what();
    }
    if (!error.empty()) {
        return fail(L, error);
    }
    return 0;
}
int thread_stop_error(lua_State* L)
{
    lua_pushboolean(L, is_stop_error(L, 1));
    return 1;
}
int thread_stopping(lua_State* L)
{
    lua_pushboolean(L, stopping());
    return 1;
}
int os_exit(lua_State* L)
{
    int status = lua_isboolean(L, 1) ? (lua_toboolean(L, 1) ? 0 : 1)
                                    : static_cast<int>(luaL_optinteger(L, 1, 0));
    request_stop(status);
    lua_sethook(L, nullptr, 0, 0);
    lua_pushlightuserdata(L, &stop_sentinel);
    return -1;
}

bool wait_stream(handle_t handle, uint64_t events, std::string& error)
{
    while (!stopping()) {
        uint64_t now;
        try {
            now = monotonic_ns();
        } catch (const std::exception& e) {
            error = e.what();
            return false;
        }
        if (now > UINT64_MAX - WAIT_MAX_WAIT_NS) {
            error = "LuaLS readiness deadline overflow";
            return false;
        }
        wait_interest interests[2]{{handle, events, 0}, {state.stop_reader, WAIT_READABLE, 0}};
        uint64_t observed[2]{};
        auto status = wait_many(interests, 2, now + WAIT_MAX_WAIT_NS, observed);
        if (status == CALL_TIMED_OUT) {
            continue; // Native blocking deadline renewal, not a sleeping poll loop.
        }
        if (status != CALL_OK) {
            error = "LuaLS native stream readiness failed";
            return false;
        }
        if (stopping() || observed[1]) {
            error = stop_error;
            return false;
        }
        if (observed[0]) {
            return true;
        }
    }
    error = stop_error;
    return false;
}
int stdio_read(lua_State* L)
{
    auto length = luaL_checkinteger(L, 1);
    if (length < 1 || length > static_cast<lua_Integer>(frame_bytes)) {
        return luaL_error(L, "LuaLS read size must be 1..4 MiB");
    }
    luaL_Buffer buffer;
    char* target = luaL_buffinitsize(L, &buffer, length);
    size_t used = 0;
    std::string error;
    while (used < static_cast<size_t>(length) && !stopping()) {
        ssize_t count = pyxis_descriptor_try_read(STDIN_FILENO, target + used, length - used);
        if (count > 0) {
            used += count;
        } else if (!count) {
            error = used ? "LuaLS truncated input frame" : "LuaLS input closed";
            break;
        } else if (errno != EAGAIN || !wait_stream(state.input.handle, WAIT_READABLE, error)) {
            if (error.empty()) {
                error = "LuaLS stdin read failed";
            }
            break;
        }
    }
    if (stopping() && error.empty()) {
        error = stop_error;
    }
    if (!error.empty()) {
        if (error == stop_error) {
            return fail(L, error);
        }
        lua_pushnil(L);
        lua_pushlstring(L, error.data(), error.size());
        return 2;
    }
    luaL_pushresultsize(&buffer, used);
    return 1;
}
int stdio_abort(lua_State* L)
{
    size_t length = 0;
    const char* text = luaL_checklstring(L, 1, &length);
    request_stop(1); // Wake the main consumer before any diagnostic attempt.
    const char prefix[] = "lua-language-server: input failed: ";
    (void)pyxis_descriptor_try_write(STDERR_FILENO, prefix, sizeof(prefix) - 1);
    (void)pyxis_descriptor_try_write(STDERR_FILENO, text, length < 8192 ? length : 8192);
    (void)pyxis_descriptor_try_write(STDERR_FILENO, "\n", 1);
    lua_settop(L, 1);
    return -1;
}
int stdio_write(lua_State* L)
{
    size_t length;
    const char* text = luaL_checklstring(L, 1, &length);
    if (length > frame_bytes + 8192) {
        return luaL_error(L, "LuaLS output frame exceeds 4 MiB budget");
    }
    size_t used = 0;
    std::string error;
    while (used < length && !stopping()) {
        ssize_t count = pyxis_descriptor_try_write(STDOUT_FILENO, text + used, length - used);
        if (count > 0) {
            used += count;
        } else if (!count) {
            error = "LuaLS stdout made no progress";
            break;
        } else if (errno != EAGAIN || !wait_stream(state.output.handle, WAIT_WRITABLE, error)) {
            if (error.empty()) {
                error = "LuaLS stdout write failed";
            }
            break;
        }
    }
    if (stopping() && error.empty()) {
        error = stop_error;
    }
    if (!error.empty()) {
        if (error != stop_error) {
            request_stop(1);
        }
        return fail(L, error);
    }
    lua_pushboolean(L, true);
    return 1;
}

void metatable(lua_State* L, const char* name, const luaL_Reg* methods, lua_CFunction gc,
    lua_CFunction close = nullptr)
{
    luaL_newmetatable(L, name);
    if (methods) {
        lua_newtable(L);
        luaL_setfuncs(L, methods, 0);
        lua_setfield(L, -2, "__index");
    }
    lua_pushcfunction(L, gc);
    lua_setfield(L, -2, "__gc");
    if (close) {
        lua_pushcfunction(L, close);
        lua_setfield(L, -2, "__close");
    }
    lua_pop(L, 1);
}
int open_channel(lua_State* L)
{
    luaL_Reg methods[]{{"push", boundary<channel_push>}, {"pop", boundary<channel_pop>}, {"fd", boundary<channel_fd>}, {nullptr, nullptr}};
    metatable(L, channel_meta, methods, channel_gc);
    luaL_Reg functions[]{{"create", boundary<channel_create>}, {"query", boundary<channel_query>}, {nullptr, nullptr}};
    luaL_newlib(L, functions);
    return 1;
}
int open_selector(lua_State* L)
{
    luaL_Reg methods[]{{"event_add", boundary<selector_add>}, {"wait", boundary<selector_wait>},
        {"close", boundary<selector_close>}, {nullptr, nullptr}};
    metatable(L, selector_meta, methods, selector_gc, selector_close);
    luaL_Reg functions[]{{"create", boundary<selector_create>}, {nullptr, nullptr}};
    luaL_newlib(L, functions);
    lua_pushinteger(L, read_event);
    lua_setfield(L, -2, "SELECT_READ");
    lua_pushinteger(L, read_event);
    lua_setfield(L, -2, "EPOLLIN");
    return 1;
}
int open_thread(lua_State* L)
{
    metatable(L, worker_meta, nullptr, worker_gc);
    luaL_Reg functions[]{{"create", boundary<thread_create>}, {"sleep", boundary<thread_sleep>},
        {"errlog", boundary<thread_errors>}, {"stopping", boundary<thread_stopping>},
        {"is_stop_error", boundary<thread_stop_error>}, {nullptr, nullptr}};
    luaL_newlib(L, functions);
    return 1;
}
int open_stdio(lua_State* L)
{
    luaL_Reg functions[]{{"read_exact", boundary<stdio_read>}, {"write", boundary<stdio_write>},
        {"abort", boundary<stdio_abort>}, {nullptr, nullptr}};
    luaL_newlib(L, functions);
    return 1;
}
} // namespace

bool stopping() { return state.stop.load(std::memory_order_acquire); }
bool is_stop_error(lua_State* L, int index)
{
    return lua_islightuserdata(L, index) && lua_touserdata(L, index) == &stop_sentinel;
}
int requested_exit_status() { return state.exit_status.load(std::memory_order_acquire); }

void request_stop(int status)
{
    bool first;
    {
        // Pair the stop predicate with the condition mutex so stop cannot fall
        // between a waiter's predicate check and its atomic unlock-and-park.
        std::lock_guard<std::mutex> lock(state.mutex);
        if (status) {
            int expected = 0;
            state.exit_status.compare_exchange_strong(expected, status, std::memory_order_acq_rel);
        }
        first = !state.stop.exchange(true, std::memory_order_acq_rel);
    }
    if (first) {
        if (state.stop_writer != HANDLE_INVALID) {
            const char byte = 1;
            size_t written = 0;
            if (pipe_try_write(state.stop_writer, &byte, 1, &written) != CALL_OK || written != 1) {
                // Closing the writer is also a level-triggered readable EOF wake.
                handle_close(state.stop_writer);
                state.stop_writer = HANDLE_INVALID;
            }
        }
    }
    state.changed.notify_all();
}

void stop_and_join()
{
    request_stop(0);
    // Only the initial thread calls this, after the service has unwound. No
    // channel lock is held while joining; producers and pipe reader can retire.
    {
        // Wait for any create that passed its stop check to publish. Later
        // creates see stop under this same mutex, so the registry is now fixed.
        std::lock_guard<std::mutex> lock(state.mutex);
    }
    for (const auto& current : state.workers) {
        if (current->thread.joinable()) {
            current->thread.join();
            current->joined = true;
        }
    }
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        for (auto& entry : state.channels) {
            for (auto& item : entry.second->queue) {
                free(item.bytes);
            }
            entry.second->queue.clear();
            entry.second->bytes = 0;
            entry.second->items = 0;
        }
        state.channels.clear();
        state.workers.clear();
        state.bytes = state.items = 0;
    }
    if (state.stop_writer != HANDLE_INVALID) {
        handle_close(state.stop_writer);
        state.stop_writer = HANDLE_INVALID;
    }
    if (state.stop_reader != HANDLE_INVALID) {
        handle_close(state.stop_reader);
        state.stop_reader = HANDLE_INVALID;
    }
}

void enable_stop_hook(lua_State* L)
{
    lua_sethook(L, instruction_hook, LUA_MASKCOUNT, 5000);
}

void register_concurrency(lua_State* L)
{
    std::call_once(state.initialize, initialize);
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
    struct { const char* name; lua_CFunction open; } modules[]{
        {"bee.thread", open_thread}, {"bee.channel", open_channel},
        {"bee.select", open_selector}, {"bee.epoll", open_selector}, {"bee.stdio", open_stdio},
    };
    for (const auto& module : modules) {
        lua_pushcfunction(L, module.open);
        lua_setfield(L, -2, module.name);
    }
    lua_pop(L, 1);
    lua_getglobal(L, "os");
    lua_pushcfunction(L, boundary<os_exit>);
    lua_setfield(L, -2, "exit");
    lua_pop(L, 1);
}
} // namespace pyxis_luals
