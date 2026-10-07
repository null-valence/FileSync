#pragma once

#include <unordered_map>
#include <filesystem>
#include <cstdint>
#include <string>
#include <expected>
#include <unordered_set>

#include "scanner.h"

namespace fs = std::filesystem;

struct FileState {
    uintmax_t size;
    fs::file_time_type last_modified;
    std::string hash;
};

struct SyncState {
    std::unordered_map<std::string, FileState> files;
};

std::expected<SyncState, std::string> load_state(const fs::path &path);
std::expected<void, std::string> save_state(const fs::path& path, const SyncState& state);
std::expected<SyncState, std::string> build_state(const std::unordered_set<std::string>& skipped_conflicts, const SyncState& previous_state, const Snapshot& snapshot, const fs::path& root);