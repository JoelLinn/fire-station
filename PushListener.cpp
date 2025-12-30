#include "PushListener.hpp"

#include "Format.hpp"
#include "RaiiHelpers.hpp"

#include <array>
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
        // TODO
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
    CURLcode curlResult;
    std::array<char, CURL_ERROR_SIZE + 1> curlErrorBuffer{};
    std::string curlResultBuffer{};
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
        // TODO curl ws listen, also honor keepRunning flag

        std::this_thread::sleep_for(5s);
        // FifoSet.NewAlarm.Put({std::make_shared<const std::string>("TEST: Dies ist eine Testmeldung."), std::chrono::steady_clock::now()});
        FifoSet.NewAlarm.Put({std::make_shared<const std::string>("Kleinbrand. Schwarze Tonne. Maumke. Es rückt aus: LF10"), 0b01, std::chrono::steady_clock::now()});
        std::this_thread::sleep_for(20s);
#else
        curlResultBuffer.resize(0);
        curlResult = curl_easy_perform(curl.get());
        if (curlResult != CURLE_OK) {
            std::cerr << "curl_easy_perform failed with {}" << curl_easy_strerror(curlResult) << std::endl;
            std::cerr << curlErrorBuffer.data() << std::endl;
            continue;
        }

        parseResponse(curlResultBuffer);

        std::this_thread::sleep_for(PUSH_LISTENER_POLL_INTERVAL);
#endif
    }
}

} // namespace FireStation
