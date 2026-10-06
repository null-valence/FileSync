#pragma once

#include <vector>
#include <filesystem>
#include <expected>

#include "scanner.h"
#include "state.h"

namespace fs = std::filesystem;

enum class SyncDirection {
    AtoB,
    BtoA
};

enum class ActionType {
    Copy,
    Delete,
    Update
};

struct SyncAction {
    ActionType type;
    SyncDirection direction;
    fs::path relative_path;
};

struct SyncConflict {
    fs::path relative_path;
};

struct SyncPlan {
    std::vector<SyncAction> actions;
    std::vector<SyncConflict> conflicts;
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

enum class ChangeStatus {
    Unchanged,
    Changed,
    Error
};

using SyncPlanResult = std::expected<SyncPlan, SyncError>;

SyncPlanResult plan_sync(
    const Snapshot& snapshotA,
    const Snapshot& snapshotB,
    const SyncState& state,
    const fs::path& rootA,
    const fs::path& rootB
);
void print_sync_plan(const std::vector<SyncAction>& actions);
void execute_sync(const std::vector<SyncAction>& actions, const fs::path& rootA, const fs::path& rootB);