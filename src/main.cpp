#include <iostream>
#include <vector>
#include <unordered_map>
#include <filesystem>

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

enum class ActionType {
    Copy,
    Delete,
    Update
};

struct SyncAction {
    ActionType type;
    fs::path relative_path;
};

vector<SyncAction> compare_snapshots(const Snapshot& snapshotA, const Snapshot& snapshotB) {
    vector<SyncAction> comparison_result;
    for(const auto& file1 : snapshotA.files) {
        auto file2 = snapshotB.files.find(file1.first);
        if(file2 == snapshotB.files.end()) {
            comparison_result.push_back({
                ActionType::Copy,
                file1.second.relative_path
            });
        }
        else {
            bool different = 
                file1.second.size != file2->second.size || 
                file1.second.last_modified != file2->second.last_modified;
            
            if(different) {
                comparison_result.push_back({
                    ActionType::Update,
                    file1.second.relative_path
                });
            }
        }
    }

    for(const auto& file1 : snapshotB.files) {
        auto file2 = snapshotA.files.find(file1.first);
        if(file2 == snapshotA.files.end()) {
            comparison_result.push_back({
                ActionType::Delete,
                file1.second.relative_path
            });
        }
    }
    return comparison_result;
}

Snapshot scan_directory(const fs::path& root) {
    Snapshot snapshot;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if(entry.is_regular_file()) {
            fs:: path relative_path = 
                fs::relative(entry.path(), root);
            
            snapshot.files.emplace(
                relative_path.string(),
                FileInfo{
                    fs::relative(entry.path(), root),
                    entry.file_size(),
                    entry.last_write_time()
                }
            );
        }
    }

    return snapshot;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: filesync <directory> <directory>\n";
        return 1;
    }

    try {
        fs::path root1 = argv[1], root2 = argv[2];
    
        auto snapshotA = scan_directory(root1),
            snapshotB = scan_directory(root2);
    
        vector<SyncAction> comparison_result = compare_snapshots(snapshotA, snapshotB);
        for(const auto& action : comparison_result) {
            switch(action.type) {
                case ActionType::Copy:
                cout << "Copy";
                break;
                
                case ActionType::Delete:
                cout << "Delete";
                break;
                
                case ActionType::Update:
                cout << "Update";
                break;
            }
            cout << ' ' << action.relative_path.string();
            cout << '\n';
        }
    }
    catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}