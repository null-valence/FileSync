#include "scanner.h"
#include "hasher.h"

Snapshot scan_directory(const fs::path& root) {
    Snapshot snapshot;

    for (auto it = fs::recursive_directory_iterator(root);
        it != fs::recursive_directory_iterator();
        ++it) {

        if(it->is_directory() && it->path().filename() == ".filesync") {
            it.disable_recursion_pending();
            continue;
        }

        if(!it->is_regular_file()) continue;

        fs::path relative_path = fs::relative(it->path(), root);
        
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