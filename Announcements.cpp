#include "Announcements.hpp"

std::filesystem::path FireStation::getAnnouncementPath(StaticAnnouncement staticAnnouncement) {
    std::filesystem::path path = std::filesystem::current_path().parent_path(); // TODO
    switch (staticAnnouncement) {
    case StaticAnnouncement::GONG_LONG:
        return path / "GongLong.wav";
    case StaticAnnouncement::GONG_SHORT:
        return path / "GongShort.wav";
    default:
        throw std::runtime_error("Unknown announcement type");
    }
}

std::filesystem::path FireStation::getAnnouncementPath(TtsHash ttsAnnouncement) {
    const auto fileName = (sha256ToHex(ttsAnnouncement.data()) + ".wav");
    return std::filesystem::path("/tmp") / fileName;
}
