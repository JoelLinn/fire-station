#pragma once

#include <array>

#include "Sha256.hpp"

namespace FireStation {
using AlarmId = uint64_t;
using TtsHash = std::array<uint8_t, SHA256_LENGTH>; // TODO class with == overload
} // namespace FireStation
