#include "protocol.h"
#include <arpa/inet.h>
#include <cstring>

std::array<std::uint8_t, 4> encode_length(std::uint32_t length) {
    std::array<std::uint8_t, 4> payload_length;
    length = htonl(length);
    memcpy(payload_length.data(), &length, sizeof(length));
    return payload_length;
}

std::uint32_t decode_length(const std::array<std::uint8_t, 4>& length) {
    std::uint32_t payload_length;
    memcpy(&payload_length, length.data(), length.size());
    payload_length = ntohl(payload_length);
    return payload_length;
}