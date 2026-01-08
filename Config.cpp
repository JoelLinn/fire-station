#include "Config.hpp"
#include "Format.hpp"
#include "RaiiHelpers.hpp"

#include <fstream>
#include <iostream>
#include <memory>

#include "cJSON.h"

namespace FireStation {

Config::Config(const std::filesystem::path &configPath) {
    std::ifstream in(configPath, std::ios::in);
    if (!in.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open config file {}", configPath.string()));
    }
    std::string s(std::istreambuf_iterator<char>(in), {});
    std::unique_ptr<cJSON, RAII::DeleterFunc<cJSON_Delete>> json(cJSON_ParseWithLength(s.c_str(), s.length()));

    if (!json) {
        throw std::runtime_error("Failed to parse config file");
    }

    diveraAccessKey = getJsonStringValue(*json, "diveraAccessKey");
    piperExecutable = getJsonStringValue(*json, "piperExecutable");
    piperModelPath = getJsonStringValue(*json, "piperModelPath");
    ethercatInterface = getJsonStringValue(*json, "ethercatInterface");

    {
        std::string tmpDir("/tmp/fire-station.XXXXXX");
        if (!mkdtemp(tmpDir.data())) {
            throw std::runtime_error("Failed to create temporary directory");
        }
        announcementTmpDir = tmpDir;
    }

    announcementStaticDir = getJsonStringValue(*json, "announcementStaticDir");
}

Config::~Config() {
    if (rmdir(announcementTmpDir.c_str()) != 0) {
        std::cerr << "Failed to remove temporary directory: " << errno << std::endl;
    }
}

std::string Config::getJsonStringValue(const cJSON &root, const std::string &key) {
    const auto *jsonString = cJSON_GetStringValue(cJSON_GetObjectItem(&root, key.c_str()));
    if (!jsonString) {
        throw std::runtime_error(fmt::format("Missing or wrong type for {} key", key));
    }
    return jsonString;
}
} // namespace FireStation
