#pragma once

#include "Ipc.hpp"

#include <string_view>

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
};

} // namespace FireStation
