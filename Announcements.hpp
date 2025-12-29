#pragma once

#include <filesystem>

#include "Alarms.hpp"

namespace FireStation {

enum class StaticAnnouncement {
    GONG_SHORT,
    GONG_LONG,
};

std::filesystem::path getAnnouncementPath(StaticAnnouncement staticAnnouncement);
std::filesystem::path getAnnouncementPath(TtsHash ttsAnnouncement);
} // namespace FireStation
