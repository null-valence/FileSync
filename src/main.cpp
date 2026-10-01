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
    if (argc != 2) {
        cerr << "Usage: filesync <directory>\n";
        return 1;
    }

    try {
        fs::path root = argv[1];
    
        auto snapshot = scan_directory(root);
    
        for(const auto& file : snapshot.files) {
            cout << file.second.relative_path << " | "
                     << file.second.size << " bytes\n";
        }
    }
    catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}