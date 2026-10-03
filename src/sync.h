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

void print_sync_plan(const vector<SyncAction>& actions);
void execute_sync(const vector<SyncAction>& actions, const fs::path& rootA, const fs::path& rootB);
vector<SyncAction> compare_snapshots(const Snapshot& snapshotA, const Snapshot& snapshotB, const fs::path& rootA, const fs::path& rootB);