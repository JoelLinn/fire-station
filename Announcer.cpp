#include "Announcer.hpp"
#include "Announcements.hpp"

#include <filesystem>
#include <iostream>
#include <optional>

#include <sys/wait.h>
#include <unistd.h>

namespace FireStation {

Announcer::Announcer(const Config &config, IPC::FifoSet &fifoSet) : Conf(config), FifoSet(fifoSet) {}

template <>
std::optional<std::filesystem::path> Announcer::getAnnouncementAction(const Config &, const IPC::Bye &) {
    return std::nullopt;
}

template <typename T>
std::optional<std::filesystem::path> Announcer::getAnnouncementAction(const Config &config, const T &announcement) {
    return getAnnouncementPath(config, announcement);
}

void Announcer::threadFunc(const bool &keepRunning) {
    while (keepRunning) {
        // Play Announcement
        auto announcementVariant = FifoSet.Announcement.Get();

        std::optional<std::filesystem::path> path;
        std::visit([&path, this](const auto &announcement) {
            path = getAnnouncementAction(Conf, announcement);
        },
                   announcementVariant);
        if (!path) {
            // break on keepRunning == false;
            continue;
        }

        if (!std::filesystem::exists(*path)) {
            std::cerr << "File " << *path << " does not exist" << std::endl;
            continue;
        }

        const auto pid = fork();
        if (pid < 0) {
            std::cerr << "fork failed" << std::endl;
        } else if (pid == 0) {
            std::string arg0("aplay");
            std::string arg1(*path);
            char *const argv[] = {arg0.data(), arg1.data(), nullptr};
            execvp(argv[0], argv);
            std::cerr << "execvp failed with errno " << errno << std::endl;
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
}

} // namespace FireStation