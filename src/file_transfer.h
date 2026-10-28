#include <filesystem>
#include "connection.h"

namespace fs = std::filesystem;

constexpr uintmax_t MAX_FILE_SIZE = 1ULL * 1024 * 1024 * 1024; // 1 GiB

bool transfer_file(fs::path source, Connection& client);
bool receive_file(fs::path destination, Connection& server);