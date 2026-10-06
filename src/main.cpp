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
        
        const fs::path state_path = rootA / ".filesync" / "state";
        SyncState state;
        if(fs::exists(state_path)) {
            auto state_result = load_state(state_path);

            if(!state_result) {
                std::cerr << "Failed to load state: " << state_result.error() << '\n';
                return 1;
            }

            state = *state_result;
        }

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
        
        auto execute_result = execute_sync(plan.actions, rootA, rootB);

        if (!execute_result) {
            std::cerr << "Synchronization failed: ";
            print_sync_error(execute_result.error());
            return 1;
        }

        auto final_snapshot = scan_directory(rootA);
        auto state_result = build_state(final_snapshot, rootA);

        if (!state_result) {
            std::cerr << "Failed to build state: "
                    << state_result.error() << '\n';
            return 1;
        }

        const fs::path state_dir = rootA / ".filesync";
        fs::create_directories(state_dir);
        auto save_result = save_state(state_path, *state_result);

        if (!save_result) {
            std::cerr << "Failed to save state: "
                    << save_result.error() << '\n';
            return 1;
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}