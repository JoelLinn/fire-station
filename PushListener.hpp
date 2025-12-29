#pragma once

#include "Ipc.hpp"

namespace FireStation {

class PushListener {
  public:
    PushListener() = delete;
    explicit PushListener(IPC::FifoSet &fifoSet);

    void threadFunc(const bool &keepRunning);

  private:
    IPC::FifoSet &FifoSet;
};

} // namespace FireStation
