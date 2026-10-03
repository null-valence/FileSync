#include "sync.h"
#include <iostream>

void print_sync_plan(const vector<SyncAction>& actions) {
    for(const auto& action : actions) {
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
        cout << ' ' << action.relative_path;
        cout << '\n';
    }
}

bool copy_file_to_destination(const fs::path& source, const fs::path& destination) {
    try {
        fs::create_directories(destination.parent_path());
        fs::copy_file(source, destination);
        fs::last_write_time(
            destination,
            fs::last_write_time(source)
        );
        return true;
    }
    catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << '\n';
        return false;
    }
}

bool delete_file(const fs::path& path) {
    return fs::remove(path);
}

bool update_file(const fs::path& source, const fs::path& destination) {
    try {
        fs::copy_file(
            source,
            destination,
            fs::copy_options::overwrite_existing
        );
        fs::last_write_time(
            destination,
            fs::last_write_time(source)
        );
        return true;
    }
    catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << '\n';
        return false;
    }
}

void execute_sync(const vector<SyncAction>& actions, const fs::path& rootA, const fs::path& rootB) {
    for(const auto& action : actions) {
        switch(action.type) {
            case ActionType::Copy: {
                fs::path source = rootA / action.relative_path;
                fs::path destination = rootB / action.relative_path;
                cout << "Copy:\n\t" << source;
                cout << "\n\t->\n\t" << destination << '\n';
                if(copy_file_to_destination(source, destination)) cout << "Copied successfully";
                else cout << "Failed copying";
                break;
            }
            case ActionType::Delete: {
                fs::path destination = rootB / action.relative_path;
                cout << "Delete:\n\t" << destination << '\n';
                if(delete_file(destination)) cout << "Deleted successfully";
                else cout << "File was not found";
                break;
            }
            case ActionType::Update: {
                fs::path source = rootA / action.relative_path;
                fs::path destination = rootB / action.relative_path;
                cout << "Update:\n\t" << source;
                cout << "\n\t->\n\t" << destination << '\n';
                if(update_file(source, destination)) cout << "Updated successfully";
                else cout << "Failed updating";
                break;
            }
        }
        cout << '\n';
    }
}

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