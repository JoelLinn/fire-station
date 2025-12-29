#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace FireStation {
constexpr size_t SHA256_LENGTH = 256 / 8;
void calculateSha256(const void *data, size_t size, uint8_t out[SHA256_LENGTH]);
std::string sha256ToHex(uint8_t digest[SHA256_LENGTH]);
}