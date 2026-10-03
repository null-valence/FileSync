#include <iostream>

#include "scanner.h"
#include "sync.h"

using namespace std;
namespace fs = filesystem;


int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: filesync <directory> <directory>\n";
        return 1;
    }

    try {
        fs::path rootA = argv[1], rootB = argv[2];
    
        auto snapshotA = scan_directory(rootA),
            snapshotB = scan_directory(rootB);
    
        auto actions = compare_snapshots(snapshotA, snapshotB);
        // print_sync_plan(actions);
        execute_sync(actions, rootA, rootB);
    }
    catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}