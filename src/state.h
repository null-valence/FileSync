#pragma once

#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <string>

namespace fs = std::filesystem;

struct FileState {
    uintmax_t size;
    fs::file_time_type last_modified;
    std::string hash;
};

struct SyncState {
    std::unordered_map<std::string, FileState> files;
};
