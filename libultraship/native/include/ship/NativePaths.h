#pragma once

#include <string>

namespace Ship::Native {
// Preserve native scheme roots; these operations do not normalize authority.
std::string Join(const std::string& base, const std::string& relative);
std::string FileName(const std::string& path);
std::string Extension(const std::string& path);
bool IsFile(const std::string& path);
void MakeDirectory(const std::string& path);
}
