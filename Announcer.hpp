#pragma once

#include "Ipc.hpp"

namespace FireStation {

class Announcer {
  public:
    Announcer() = delete;
    explicit Announcer(IPC::FifoSet &fifoSet);

    void threadFunc(const bool &keepRunning);

  private:
    IPC::FifoSet &FifoSet;
};

} // namespace FireStation
