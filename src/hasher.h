#pragma once

#include<filesystem>
#include <string>
#include <optional>

namespace fs = std::filesystem;

std::optional<std::string> hash_file(const fs::path& path);