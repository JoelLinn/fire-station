#include "TtsDispatcher.hpp"

#include <iostream>

#include <sys/wait.h>
#include <unistd.h>

namespace FireStation {

TtsDispatcher::TtsDispatcher(const Config &config, IPC::FifoSet &fifoSet) : Conf(config), FifoSet(fifoSet) {}

void TtsDispatcher::threadFunc(const bool &keepRunning) {
    while (keepRunning) {
        const auto orderVariant = FifoSet.TtsOrder.Get();

        if (std::holds_alternative<IPC::Bye>(orderVariant)) {
            // break on keepRunning == false;
            continue;
        }

        const auto order = std::get<IPC::TtsOrderData>(orderVariant);
        const auto pid = fork();
        if (pid < 0) {
            std::cerr << "fork failed" << std::endl;
        } else if (pid == 0) {
            using namespace std::string_literals;
            std::array arguments = {
                Conf.getPiperExecutable(),
                "-m"s,
                Conf.getPiperModelPath(),
                "--sentence-silence"s,
                "1"s,
                "-f"s,
                getAnnouncementPath(Conf, order.Hash).string(),
                "--"s,
                *order.Text.get()};
            // exec requires non const arg list
            std::array<char *, arguments.size() + 1> argv{};
            std::ranges::transform(arguments, argv.begin(), [](auto &s) { return s.data(); });
            argv.back() = nullptr;
            execvp(argv[0], argv.data());
            std::cerr << "execvp failed with errno " << errno << std::endl;
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
}

} // namespace FireStation