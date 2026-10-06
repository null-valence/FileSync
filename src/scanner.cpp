#include "scanner.h"
#include "hasher.h"

Snapshot scan_directory(const fs::path& root) {
    Snapshot snapshot;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if(entry.is_regular_file()) {
            fs::path relative_path = fs::relative(entry.path(), root);
            
            snapshot.files.emplace(
                relative_path.string(),
                FileInfo{
                    relative_path,
                    entry.file_size(),
                    entry.last_write_time(),
                }
            );
        }
    }

    return snapshot;
}