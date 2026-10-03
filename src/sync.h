#pragma once

#include <vector>
#include <filesystem>

#include "scanner.h"

using namespace std;
namespace fs = filesystem;

enum class ActionType {
    Copy,
    Delete,
    Update
};

struct SyncAction {
    ActionType type;
    fs::path relative_path;
};

void execute_sync(const vector<SyncAction>& actions, const fs::path& rootA, const fs::path& rootB);
vector<SyncAction> compare_snapshots(const Snapshot& snapshotA, const Snapshot& snapshotB);