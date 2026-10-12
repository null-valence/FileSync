#pragma once

#include <filesystem>

namespace fs = std::filesystem;

bool is_path_prefix(const fs::path& parent, const fs::path& child);
bool validate_roots(fs::path& rootA, fs::path& rootB);