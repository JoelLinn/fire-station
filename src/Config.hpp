#pragma once

#include <filesystem>
#include <map>
#include <string>

typedef struct cJSON cJSON;

namespace FireStation {
class Config {
  public:
    Config(const std::filesystem::path &configPath);
    ~Config();

    const std::string &getDiveraAccessKey() const {
        return diveraAccessKey;
    }

    bool getDiveraDebugApi() const {
        return diveraDebugApi;
    }

    const std::string &getPiperExecutable() const {
        return piperExecutable;
    }

    const std::string &getPiperModelPath() const {
        return piperModelPath;
    }

    const std::string &getEthercatInterface() const {
        return ethercatInterface;
    }

    const std::filesystem::path &getAnnouncementTmpDir() const {
        return announcementTmpDir;
    }

    const std::filesystem::path &getAnnouncementStaticDir() const {
        return announcementStaticDir;
    }

    const std::map<uint64_t, std::tuple<size_t, std::string>> &getVehicleMap() const {
        return vehicleMap;
    }

  private:
    static std::string getJsonStringValue(const cJSON &root, const std::string &key);

  private:
    std::string diveraAccessKey;
    bool diveraDebugApi;
    std::string piperExecutable;
    std::string piperModelPath;
    std::string ethercatInterface;
    std::filesystem::path announcementTmpDir;
    std::filesystem::path announcementStaticDir;
    std::map<uint64_t, std::tuple<size_t, std::string>> vehicleMap;
};
} // namespace FireStation
