#include <iostream>
#include <vector>
#include <filesystem>

using namespace std;
namespace fs = filesystem;

struct FileInfo {
    fs::path path;
    uintmax_t size;
    fs::file_time_type last_modified;
};

vector<FileInfo> scan_directory(const fs::path& root) {
    vector<FileInfo> files;

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if(entry.is_regular_file()) {
            files.push_back({
                fs::relative(entry.path(), root),
                entry.file_size(),
                entry.last_write_time()
            });
        }
    }

    return files;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: filesync <directory>\n";
        return 1;
    }

    try {
        fs::path root = argv[1];
    
        auto files = scan_directory(root);
    
        for(const auto& file : files) {
            cout << file.path << " | "
                     << file.size << " bytes\n";
        }
    }
    catch (const fs::filesystem_error& e) {
        cerr << "Filesystem error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}