#pragma once

#include <unordered_map>
#include <filesystem>
#include <cstdint>

using namespace std;
namespace fs = filesystem;

struct FileInfo {
    fs::path relative_path;
    uintmax_t size;
    fs::file_time_type last_modified;
};

struct Snapshot {
    unordered_map<string, FileInfo> files;
};

Snapshot scan_directory(const fs::path& root);