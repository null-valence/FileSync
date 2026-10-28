#include "file_transfer.h"
#include <fstream>
#include <charconv>
#include <system_error>


class TempFileGuard {
private:
    fs::path path_;
    bool keep_ = false;

public:
    explicit TempFileGuard(fs::path path) : path_(std::move(path)) {}

    ~TempFileGuard() {
        if (!keep_) {
            std::error_code ec;
            fs::remove(path_, ec);
        }
    }

    void keep() {
        keep_ = true;
    }

    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;
};

bool transfer_file(fs::path source, Connection& client) { // Remove it later, if no use
    std::ifstream file(source, std::ios::binary);
    if(!file.is_open()) return false;
    
    std::error_code ec;
    uintmax_t file_size = fs::file_size(source, ec);
    if(ec) return false;
    if(file_size > MAX_FILE_SIZE) return false;
    if (!client.send_message(std::to_string(file_size))) return false;

    const size_t buffer_size = 4096;
    char buffer[buffer_size];
    
    uintmax_t total_read = 0;
    while(true) {
        file.read(buffer, buffer_size);
        std::streamsize bytes_read = file.gcount();

        if (file.bad()) return false;
        if (bytes_read == 0) break;
        
        total_read += bytes_read;
        std::string chunk(buffer, bytes_read);
        if (!client.send_message(chunk)) return false;
        if (file.fail() && !file.eof()) return false;
        if(file.eof()) break;
    }
    
    if(file.eof() && total_read == file_size) return true;
    return false;
}

bool receive_file(fs::path destination, Connection& server) {
    std::string file_size;
    if (!server.receive_message(file_size)) return false;
    uintmax_t size_left = 0;
    const auto [ptr, e] = std::from_chars(file_size.data(), file_size.data() + file_size.size(), size_left);
    if (e != std::errc{} || ptr != file_size.data() + file_size.size()) return false;
    if(size_left > MAX_FILE_SIZE) return false;


    fs::path temp_file = destination.parent_path() / (".filesync-" + destination.filename().string() + ".tmp");
    TempFileGuard guard(temp_file);
    std::ofstream file(temp_file, std::ios::binary);
    if(!file.is_open()) return false;
    
    std::string buffer;
    while(size_left > 0) {
        if (!server.receive_message(buffer)) return false;
        if (buffer.size() > size_left || buffer.size() == 0) return false;
        if (!file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()))) return false;
        if (!file) return false;
        size_left -= buffer.size();
    }

    file.close();
    if (!file) return false;

    std::error_code ec;
    fs::rename(temp_file, destination, ec);
    if(ec) return false;

    guard.keep();
    return true;
}