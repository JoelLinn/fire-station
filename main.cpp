#include <iostream>
#include <thread>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>

#include "Announcer.hpp"
#include "Controller.hpp"
#include "GeneralizedQueue.hpp"
#include "Ipc.hpp"
#include "PushListener.hpp"
#include "TtsDispatcher.hpp"

constexpr size_t MESSAGE_BUFFER_SIZE = 256;

static bool keepRunning{true};
using namespace FireStation;

static void handlerSigint([[maybe_unused]] int payload) {
    keepRunning = false;
}

int main(int argc, char **argv) {
    struct sigaction actionInt = {};
    actionInt.sa_handler = handlerSigint;
    if (sigaction(SIGINT, &actionInt, nullptr) != 0) {
        return 1;
    }

    // TODO config
    if (argc != 2) {
        printf("Usage: fire-station IFNAME\n"
               "IFNAME is the NIC interface name, e.g. 'eth0'\n");
        return 1;
    }
    const auto *ifname = argv[1];

    // TODO gotify websocket stream thread
    // Will put new alarms in ring buffer to process thread

    IPC::FifoSet fifoSet;
    PushListener pushListener(fifoSet);
    Controller controller(fifoSet, ifname);
    TtsDispatcher ttsDispatcher(fifoSet);
    Announcer announcer(fifoSet);

    // Lock all memory pages required for physical I/O and processing
    if (mlockall(MCL_CURRENT) != 0) {
        std::cerr << "mlockall failed with " << errno << std::endl;
    }

    std::thread pushListenerThread([&]() {
        pthread_setname_np(pthread_self(), "PushListener");
        pushListener.threadFunc(keepRunning);
    });
    std::thread controllerThread([&]() {
        // I/O thread has priority, needs root or CAP_SYS_NICE
        struct sched_param schedParam = {
            .sched_priority = sched_get_priority_max(SCHED_FIFO),
        };
        if (const auto ret = pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam); ret != 0) {
            std::cerr << "pthread_setschedparam failed with " << ret << std::endl;
        }
        pthread_setname_np(pthread_self(), "Controller");
        controller.threadFunc(keepRunning);
    });
    std::thread ttsDispatcherThread([&]() {
        pthread_setname_np(pthread_self(), "TtsDispatcher");
        ttsDispatcher.threadFunc(keepRunning);
    });
    std::thread announcerThread([&] {
        pthread_setname_np(pthread_self(), "Announcer");
        announcer.threadFunc(keepRunning);
    });

    pushListenerThread.join();
    controllerThread.join();
    ttsDispatcherThread.join();
    announcerThread.join();

    return 0;
}
