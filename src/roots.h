#pragma once

#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

bool is_path_prefix(const fs::path& parent, const fs::path& child);
bool validate_roots(fs::path& rootA, fs::path& rootB);
std::optional<fs::path> resolve_shared_path(const fs::path& shared_root, const fs::path& requested_path);