#include "line_parser.h"

bool extract_line(std::string& buffer, std::string& message) {
    std::size_t newline = buffer.find('\n');
    if(newline == std::string::npos || newline + 1 > 1024) return false;

    message = buffer.substr(0, newline + 1);
    buffer.erase(0, newline + 1);

    return true;
}