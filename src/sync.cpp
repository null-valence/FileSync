#include <iostream>
#include <unordered_set>
#include "sync.h"
#include "hasher.h"

void print_sync_plan(const std::vector<SyncAction>& actions) {
    for(const auto& action : actions) {
        switch(action.type) {
            case ActionType::Copy:
                std::cout << "Copy";
                break;
            
            case ActionType::Delete:
                std::cout << "Delete";
                break;
            
            case ActionType::Update:
                std::cout << "Update";
                break;
        }
        std::cout << ' ' << action.relative_path;
        std::cout << '\n';
    }
}

void print_sync_error(SyncError error) {
    switch(error) {
        case SyncError::SourceNotFound:
            std::cout << "Source file was not found\n";
            break;
        
        case SyncError::DestinationNotFound:
            std::cout << "Destination was not found\n";
            break;
        
        case SyncError::PermissionDenied:
            std::cout << "Permission denied";
            break;

        default:
            std::cout << "Filesystem error";
            break;
    }
}

SyncError classify_error(const std::error_code& ec, PathRole role) {
    if(ec == std::errc::permission_denied) return SyncError::PermissionDenied;
    else if(ec == std::errc::no_such_file_or_directory) {
        if(role == PathRole::Destination) return SyncError::DestinationNotFound;
        else return SyncError::SourceNotFound;
    }

    return SyncError::FilesystemError;
}

std::expected<void, SyncError> copy_file_to_destination(const fs::path& source, const fs::path& destination) {
    std::error_code ec;

    fs::create_directories(destination.parent_path(), ec);
    if(ec) return std::unexpected(classify_error(ec, PathRole::Destination));
    
    fs::copy_file(source, destination, ec);
    if(ec) return std::unexpected(classify_error(ec, PathRole::Source));

    auto source_time = fs::last_write_time(source, ec);
    if(ec) return std::unexpected(classify_error(ec, PathRole::Source));

    fs::last_write_time(destination, source_time, ec);
    if(ec) return std::unexpected(classify_error(ec, PathRole::Destination));

    return {};
}

std::expected<void, SyncError> delete_file(const fs::path& path) {
    std::error_code ec;
    fs::remove(path, ec);
    if(ec) return std::unexpected(classify_error(ec, PathRole::Destination));

    /*
    remove(path, ec) returns 0 on failure, 1 on success
    since failure means, file was already removed after scanning process, but before this removal,
    this is also a success, so return {} in both cases.
    */
    return {};
}

std::expected<void, SyncError> update_file(const fs::path& source, const fs::path& destination) {
    std::error_code ec;
    fs::path temp_file = destination.parent_path() / (".filesync-" + destination.filename().string() + ".tmp");
    fs::copy_file(
        source,
        temp_file,
        fs::copy_options::overwrite_existing,
        ec
    );
    if(ec) return std::unexpected(classify_error(ec, PathRole::Source));

    fs::rename(temp_file, destination, ec);
    if(ec) {
        fs::remove(temp_file);
        return std::unexpected(classify_error(ec, PathRole::Destination));
    }

    auto source_time = fs::last_write_time(source, ec);
    if(ec) return std::unexpected(classify_error(ec, PathRole::Source));

    fs::last_write_time(destination, source_time, ec);
    if(ec) return std::unexpected(classify_error(ec, PathRole::Destination));

    return {};
}

ChangeStatus file_changed(const FileInfo& current, const FileState& previous, const fs::path& root) {
    if(current.size == previous.size && current.last_modified == previous.last_modified)
        return ChangeStatus::Unchanged;

    auto current_hash = hash_file(root / current.relative_path);
    if(!current_hash) return ChangeStatus::Error;

    if(current_hash != previous.hash) return ChangeStatus::Changed;
    return ChangeStatus::Unchanged;
}

std::expected<void, SyncError> execute_sync(const std::vector<SyncAction>& actions, const fs::path& rootA, const fs::path& rootB) {
    fs::path source_root;
    fs::path destination_root;
    
    for(const auto& action : actions) {
        if(action.direction == SyncDirection::AtoB) {
            source_root = rootA;
            destination_root = rootB;
        }else {
            source_root = rootB;
            destination_root = rootA;
        }

        switch(action.type) {
            case ActionType::Copy: {
                fs::path source = source_root / action.relative_path;
                fs::path destination = destination_root / action.relative_path;
                std::cout << "Copy:\n\t" << source;
                std::cout << "\n\t->\n\t" << destination << '\n';

                auto result = copy_file_to_destination(source, destination);
                if(!result) return std::unexpected(result.error());
                std::cout << "Copied successfully";
                break;
            }
            case ActionType::Delete: {
                fs::path destination = destination_root / action.relative_path;
                std::cout << "Delete:\n\t" << destination << '\n';

                auto result = delete_file(destination);
                if(!result) return std::unexpected(result.error());
                std::cout << "Deleted successfully";
                break;
            }
            case ActionType::Update: {
                fs::path source = source_root / action.relative_path;
                fs::path destination = destination_root / action.relative_path;
                std::cout << "Update:\n\t" << source;
                std::cout << "\n\t->\n\t" << destination << '\n';

                auto result = update_file(source, destination);
                if(!result) return std::unexpected(result.error());
                std::cout << "Updated successfully";
                break;
            }
        }
        std::cout << '\n';
    }
    return {};
}

std::optional<bool> files_equal(const FileInfo& fileA, const FileInfo& fileB, const fs::path& rootA, const fs::path& rootB) {
    if(fileA.size != fileB.size)
        return false;

    auto hashA = hash_file(rootA / fileA.relative_path);
    auto hashB = hash_file(rootB / fileB.relative_path);

    if(!hashA || !hashB)
        return std::nullopt;

    return *hashA == *hashB;
}

SyncPlanResult plan_sync(const Snapshot& snapshotA, const Snapshot& snapshotB, const SyncState& state, const fs::path& rootA, const fs::path& rootB) {
    SyncPlan plan;
    std::unordered_set<std::string> paths;
    for(const auto& file : snapshotA.files) {
        paths.insert(file.first);
    }
    for(const auto& file : snapshotB.files) {
        paths.insert(file.first);
    }
    for(const auto& file : state.files) {
        paths.insert(file.first);
    }

    for(const auto& path : paths) {
        auto file_base_it = state.files.find(path);
        auto fileA_it = snapshotA.files.find(path);
        auto fileB_it = snapshotB.files.find(path);

        ChangeStatus change_status_A;
        ChangeStatus change_status_B;
        bool existsA = false, existsB = false;

        if(file_base_it == state.files.end()) {
            if(fileA_it != snapshotA.files.end()) {
                change_status_A = ChangeStatus::Changed;
                existsA = true;
            }
            else
                change_status_A = ChangeStatus::Unchanged;

            if(fileB_it != snapshotB.files.end()) {
                change_status_B = ChangeStatus::Changed;
                existsB = true;
            }
            else
                change_status_B = ChangeStatus::Unchanged;
        }
        else {
            if(fileA_it != snapshotA.files.end()) {
                change_status_A = file_changed(fileA_it->second, file_base_it->second, rootA);
                existsA = true;
            }
            else
                change_status_A = ChangeStatus::Changed;

            if(fileB_it != snapshotB.files.end()) {
                change_status_B = file_changed(fileB_it->second, file_base_it->second, rootB);
                existsB = true;
            }
            else
                change_status_B = ChangeStatus::Changed;
        }

        if(change_status_A == ChangeStatus::Changed && change_status_B == ChangeStatus::Unchanged) {
            ActionType type;
            fs::path relative_path;
            if(existsA) {
                relative_path = fileA_it->second.relative_path;
                if(existsB) type = ActionType::Update;
                else type = ActionType::Copy;
            }else {
                type = ActionType::Delete;
                relative_path = fileB_it->second.relative_path;
            }
            plan.actions.push_back({
                type,
                SyncDirection::AtoB,
                relative_path
            });
        }
        else if(change_status_A == ChangeStatus::Unchanged && change_status_B == ChangeStatus::Changed) {
            ActionType type;
            fs::path relative_path;
            if(existsB) {
                relative_path = fileB_it->second.relative_path;
                if(existsA) type = ActionType::Update;
                else type = ActionType::Copy;
            }else {
                type = ActionType::Delete;
                relative_path = fileA_it->second.relative_path;
            }
            plan.actions.push_back({
                type,
                SyncDirection::BtoA,
                relative_path
            });
        }
        else if(change_status_A == ChangeStatus::Changed && change_status_B == ChangeStatus::Changed) {
            if(file_base_it != state.files.end()) {
                if(existsA != existsB) {
                    if(existsA) plan.conflicts.push_back({
                        fileA_it->second.relative_path
                    });
                    else plan.conflicts.push_back({
                        fileB_it->second.relative_path
                    });
                }
                else if(existsA && existsB) {
                    auto equal = files_equal(fileA_it->second, fileB_it->second, rootA, rootB);
                    if(!equal) return std::unexpected(SyncError::FilesystemError);

                    if(!*equal) plan.conflicts.push_back({
                        fileB_it->second.relative_path
                    });
                }
            }
            else {
                plan.conflicts.push_back({
                    fileA_it->second.relative_path
                });
            }
        }
        else if(change_status_A == ChangeStatus::Error || change_status_B == ChangeStatus::Error) return std::unexpected(SyncError::FilesystemError);
    }

    return plan;
}

std::unordered_set<std::string> resolve_conflict(SyncPlan& plan) {
    std::unordered_set<std::string> skipped_conflicts;

    for(const auto& conflict : plan.conflicts) {
        std::cout << "Conflict: " << conflict.relative_path << "\n\n";
        std::cout << "Choose:\n";
        std::cout << "\t[A] Keep version from A\n";
        std::cout << "\t[B] Keep version from B\n";
        std::cout << "\t[S] Skip\n\n";
        std::cout << "Selection (A/B/S): ";
        char selection;
        std::cin >> selection;
        while(
            selection != 'a' && selection != 'A' &&
            selection != 'b' && selection != 'B' &&
            selection != 's' && selection != 'S') {
                std::cout << "Please select between A, B, or S\n";
                std::cout << "Selection (A/B/S): ";
                std::cin >> selection;
        }

        if(selection == 'a' || selection == 'A') plan.actions.push_back({
            ActionType::Update,
            SyncDirection::AtoB,
            conflict.relative_path
        });
        else if(selection == 'b' || selection == 'B') plan.actions.push_back({
            ActionType::Update,
            SyncDirection::BtoA,
            conflict.relative_path
        });
        else {
            skipped_conflicts.emplace(conflict.relative_path);
        }
    }
    return skipped_conflicts;
}
