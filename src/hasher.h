#pragma once

#include<filesystem>
#include <string>
#include <optional>


using namespace std;
namespace fs = filesystem;

optional<string> hash_file(const fs::path& path);