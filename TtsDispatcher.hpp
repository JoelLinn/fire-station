#pragma once

#include "Config.hpp"
#include "Ipc.hpp"

namespace FireStation {

class TtsDispatcher {
  public:
    TtsDispatcher() = delete;
    TtsDispatcher(const Config &config, IPC::FifoSet &fifoSet);

    void threadFunc(const bool &keepRunning);

  private:
    const Config &Conf;
    IPC::FifoSet &FifoSet;
};

} // namespace FireStation
