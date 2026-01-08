#pragma once

#include <filesystem>
#include <optional>

#include "Ipc.hpp"

namespace FireStation {

class Announcer {
  public:
    Announcer() = delete;
    Announcer(const Config &config, IPC::FifoSet &fifoSet);

    void threadFunc(const bool &keepRunning);

  private:
    template <typename T>
    static std::optional<std::filesystem::path> getAnnouncementAction(const Config &config, const T &announcement);

  private:
    const Config &Conf;
    IPC::FifoSet &FifoSet;
};

} // namespace FireStation
