#include "binding.hpp"

extern "C" {
#include <directory.h>
#include <file.h>
#include <handle.h>
#include <path.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include <memory>
#include <new>
#include <vector>

namespace pyxis_luals {
namespace {
constexpr char path_type[] = "pyxis.luals.path";
constexpr char status_type[] = "pyxis.luals.status";
constexpr char iterator_type[] = "pyxis.luals.directory";
constexpr char state_parent[] = "home://.cache/lua-language-server";
constexpr lua_Integer overwrite_existing = 2;

size_t root_length(const std::string& value)
{
    size_t colon = value.find("://");
    if (colon == std::string::npos || colon == 0) {
        return 0;
    }
    for (size_t i = 0; i < colon; ++i) {
        unsigned char c = value[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '+' || c == '-')) {
            return 0;
        }
    }
    return colon + 3;
}

void validate_path(const std::string& value)
{
    if (value.find('\0') != std::string::npos || (!value.empty() && value[0] == '/')) {
        throw std::runtime_error("path requires a relative name or native scheme root");
    }
}

std::string argument(lua_State* state, int index)
{
    if (auto* value = static_cast<std::string*>(luaL_testudata(state, index, path_type))) {
        return *value;
    }
    if (lua_type(state, index) != LUA_TSTRING) {
        throw std::runtime_error("expected a path or string");
    }
    size_t length;
    const char* bytes = lua_tolstring(state, index, &length);
    std::string value(bytes, length);
    validate_path(value);
    return value;
}

std::string& path_value(lua_State* state)
{
    auto* value = static_cast<std::string*>(luaL_testudata(state, 1, path_type));
    if (!value) {
        throw std::runtime_error("expected a path");
    }
    return *value;
}

int push_path(lua_State* state, const std::string& value)
{
    auto* storage = lua_newuserdatauv(state, sizeof(std::string), 0);
    new (storage) std::string(value);
    luaL_setmetatable(state, path_type);
    return 1;
}

std::string join(const std::string& left, const std::string& right)
{
    if (root_length(right) || left.empty()) {
        return right;
    }
    if (right.empty() || left.back() == '/') {
        return left + right;
    }
    return left + '/' + right;
}

std::string parent(const std::string& value)
{
    size_t root = root_length(value);
    size_t end = value.size();
    while (end > root && end && value[end - 1] == '/') {
        --end;
    }
    if (end < value.size()) {
        return value.substr(0, end);
    }
    if (end == root) {
        return value.substr(0, root);
    }
    size_t slash = value.rfind('/', end - 1);
    return slash == std::string::npos ? std::string() : value.substr(0, slash < root ? root : slash);
}

std::string filename(const std::string& value)
{
    size_t root = root_length(value);
    if (value.size() == root || (!value.empty() && value.back() == '/')) {
        return {};
    }
    size_t slash = value.rfind('/');
    return value.substr(slash == std::string::npos ? 0 : slash + 1);
}

std::string lexical(const std::string& value)
{
    size_t root = root_length(value);
    std::vector<std::string> parts;
    for (size_t begin = root; begin < value.size();) {
        size_t end = value.find('/', begin);
        if (end == std::string::npos) {
            end = value.size();
        }
        std::string part = value.substr(begin, end - begin);
        if (part == "..") {
            if (!parts.empty() && parts.back() != "..") {
                parts.pop_back();
            } else if (root) {
                throw std::runtime_error("path escapes native root");
            } else {
                parts.push_back(part);
            }
        } else if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        begin = end + 1;
    }
    std::string result = value.substr(0, root);
    for (const auto& part : parts) {
        result = join(result, part);
    }
    return result.empty() && !value.empty() ? "." : result;
}

std::string current_directory()
{
    std::unique_ptr<char, decltype(&free)> value(getcwd(nullptr, 0), free);
    if (!value) {
        system_error("getcwd");
    }
    return value.get();
}

std::string absolute(const std::string& value)
{
    // This supplies a descriptive path, not a realpath or an existence proof.
    return root_length(value) ? value : join(current_directory(), value);
}

int path_construct(lua_State* state)
{
    return push_path(state, lua_gettop(state) ? argument(state, 1) : std::string());
}

int path_gc(lua_State* state)
{
    static_cast<std::string*>(lua_touserdata(state, 1))->~basic_string();
    return 0;
}

int path_string(lua_State* state)
{
    const auto& value = path_value(state);
    lua_pushlstring(state, value.data(), value.size());
    return 1;
}

int path_parent(lua_State* state) { return push_path(state, parent(path_value(state))); }
int path_filename(lua_State* state) { return push_path(state, filename(path_value(state))); }
int path_normal(lua_State* state) { return push_path(state, lexical(path_value(state))); }

int path_stem(lua_State* state)
{
    std::string value = filename(path_value(state));
    size_t dot = value.rfind('.');
    if (dot != 0 && dot != std::string::npos && value != "..") {
        value.resize(dot);
    }
    return push_path(state, value);
}

int path_extension(lua_State* state)
{
    std::string value = filename(path_value(state));
    size_t dot = value.rfind('.');
    value = dot == 0 || dot == std::string::npos || value == ".." ? "" : value.substr(dot);
    lua_pushlstring(state, value.data(), value.size());
    return 1;
}

int path_absolute(lua_State* state)
{
    lua_pushboolean(state, root_length(path_value(state)) != 0);
    return 1;
}

int path_relative(lua_State* state)
{
    lua_pushboolean(state, root_length(path_value(state)) == 0);
    return 1;
}

int path_divide(lua_State* state)
{
    std::string left = argument(state, 1);
    return push_path(state, join(left, argument(state, 2)));
}

int path_concat(lua_State* state)
{
    std::string value = argument(state, 1) + argument(state, 2);
    validate_path(value);
    return push_path(state, value);
}

int path_equal(lua_State* state)
{
    std::string left = lexical(argument(state, 1));
    lua_pushboolean(state, left == lexical(argument(state, 2)));
    return 1;
}

struct file_status {
    const char* type;
};

const char* mode_type(mode_t mode)
{
    if (S_ISDIR(mode)) return "directory";
    if (S_ISREG(mode)) return "regular";
    if (S_ISFIFO(mode)) return "fifo";
    if (S_ISCHR(mode)) return "character";
    return "unknown";
}

const char* status(const std::string& value)
{
    struct stat info;
    if (lstat(value.c_str(), &info) == 0) {
        return mode_type(info.st_mode);
    }
    if (errno == ENOENT) {
        return "not_found";
    }
    system_error("status");
    return nullptr;
}

int push_status(lua_State* state, const char* type)
{
    auto* value = static_cast<file_status*>(lua_newuserdatauv(state, sizeof(file_status), 0));
    value->type = type;
    luaL_setmetatable(state, status_type);
    return 1;
}

int status_name(lua_State* state)
{
    auto* value = static_cast<file_status*>(luaL_testudata(state, 1, status_type));
    if (!value) throw std::runtime_error("expected file status");
    lua_pushstring(state, value->type);
    return 1;
}

int fs_status(lua_State* state) { return push_status(state, status(argument(state, 1))); }
int fs_exists(lua_State* state)
{
    lua_pushboolean(state, std::strcmp(status(argument(state, 1)), "not_found") != 0);
    return 1;
}
int fs_is_directory(lua_State* state)
{
    lua_pushboolean(state, std::strcmp(status(argument(state, 1)), "directory") == 0);
    return 1;
}
int fs_is_file(lua_State* state)
{
    lua_pushboolean(state, std::strcmp(status(argument(state, 1)), "regular") == 0);
    return 1;
}

bool create_directory(const std::string& value)
{
    if (mkdir(value.c_str(), 0700) == 0) return true;
    if (errno == EEXIST && std::strcmp(status(value), "directory") == 0) return false;
    system_error("create_directory");
    return false;
}

bool create_directories(const std::string& value)
{
    size_t root = root_length(value);
    bool created = false;
    for (size_t end = root; end < value.size();) {
        end = value.find('/', end);
        if (end == std::string::npos) end = value.size();
        std::string prefix = value.substr(0, end);
        if (prefix.size() > root) created = create_directory(prefix);
        ++end;
    }
    if (value.size() == root && root && std::strcmp(status(value), "directory") != 0) {
        throw std::runtime_error("create_directories: root is not a directory");
    }
    return created;
}

int fs_create_directory(lua_State* state)
{
    lua_pushboolean(state, create_directory(argument(state, 1)));
    return 1;
}
int fs_create_directories(lua_State* state)
{
    lua_pushboolean(state, create_directories(argument(state, 1)));
    return 1;
}
int fs_remove(lua_State* state)
{
    std::string value = argument(state, 1);
    if (::remove(value.c_str()) == 0) {
        lua_pushboolean(state, true);
    } else if (errno == ENOENT) {
        lua_pushboolean(state, false);
    } else {
        system_error("remove");
    }
    return 1;
}
int fs_rename(lua_State* state)
{
    std::string from = argument(state, 1), to = argument(state, 2);
    if (::rename(from.c_str(), to.c_str()) != 0) system_error("rename");
    return 0;
}
int fs_current(lua_State* state)
{
    if (!lua_gettop(state)) return push_path(state, current_directory());
    std::string value = argument(state, 1);
    if (chdir(value.c_str()) != 0) system_error("chdir");
    return 0;
}
int fs_absolute(lua_State* state) { return push_path(state, absolute(argument(state, 1))); }

struct stream_closer {
    void operator()(FILE* stream) const { if (stream) fclose(stream); }
};

int fs_copy_file(lua_State* state)
{
    std::string from = argument(state, 1), to = argument(state, 2);
    lua_Integer options = lua_isnoneornil(state, 3) ? 0 : lua_tointeger(state, 3);
    if ((!lua_isnoneornil(state, 3) && !lua_isinteger(state, 3)) || (options != 0 && options != overwrite_existing)) {
        throw std::runtime_error("copy_file: unsupported options");
    }
    std::unique_ptr<FILE, stream_closer> input(fopen(from.c_str(), "rb"));
    if (!input) system_error("copy_file source");
    struct stat source_info, target_info;
    if (fstat(fileno(input.get()), &source_info) != 0) system_error("copy_file source info");
    if (!S_ISREG(source_info.st_mode)) throw std::runtime_error("copy_file requires a regular file");
    int descriptor = open(to.c_str(), O_WRONLY | O_CREAT | (options == overwrite_existing ? 0 : O_EXCL), 0666);
    if (descriptor < 0) system_error("copy_file destination");
    std::unique_ptr<FILE, stream_closer> output(fdopen(descriptor, "wb"));
    if (!output) {
        int error = errno;
        close(descriptor);
        errno = error;
        system_error("copy_file destination stream");
    }
    if (fstat(descriptor, &target_info) != 0) system_error("copy_file destination info");
    constexpr uint64_t identity = STAT_DEV_VALID | STAT_INO_VALID;
    if ((source_info.st_valid & identity) != identity || (target_info.st_valid & identity) != identity) {
        throw std::runtime_error("copy_file: cannot prove destination differs from source");
    }
    if (source_info.st_dev == target_info.st_dev && source_info.st_ino == target_info.st_ino) {
        throw std::runtime_error("copy_file: source and destination are the same file");
    }
    if (ftruncate(descriptor, 0) != 0) system_error("copy_file truncate");
    char buffer[BUFSIZ];
    for (;;) {
        size_t count = fread(buffer, 1, sizeof(buffer), input.get());
        if (count && fwrite(buffer, 1, count, output.get()) != count) system_error("copy_file write");
        if (ferror(input.get())) system_error("copy_file read");
        if (!count) break;
    }
    if (fclose(output.release()) != 0) system_error("copy_file close output");
    if (fclose(input.release()) != 0) system_error("copy_file close input");
    lua_pushboolean(state, true);
    return 1;
}

struct directory_iterator {
    DIR* directory;
    std::string path;
    ~directory_iterator() { if (directory) closedir(directory); }
};

int iterator_close(lua_State* state)
{
    auto* value = static_cast<directory_iterator*>(lua_touserdata(state, 1));
    if (value->directory) {
        DIR* directory = value->directory;
        value->directory = nullptr;
        if (closedir(directory) != 0) system_error("closedir");
    }
    return 0;
}
int iterator_gc(lua_State* state)
{
    static_cast<directory_iterator*>(lua_touserdata(state, 1))->~directory_iterator();
    return 0;
}
int iterator_next(lua_State* state)
{
    auto* value = static_cast<directory_iterator*>(lua_touserdata(state, lua_upvalueindex(1)));
    if (!value->directory) return 0;
    errno = 0;
    struct dirent* entry = readdir(value->directory);
    if (!entry) {
        int error = errno;
        DIR* directory = value->directory;
        value->directory = nullptr;
        int closed = closedir(directory);
        if (error) { errno = error; system_error("readdir"); }
        if (closed != 0) system_error("closedir");
        return 0;
    }
    std::string name = join(value->path, entry->d_name);
    push_path(state, name);
    const char* type;
    switch (entry->d_type) {
    case DT_DIR: type = "directory"; break;
    case DT_REG: type = "regular"; break;
    case DT_LNK: type = "symlink"; break;
    default: type = "unknown"; break;
    }
    push_status(state, type);
    return 2;
}
int fs_pairs(lua_State* state)
{
    if (!lua_isnoneornil(state, 2) && (!lua_isinteger(state, 2) || lua_tointeger(state, 2) != 0)) {
        throw std::runtime_error("pairs: directory options are unsupported");
    }
    std::string path = argument(state, 1);
    auto* storage = lua_newuserdatauv(state, sizeof(directory_iterator), 0);
    auto* iterator = new (storage) directory_iterator{nullptr, path};
    luaL_setmetatable(state, iterator_type);
    iterator->directory = opendir(path.c_str());
    if (!iterator->directory) system_error("opendir");
    lua_pushvalue(state, -1);
    lua_pushcclosure(state, binding<iterator_next>, 1);
    lua_pushnil(state);
    lua_pushnil(state);
    lua_rotate(state, -4, -1);
    return 4;
}

struct native_handle {
    handle_t value = HANDLE_INVALID;
    ~native_handle() { if (value != HANDLE_INVALID) handle_close(value); }
};
native_handle owned_directory, owned_parent;
std::string owned_path, owned_name;
constexpr uint64_t cleanup_rights = DIRECTORY_RIGHT_LOOKUP | DIRECTORY_RIGHT_ENUMERATE |
    DIRECTORY_RIGHT_READ_FILES | DIRECTORY_RIGHT_REMOVE;

void native_check(enum call_status result, const char* operation)
{
    if (result != CALL_OK) {
        throw std::runtime_error(std::string(operation) + ": native status " + std::to_string(result));
    }
}

void close_native(native_handle& handle, const char* operation)
{
    handle_t value = handle.value;
    handle.value = HANDLE_INVALID;
    if (handle_close(value) != 0) {
        throw std::runtime_error(std::string(operation) + ": native close failed");
    }
}

void open_native_directory(const std::string& path, native_handle& result)
{
    std::vector<handle_t> directories(path.size() + 1);
    std::vector<char> component(path.size() + 1);
    struct path_workspace workspace{};
    workspace.directories = directories.data();
    workspace.directory_capacity = directories.size();
    workspace.component = component.data();
    workspace.component_capacity = component.size();
    native_check(path_resolve_native(nullptr, path.c_str(), DIRECTORY_KIND_DIRECTORY,
        cleanup_rights, &workspace, &result.value), "retain state directory");
}

void confirm_child(handle_t parent_handle, const std::string& name, handle_t original, bool directory)
{
    native_handle current;
    native_check(directory_lookup(parent_handle, name.c_str(),
        directory ? DIRECTORY_KIND_DIRECTORY : DIRECTORY_KIND_FILE,
        directory ? cleanup_rights : FILE_RIGHT_READ, &current.value), "recheck state entry");
    struct file_info_reply before{}, after{};
    native_check(directory ? directory_info(original, &before) : file_info(original, &before), "state identity");
    native_check(directory ? directory_info(current.value, &after) : file_info(current.value, &after), "state identity");
    constexpr uint64_t identity = FILE_INFO_DOMAIN_VALID | FILE_INFO_OBJECT_VALID;
    if ((before.valid & identity) != identity || (after.valid & identity) != identity ||
        before.domain != after.domain || before.object != after.object) {
        throw std::runtime_error("state cleanup: unavailable or replaced identity");
    }
}

void clear_directory(handle_t directory)
{
    std::vector<std::string> names;
    std::vector<char> name(64);
    struct directory_cursor cursor{};
    for (;;) {
        struct directory_enumerate_reply reply{};
        native_check(directory_enumerate(directory, &cursor, name.data(), name.size(), &reply), "enumerate state");
        if (reply.outcome == DIRECTORY_END) break;
        if (reply.outcome == DIRECTORY_BUFFER_TOO_SMALL) { name.resize(reply.name_size); continue; }
        if (reply.outcome != DIRECTORY_ENTRY) throw std::runtime_error("state cleanup: directory changed");
        names.emplace_back(name.data());
        cursor = reply.cursor;
    }
    for (const auto& child : names) {
        native_handle target;
        struct directory_typed_lookup_reply reply{};
        native_check(directory_lookup_typed(directory, child.c_str(), FILE_RIGHT_READ,
            cleanup_rights, &reply), "open state entry");
        target.value = reply.handle;
        bool is_directory = reply.protocol == PROTOCOL_DIRECTORY;
        if (is_directory) clear_directory(target.value);
        confirm_child(directory, child, target.value, is_directory);
        // This checks the sampled name, not a namespace mutation lease. The
        // native remove remains name-based under the inherited home authority.
        native_check(directory_remove(directory, child.c_str(), is_directory ?
            DIRECTORY_KIND_DIRECTORY : DIRECTORY_KIND_FILE), "remove state entry");
    }
}
}

int open_filesystem(lua_State* state)
{
    static const luaL_Reg path_methods[] = {
        {"string", binding<path_string>}, {"filename", binding<path_filename>},
        {"parent_path", binding<path_parent>}, {"stem", binding<path_stem>},
        {"extension", binding<path_extension>}, {"is_absolute", binding<path_absolute>},
        {"is_relative", binding<path_relative>}, {"lexically_normal", binding<path_normal>},
        {nullptr, nullptr},
    };
    luaL_newmetatable(state, path_type);
    luaL_newlib(state, path_methods);
    lua_setfield(state, -2, "__index");
    static const luaL_Reg path_meta[] = {
        {"__gc", path_gc}, {"__tostring", binding<path_string>},
        {"__div", binding<path_divide>}, {"__concat", binding<path_concat>},
        {"__eq", binding<path_equal>}, {nullptr, nullptr},
    };
    luaL_setfuncs(state, path_meta, 0);
    lua_pop(state, 1);
    luaL_newmetatable(state, status_type);
    lua_newtable(state);
    lua_pushcfunction(state, binding<status_name>);
    lua_setfield(state, -2, "type");
    lua_setfield(state, -2, "__index");
    lua_pop(state, 1);
    luaL_newmetatable(state, iterator_type);
    lua_pushcfunction(state, binding<iterator_close>);
    lua_setfield(state, -2, "__close");
    lua_pushcfunction(state, iterator_gc);
    lua_setfield(state, -2, "__gc");
    lua_pop(state, 1);
    static const luaL_Reg methods[] = {
        {"path", binding<path_construct>}, {"status", binding<fs_status>},
        {"symlink_status", binding<fs_status>}, {"exists", binding<fs_exists>},
        {"is_directory", binding<fs_is_directory>}, {"is_regular_file", binding<fs_is_file>},
        {"create_directory", binding<fs_create_directory>}, {"create_directories", binding<fs_create_directories>},
        {"rename", binding<fs_rename>}, {"remove", binding<fs_remove>},
        {"current_path", binding<fs_current>}, {"absolute", binding<fs_absolute>},
        {"copy_file", binding<fs_copy_file>}, {"pairs", binding<fs_pairs>},
        {nullptr, nullptr},
    };
    luaL_newlib(state, methods);
    lua_newtable(state);
    lua_pushinteger(state, 0);
    lua_setfield(state, -2, "none");
    lua_pushinteger(state, overwrite_existing);
    lua_setfield(state, -2, "overwrite_existing");
    lua_setfield(state, -2, "copy_options");
    return 1;
}

std::string create_state_directory()
{
    if (!owned_path.empty()) throw std::runtime_error("state directory is already owned");
    create_directories(state_parent);
    open_native_directory(state_parent, owned_parent);
    std::string candidate = std::string(state_parent) + "/instance-XXXXXX";
    if (!mkdtemp(candidate.data())) system_error("mkdtemp state");
    // Save the name before retaining so a failed retain cannot lead to reuse.
    owned_path = candidate;
    owned_name = filename(candidate);
    open_native_directory(candidate, owned_directory);
    confirm_child(owned_parent.value, owned_name, owned_directory.value, true);
    return owned_path;
}

bool remove_state_directory(const std::string& path, std::string& error)
{
    try {
        if (path != owned_path || owned_path.empty() || owned_directory.value == HANDLE_INVALID) {
            throw std::runtime_error("state cleanup: directory is not this instance's retained state");
        }
        confirm_child(owned_parent.value, owned_name, owned_directory.value, true);
        clear_directory(owned_directory.value);
        confirm_child(owned_parent.value, owned_name, owned_directory.value, true);
        native_check(directory_remove(owned_parent.value, owned_name.c_str(), DIRECTORY_KIND_DIRECTORY), "remove state directory");
        close_native(owned_directory, "close state directory");
        close_native(owned_parent, "close state parent");
        owned_path.clear();
        owned_name.clear();
        error.clear();
        return true;
    } catch (const std::exception& failure) {
        error = failure.what();
        return false;
    }
}
}
