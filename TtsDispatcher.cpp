#include "TtsDispatcher.hpp"

#include <iostream>

#include <sys/wait.h>
#include <unistd.h>

namespace FireStation {

TtsDispatcher::TtsDispatcher(IPC::FifoSet &fifoSet) : FifoSet(fifoSet) {}

void TtsDispatcher::threadFunc(const bool &keepRunning) {
    while (keepRunning) {
        auto orderVariant = FifoSet.TtsOrder.Get();

        std::visit([this]<typename T0>(T0 &&order) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, IPC::Bye>) {
                // continue and break on keepRunning == false;
            } else if constexpr (std::is_same_v<T, IPC::TtsOrderData>) {
                const auto pid = fork();
                if (pid < 0) {
                    std::cerr << "fork failed" << std::endl;
                } else if (pid == 0) {
                    // TODO
                    std::string cmd("/usr/local/bin/piper");
                    std::string arg1("-m");
                    std::string arg2("/home/feuerwehr/thorsten-voice/data/de_DE-thorsten-high.onnx");
                    std::string arg3("-f");
                    std::string arg4(getAnnouncementPath(order.Hash));
                    std::string arg5("--");
                    std::string arg6(*(order.Text.get()));
                    (void)cmd.c_str(); // Ensure terminating character
                    char *const args[] = {cmd.data(), arg1.data(), arg2.data(), arg3.data(), arg4.data(), arg5.data(), arg6.data(), nullptr};
                    execv(cmd.data(), args);
                    std::cerr << "execv failed witth errno " << errno << std::endl;
                    exit(1);
                } else {
                    int status;
                    if (waitpid(pid, &status, 0) <= 0) {
                        std::cerr << "waitpid failed" << std::endl;
                    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                        std::cerr << "External tts program exited with error" << std::endl;
                    } else {
                        FifoSet.TtsReady.Put(std::move(order.Hash));
                    }
                }
            }
        },
                   orderVariant);
    }
}

} // namespace FireStation