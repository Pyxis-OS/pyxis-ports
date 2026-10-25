#include "ship/NativePaths.h"

#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <sys/stat.h>

namespace Ship::Native {
std::string Join(const std::string& base, const std::string& relative) {
    return base + (base.empty() || base.back() == '/' ? "" : "/") + relative;
}

std::string FileName(const std::string& path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string Extension(const std::string& path) {
    const auto name = FileName(path);
    const auto dot = name.find_last_of('.');
    return dot == std::string::npos ? "" : name.substr(dot);
}

bool IsFile(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) == 0) {
        return S_ISREG(info.st_mode);
    }
    if (errno == ENOENT) {
        return false;
    }
    throw std::system_error(errno, std::generic_category(), "cannot inspect " + path);
}

void MakeDirectory(const std::string& path) {
    if (mkdir(path.c_str(), 0700) == 0) {
        return;
    }
    const auto error = errno;
    struct stat info;
    if (error == EEXIST && stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) {
        return;
    }
    throw std::system_error(error, std::generic_category(), "cannot prepare " + path);
}
}
