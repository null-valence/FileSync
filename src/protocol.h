#pragma once

#include <array>
#include <cstdint>

constexpr int MAX_MESSAGE_SIZE = 16*1024*1024; // 16 MiB

std::array<std::uint8_t, 4> encode_length(std::uint32_t length);
std::uint32_t decode_length(const std::array<std::uint8_t, 4>& length);