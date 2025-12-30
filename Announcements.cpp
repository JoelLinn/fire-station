#include "Announcements.hpp"

std::filesystem::path FireStation::getAnnouncementPath(const Config &config, StaticAnnouncement staticAnnouncement) {
    const auto &path = config.getAnnouncementStaticDir();
    switch (staticAnnouncement) {
    case StaticAnnouncement::GONG_LONG:
        return path / "GongLong.wav";
    case StaticAnnouncement::GONG_SHORT:
        return path / "GongShort.wav";
    default:
        throw std::runtime_error("Unknown announcement type");
    }
}

std::filesystem::path FireStation::getAnnouncementPath(const Config &config, const TtsHash &ttsAnnouncement) {
    const auto fileName = sha256ToHex(ttsAnnouncement.data()) + ".wav";
    return config.getAnnouncementTmpDir() / fileName;
}
