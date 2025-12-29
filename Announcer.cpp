#include "Announcer.hpp"
#include "Announcements.hpp"

#include <filesystem>
#include <iostream>
#include <optional>

#include <sys/wait.h>
#include <unistd.h>

namespace FireStation {

Announcer::Announcer(IPC::FifoSet &fifoSet) : FifoSet(fifoSet) {}

void Announcer::threadFunc(const bool &keepRunning) {
    while (keepRunning) {
        // Play Announcement
        auto announcementVariant = FifoSet.Announcement.Get();

        std::optional<std::filesystem::path> file = std::nullopt;
        // TODO config base paths
        std::visit([&file]<typename T0>(T0 &&announcement) {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, IPC::Bye>) {
                // continue and break on keepRunning == false;
            } else {
                auto path = getAnnouncementPath(announcement).string();

                const auto pid = fork();
                if (pid < 0) {
                    std::cerr << "fork failed" << std::endl;
                } else if (pid == 0) {
                    std::string cmd("aplay");
                    (void)cmd.c_str(); // Ensure terminating character
                    char *const args[] = {cmd.data(), path.data(), nullptr};
                    execvp(cmd.data(), args);
                    std::cerr << "execvp failed witth errno " << errno << std::endl;
                    exit(1);
                } else {
                    int status;
                    if (waitpid(pid, &status, 0) <= 0) {
                        std::cerr << "waitpid failed" << std::endl;
                    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                        std::cerr << "External tts program exited with error" << std::endl;
                    }
                }
            }
        },
                   announcementVariant);

        if (!file) {
            continue;
        }

        if (!std::filesystem::exists(*file)) {
            std::cerr << "File " << *file << " does not exist" << std::endl;
            continue;
        }

        std::cerr << "TODO: Play file " << *file << std::endl;
    }
}

} // namespace FireStation