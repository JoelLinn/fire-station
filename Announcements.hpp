#pragma once

#include <filesystem>

#include "Alarms.hpp"
#include "Config.hpp"

namespace FireStation {

enum class StaticAnnouncement {
    GONG_SHORT,
    GONG_LONG,
};

std::filesystem::path getAnnouncementPath(const Config &config, StaticAnnouncement staticAnnouncement);
std::filesystem::path getAnnouncementPath(const Config &config, const TtsHash &ttsAnnouncement);
} // namespace FireStation
