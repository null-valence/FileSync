#include "state.h"
#include "hasher.h"

#include <fstream>
#include <filesystem>

std::expected<SyncState, std::string> load_state(const fs::path &path) {
    std::ifstream in(path);

    if(!in) return std::unexpected("Failed to open state file");

    int version;
    in >> version;

    if(!in || version != 1) return std::unexpected("Invalid state file version");

    std::string relative_path;
    SyncState state;

    while(std::getline(in >> std::ws, relative_path)) {
        std::string size_string;
        std::string time_string;
        std::string hash;

        if (!std::getline(in, size_string) ||
            !std::getline(in, time_string) ||
            !std::getline(in, hash)
        ) return std::unexpected("Corrupted state file");

        uintmax_t size;
        long long time;

        try {
            size = std::stoull(size_string);
            time = std::stoll(time_string);
        }
        catch (...) {
            return std::unexpected("Invalid state file data");
        }

        state.files[relative_path] = {
            size,
            fs::file_time_type(fs::file_time_type::duration(time)),
            hash
        };
    }

    return state;
}

std::expected<void, std::string> save_state(const fs::path& path, const SyncState& state) {
    std::ofstream out(path);

    if(!out) return std::unexpected("Failed to open state file for writitng");
    out << "1\n";

    for(const auto& [path, file] : state.files) {
        out << path << '\n';
        out << file.size << '\n';
        out << file.last_modified.time_since_epoch().count() << '\n';
        out << file.hash << '\n';
    }

    if(!out) return std::unexpected("Failed while writing state file");

    return {};
}

std::expected<SyncState, std::string> build_state(const Snapshot& snapshot, const fs::path& root) {
    SyncState state;
    for(const auto& [path, file] : snapshot.files) {
        auto hash = hash_file(root / file.relative_path);
        if(!hash) return std::unexpected("Failed to hash file: " + (root / file.relative_path).string());
        
        state.files.emplace(path, FileState{
            file.size,
            file.last_modified,
            *hash
        });
    }
    return state;
}