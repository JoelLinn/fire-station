#pragma once

#include "Ipc.hpp"

namespace FireStation {

class PushListener {
  public:
    PushListener() = delete;
    PushListener(const Config &config, IPC::FifoSet &fifoSet);

    void threadFunc(const bool &keepRunning);

    const Config &Conf;
    IPC::FifoSet &FifoSet;
};

} // namespace FireStation
