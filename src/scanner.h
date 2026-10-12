#pragma once

#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <string>
#include <expected>

namespace fs = std::filesystem;

struct FileInfo {
    fs::path relative_path;
    uintmax_t size;
    fs::file_time_type last_modified;
};

struct Snapshot {
    std::unordered_map<std::string, FileInfo> files;
};

std::expected<Snapshot, std::string> scan_directory(const fs::path& root);