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
    bool parseResponse(std::string_view jsonResponse);

  private:
    const Config &Conf;
    IPC::FifoSet &FifoSet;
    std::vector<uint64_t> Alarms;
};

} // namespace FireStation
