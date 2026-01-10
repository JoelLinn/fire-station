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

std::optional<std::tuple<std::string, IPC::GatesType, std::chrono::system_clock::duration>> PushListener::constructMessage(const cJSON *jsonItem) const {
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
            return std::nullopt;
        }
        const auto vehicleMapping = vehicleMap.find(static_cast<uint64_t>(v));
        if (vehicleMapping == vehicleMap.end()) {
            std::cerr << "API failed to find mapped vehicle" << std::endl;
            return std::nullopt;
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
    IPC::GatesType gates;
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
        return std::nullopt;
    }
    // Need to convert to steady clock
    const auto age = std::chrono::system_clock::now().time_since_epoch() - std::chrono::seconds(static_cast<uint64_t>(date));

    return std::make_tuple(std::move(message), gates, age);
}

void PushListener::parseResponse(std::string_view jsonResponse) {
    std::unique_ptr<cJSON, RAII::DeleterFunc<cJSON_Delete>> json(cJSON_ParseWithLength(jsonResponse.data(), jsonResponse.size()));

    if (!json) {
        std::cerr << "cJSON_ParseWithLength failed" << jsonResponse.data() << std::endl;
        return;
    }

    if (cJSON_IsFalse(cJSON_GetObjectItem(json.get(), "success"))) {
        std::cerr << "API query unsuccessful" << std::endl;
        return;
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
        const auto idNum = static_cast<AlarmId>(id);
        const auto messageRes = constructMessage(jsonItem);
        if (!messageRes) {
            continue;
        }
        const auto [message, gates, age] = *messageRes;
        TtsHash ttsHash;
        calculateSha256(message.data(), message.size(), ttsHash.data());

        bool sendUpdate;
        // Search if id exists in list already
        if (auto it = std::ranges::find_if(Alarms, [idNum](const auto &alarm) { return std::get<0>(alarm) == idNum; }); it == Alarms.end()) {
            // New Alarm
            Alarms.push_back({idNum, ttsHash});
            sendUpdate = true;
        } else {
            // Existing alarm but maybe changed text (and gates)

            const auto &oldTtsHash = std::get<1>(*it);
            sendUpdate = !std::equal(ttsHash.begin(), ttsHash.end(), oldTtsHash.begin());
            if (sendUpdate) {
                *it = {idNum, ttsHash};
            }
        }
        if (sendUpdate) {
            std::cout << message << std::endl;
            FifoSet.NewAlarm.Put({idNum, std::make_shared<const std::string>(std::move(message)), ttsHash, gates, std::chrono::steady_clock::now() - age});
        }
    }
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
        //FifoSet.NewAlarm.Put({std::make_shared<const std::string>("Kleinbrand. Schwarze Tonne. Maumke. Es rückt aus: LF10"), 0b01, std::chrono::steady_clock::now()});
        parseResponse(R"({"success":true,"data":{"items":{"30809056":{"id":30809056,"author_id":253278,"cluster_id":30870,"alarmcode_id":0,"message_channel_id":8962137,"foreign_id":"1260000493","title":"Feuer 1 - Kaminbrand","text":"Meldender: Meldender: H\u00f6rig","report":"","address":"Waldstra\u00dfe 3, 57368 Lennestadt Oberelspe","lat":51.15741188,"lng":8.08980479,"priority":true,"date":1767649572,"new":false,"editable":false,"answerable":false,"notification_type":3,"vehicle":[90710,90716],"group":[162591,163011],"cluster":[],"user_cluster_relation":[],"hidden":false,"deleted":false,"message_channel":true,"custom_answers":true,"attachment_count":0,"closed":false,"close_state":-1,"duration":"","ts_response":1767653172,"response_time":3600,"ucr_addressed":[88760,88827,88848,88907,88912,89193,89194,89204,89286,89287,89288,89290,89291,89293,89294,89295,89297,89298,89300,89301,89307,143894,308124,308135,563041,722129,773016],"ucr_answered":{"98328":{"89286":{"ts":1767649762,"note":""},"89193":{"ts":1767649580,"note":""}},"98329":{"89194":{"ts":1767649691,"note":""},"88912":{"ts":1767649635,"note":""},"89204":{"ts":1767649629,"note":""},"89290":{"ts":1767649603,"note":""},"308135":{"ts":1767649598,"note":""},"308124":{"ts":1767649588,"note":""},"89294":{"ts":1767649584,"note":""}}},"ucr_answeredcount":{"98328":2,"98329":7},"ucr_read":[88760,88848,88907,88912,89193,89194,89204,89286,89288,89290,89294,89295,89298,89307,143894,308124,308135,563041,722129],"ucr_self_addressed":false,"count_recipients":27,"count_read":19,"private_mode":false,"custom":[],"ts_publish":0,"ts_create":1767649572,"ts_update":1767678143,"ts_close":0,"notification_filter_vehicle":false,"notification_filter_status":true,"notification_filter_shift_plan":0,"notification_filter_access":false,"notification_filter_status_access":false,"ucr_self_status_id":0,"ucr_self_note":""}},"sorting":[30809056]},"ucr":711547})");
        std::this_thread::sleep_for(20s);
#else
        curlResultBuffer.resize(0);
        const auto curlResult = curl_easy_perform(curl.get());
        if (curlResult != CURLE_OK) {
            std::cerr << "curl_easy_perform failed with {}" << curl_easy_strerror(curlResult) << std::endl;
            std::cerr << curlErrorBuffer.data() << std::endl;
            continue;
        }

        parseResponse(curlResultBuffer);

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
