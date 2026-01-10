#pragma once

#include "Ipc.hpp"

#include <string_view>
#include <vector>

namespace FireStation {

class PushListener {
  public:
    PushListener() = delete;
    PushListener(const Config &config, IPC::FifoSet &fifoSet);
    ~PushListener();

    void threadFunc(const bool &keepRunning);

  private:
    std::optional<std::tuple<std::string, IPC::GatesType, std::chrono::steady_clock::time_point>> constructMessage(const cJSON *jsonItem) const;
    void parseResponse(std::string_view jsonResponse);

  private:
    const Config &Conf;
    IPC::FifoSet &FifoSet;
    std::vector<std::tuple<AlarmId, TtsHash>> Alarms;
};

} // namespace FireStation
