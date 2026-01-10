#include "PushListener.hpp"

#include "Format.hpp"
#include "RaiiHelpers.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>

#include <curl/curl.h>

#include "cJSON.h"

namespace FireStation {

using namespace std::chrono_literals;
static constexpr auto PUSH_LISTENER_POLL_INTERVAL = 1s;

PushListener::PushListener(const Config &config, IPC::FifoSet &fifoSet) : Conf(config), FifoSet(fifoSet) {
    CURLcode curlResult = curl_global_init(CURL_GLOBAL_ALL);
    if (curlResult != CURLE_OK) {
        throw std::runtime_error(fmt::format("curl_global_init failed with {}", curl_easy_strerror(curlResult)));
    }
}

PushListener::~PushListener() {
    curl_global_cleanup();
}

bool PushListener::parseResponse(std::string_view jsonResponse) {
    std::unique_ptr<cJSON, RAII::DeleterFunc<cJSON_Delete>> json(cJSON_ParseWithLength(jsonResponse.data(), jsonResponse.size()));

    if (!json) {
        std::cerr << "cJSON_ParseWithLength failed" << jsonResponse.data() << std::endl;
        return false;
    }

    if (cJSON_IsFalse(cJSON_GetObjectItem(json.get(), "success"))) {
        std::cerr << "API query unsuccessful" << std::endl;
        return false;
    }

    const auto *jsonItems = cJSON_GetObjectItem(cJSON_GetObjectItem(json.get(), "data"), "items");
    if (!jsonItems) {
        std::cerr << "API failed to get items" << std::endl;
    }

    const cJSON *jsonItem;
    cJSON_ArrayForEach(jsonItem, jsonItems) {
        const auto id = cJSON_GetNumberValue(cJSON_GetObjectItem(jsonItem, "id"));
        if (std::isnan(id)) {
            std::cerr << "API failed to get id" << std::endl;
            continue;
        }
        const auto idU64 = static_cast<uint64_t>(id);
        if (std::ranges::find(Alarms, idU64) == Alarms.end()) {
            // New alarm
            Alarms.push_back(idU64);
            {

                std::unique_ptr<char, RAII::DeleterFunc<cJSON_free>> s(cJSON_PrintUnformatted(jsonItem));
                if (s) {
                    std::cout << s.get() << std::endl;
                }
            }
            const auto *title = cJSON_GetStringValue(cJSON_GetObjectItem(jsonItem, "title"));
            const auto *text = cJSON_GetStringValue(cJSON_GetObjectItem(jsonItem, "text"));
            const auto *address = cJSON_GetStringValue(cJSON_GetObjectItem(jsonItem, "address"));
            const cJSON *jsonVehicle;
            const auto &vehicleMap = Conf.getVehicleMap();
            std::vector<std::remove_reference_t<decltype(vehicleMap)>::mapped_type> vehicles;
            cJSON_ArrayForEach(jsonVehicle, cJSON_GetObjectItem(jsonItem, "vehicle")) {
                const auto v = cJSON_GetNumberValue(jsonVehicle);
                if (std::isnan(v)) {
                    std::cerr << "API failed to get vehicle" << std::endl;
                    continue;
                }
                const auto vehicleMapping = vehicleMap.find(static_cast<uint64_t>(v));
                if (vehicleMapping == vehicleMap.end()) {
                    std::cerr << "API failed to find mapped vehicle" << std::endl;
                    continue;
                }
                vehicles.push_back(vehicleMapping->second);
            }

            std::string message;
            if (title) {
                message += title;
                message += ". ";
            }
            if (text) {
                message += text;
                message += ". ";
            }
            if (address) {
                message += address;
                message += ". ";
            }
            decltype(IPC::NewAlarm::Gates) gates;
            if (vehicles.size() == 1) {
                gates.set(std::get<0>(vehicles[0]));
                message += "Es rückt aus: " + std::get<1>(vehicles[0]) + ".";
            } else if (vehicles.size() > 1) {
                message += "Es rücken aus:";
                for (const auto &vehicle : vehicles) {
                    gates.set(std::get<0>(vehicle));
                    message += " " + std::get<1>(vehicle);
                }
                message += ".";
            }

            const auto date = cJSON_GetNumberValue(cJSON_GetObjectItem(jsonItem, "date"));
            if (std::isnan(date)) {
                std::cerr << "API failed to get date" << std::endl;
                continue;
            }
            // Need to convert to steady clock
            const auto age = std::chrono::system_clock::now().time_since_epoch() - std::chrono::seconds(static_cast<uint64_t>(date));

            TtsHash alarmHash;
            calculateSha256(message.data(), message.size(), alarmHash.data());

            FifoSet.NewAlarm.Put({std::make_shared<const std::string>(std::move(message)), alarmHash, gates, std::chrono::steady_clock::now() - age});
        }
    }

    return true;
}

template <typename T>
static void curlEasySetoptThrow(CURL *curl, CURLoption option, T parameter) {
    const auto curlResult = curl_easy_setopt(curl, option, parameter);
    if (curlResult != CURLE_OK) {
        const auto *opt = curl_easy_option_by_id(option);
        throw std::runtime_error(fmt::format("curl_easy_setopt for {} failed with {}", opt ? opt->name : "?", curl_easy_strerror(curlResult)));
    }
}

static size_t curlStdStringWriteFunction(void *ptr, size_t size, size_t nmemb, void *userdata) {
    if (!userdata) {
        return 0;
    }
    auto *str = reinterpret_cast<std::string *>(userdata);
    str->append(reinterpret_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

void PushListener::threadFunc(const bool &keepRunning) {
    std::array<char, CURL_ERROR_SIZE + 1> curlErrorBuffer{};
    std::string curlResultBuffer{};
    std::string curlResultBufferLast{}; // Used when Conf.getDiveraDebugApi()
    std::unique_ptr<CURL, RAII::DeleterFunc<curl_easy_cleanup>> curl(curl_easy_init());
    if (!curl) {
        throw std::runtime_error("curl_easy_init failed");
    }

    const auto url = "https://app.divera247.com/api/v2/alarms?accesskey=" + Conf.getDiveraAccessKey();
    curlEasySetoptThrow(curl.get(), CURLOPT_URL, url.c_str());
    curlEasySetoptThrow(curl.get(), CURLOPT_CA_CACHE_TIMEOUT, 604800L);
    curlEasySetoptThrow(curl.get(), CURLOPT_ERRORBUFFER, curlErrorBuffer.data());
    curlEasySetoptThrow(curl.get(), CURLOPT_WRITEDATA, &curlResultBuffer);
    curlEasySetoptThrow(curl.get(), CURLOPT_WRITEFUNCTION, curlStdStringWriteFunction);

    while (keepRunning) {
#if 0
        std::this_thread::sleep_for(5s);
        // FifoSet.NewAlarm.Put({std::make_shared<const std::string>("TEST: Dies ist eine Testmeldung."), std::chrono::steady_clock::now()});
        FifoSet.NewAlarm.Put({std::make_shared<const std::string>("Kleinbrand. Schwarze Tonne. Maumke. Es rückt aus: LF10"), 0b01, std::chrono::steady_clock::now()});
        std::this_thread::sleep_for(20s);
#else
        curlResultBuffer.resize(0);
        const auto curlResult = curl_easy_perform(curl.get());
        if (curlResult != CURLE_OK) {
            std::cerr << "curl_easy_perform failed with {}" << curl_easy_strerror(curlResult) << std::endl;
            std::cerr << curlErrorBuffer.data() << std::endl;
            continue;
        }

        if (!parseResponse(curlResultBuffer)) {
            std::cerr << "Failed to parse api response" << std::endl;
        }

        if (Conf.getDiveraDebugApi()) {
            if (curlResultBuffer != curlResultBufferLast) {
                std::cout << curlResultBuffer << std::endl;
            }
            std::swap(curlResultBufferLast, curlResultBuffer);
        }

        std::this_thread::sleep_for(PUSH_LISTENER_POLL_INTERVAL);
#endif
    }
}

} // namespace FireStation
