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