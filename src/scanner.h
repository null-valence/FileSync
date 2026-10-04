#pragma once

#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <string>

namespace fs = std::filesystem;

struct FileInfo {
    fs::path relative_path;
    uintmax_t size;
    fs::file_time_type last_modified;
};

struct Snapshot {
    std::unordered_map<std::string, FileInfo> files;
};

Snapshot scan_directory(const fs::path& root);