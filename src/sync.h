#pragma once

#include <vector>
#include <filesystem>
#include <expected>

#include "scanner.h"

namespace fs = std::filesystem;

enum class ActionType {
    Copy,
    Delete,
    Update
};

enum class PathRole {
    Source,
    Destination
};

enum class SyncError {
    SourceNotFound,
    DestinationNotFound,
    PermissionDenied,
    FilesystemError
};

struct SyncAction {
    ActionType type;
    fs::path relative_path;
};

void print_sync_plan(const std::vector<SyncAction>& actions);
void execute_sync(const std::vector<SyncAction>& actions, const fs::path& rootA, const fs::path& rootB);
std::vector<SyncAction> compare_snapshots(const Snapshot& snapshotA, const Snapshot& snapshotB, const fs::path& rootA, const fs::path& rootB);