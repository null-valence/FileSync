#include <iostream>

#include "scanner.h"
#include "sync.h"

namespace fs = std::filesystem;


int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: filesync <directory> <directory>\n";
        return 1;
    }

    try {
        fs::path rootA = argv[1], rootB = argv[2];
    
        auto snapshotA = scan_directory(rootA),
            snapshotB = scan_directory(rootB);
    
        SyncState state;
        auto result = plan_sync(snapshotA, snapshotB, state, rootA, rootB);
        if(!result) {
            std::cerr << "Failed to create synchronization plan\n";
            return 1;
        }

        const auto& plan = *result;

        print_sync_plan(plan.actions);
        if(!plan.conflicts.empty()) {
            std::cerr << "Conflicts detected:\n";

            for(const auto& conflict : plan.conflicts) {
                std::cerr << "  " << conflict.relative_path << '\n';
            }

            return 1;
        }
        execute_sync(plan.actions, rootA, rootB);
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}