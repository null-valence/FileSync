#include "scanner.h"
#include "hasher.h"

std::expected<Snapshot, std::string> scan_directory(const fs::path& root1, const fs::path& root2, std::string folderA_name, std::string folderB_name) {
    Snapshot snapshot;

    for (auto it = fs::recursive_directory_iterator(root1); it != fs::recursive_directory_iterator(); ++it) {
        if(it->is_symlink()) {
            return std::unexpected("Symlink detected: " + it->path().string());
        }
        if(it->is_directory() && it->path().filename() == ".filesync") {
            it.disable_recursion_pending();
            continue;
        }

        fs::path relative_path = fs::relative(it->path(), root1);

        if(!it->is_regular_file()) {
            if(it->is_directory() && fs::is_regular_file(root2 / relative_path)) {
                return std::unexpected(
                    std::string("Error: File type conflict detected.\nPath: ") +
                    relative_path.string() +
                    '\n' +
                    folderA_name +
                    ": Directory\n" +
                    folderB_name +
                    ": Regular file\nSynchronization aborted. No changes made."
                );
            }
            continue;
        }

        if(fs::is_directory(root2 / relative_path)) {
            return std::unexpected(
                std::string("Error: File type conflict detected.\nPath: ") +
                relative_path.string() +
                '\n' +
                folderA_name +
                ": Regular file\n" +
                folderB_name +
                ": Directory\nSynchronization aborted. No changes made."
            );
        }
        
        snapshot.files.emplace(
            relative_path.string(),
            FileInfo{
                relative_path,
                it->file_size(),
                it->last_write_time(),
            }
        );
    }

    return snapshot;
}