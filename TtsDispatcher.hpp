#pragma once

#include "Ipc.hpp"

namespace FireStation {

class TtsDispatcher {
  public:
    TtsDispatcher() = delete;
    explicit TtsDispatcher(IPC::FifoSet &fifoSet);

    void threadFunc(const bool &keepRunning);

  private:
    IPC::FifoSet &FifoSet;
};

} // namespace FireStation
