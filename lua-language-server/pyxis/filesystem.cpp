#include "binding.hpp"

extern "C" {
#include <directory.h>
#include <file.h>
#include <handle.h>
#include <path.h>
#include <stdlib.h>
}

#include <filesystem>
#include <vector>

namespace pyxis_luals {
namespace {
constexpr char state_parent[] = "home://.cache/lua-language-server";

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

std::string create_state_directory()
{
    if (!owned_path.empty()) throw std::runtime_error("state directory is already owned");
    std::filesystem::create_directories(state_parent);
    open_native_directory(state_parent, owned_parent);
    std::string candidate = std::string(state_parent) + "/instance-XXXXXX";
    if (!mkdtemp(candidate.data())) system_error("mkdtemp state");
    // Save the name before retaining so a failed retain cannot lead to reuse.
    owned_path = candidate;
    owned_name = std::filesystem::path(candidate).filename().string();
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
