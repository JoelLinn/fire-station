#include "PushListener.hpp"

#include <filesystem>

namespace FireStation {

PushListener::PushListener(const Config &config, IPC::FifoSet &fifoSet) : Conf(config), FifoSet(fifoSet) {

void PushListener::threadFunc(const bool &keepRunning) {
    while (keepRunning) {
        // TODO curl ws listen, also honor keepRunning flag

        std::this_thread::sleep_for(std::chrono::seconds(10));
        // FifoSet.NewAlarm.Put({std::make_shared<const std::string>("Test"), std::chrono::steady_clock::now()});
        // std::this_thread::sleep_for(std::chrono::seconds(20));
    }
}

} // namespace FireStation