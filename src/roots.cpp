#include "roots.h"

#include <iostream>

bool is_path_prefix(const fs::path& parent, const fs::path& child) {
    auto p = parent.begin();
    auto c = child.begin();

    for (; p != parent.end(); ++p, ++c) {
        if (c == child.end() || *p != *c) {
            return false;
        }
    }

    return true;
}

bool validate_roots(fs::path& rootA, fs::path& rootB) {
    std::error_code ec;

    if (!fs::exists(rootA, ec) || ec) {
        std::cerr << "Source directory does not exist: " << rootA << '\n';
        return false;
    }

    if (!fs::is_directory(rootA, ec) || ec) {
        std::cerr << "Source path is not a directory: " << rootA << '\n';
        return false;
    }

    if (!fs::exists(rootB, ec) || ec) {
        std::cerr << "Destination directory does not exist: " << rootB << '\n';
        return false;
    }

    if (!fs::is_directory(rootB, ec) || ec) {
        std::cerr << "Destination path is not a directory: " << rootB << '\n';
        return false;
    }

    rootA = fs::canonical(rootA);
    rootB = fs::canonical(rootB);

    if (rootA == rootB) {
        std::cerr << "Source and destination cannot be the same directory.\n";
        return false;
    }

    if (is_path_prefix(rootA, rootB) || is_path_prefix(rootB, rootA)) {
        std::cerr << "Source and destination cannot contain one another.\n";
        return false;
    }

    return true;
}

std::optional<fs::path> resolve_shared_path(const fs::path& shared_root, const fs::path& requested_path) {
    if (requested_path.is_absolute()) return std::nullopt;
    for(auto& dir : requested_path) {
        if(dir == "..") return std::nullopt;
    }
    
    std::error_code ec;
    fs::path root = fs::canonical(shared_root, ec);
    if(ec) return std::nullopt;

    bool directory = fs::is_directory(root, ec);
    if (ec || !directory) return std::nullopt;

    fs::path file_path = root / requested_path;
    file_path = fs::canonical(file_path, ec);
    if(ec) return std::nullopt;

    if (!is_path_prefix(root, file_path)) return std::nullopt;

    bool regular = fs::is_regular_file(file_path, ec);
    if (ec || !regular) return std::nullopt;
    
    return file_path;
}