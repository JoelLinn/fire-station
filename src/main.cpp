#include <iostream>
#include <latch>
#include <thread>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>

#include "Announcer.hpp"
#include "Config.hpp"
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

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " CONFIG_FILE" << std::endl;
        std::cout << "CONFIG_FILE is the path to the json config file" << std::endl;
        return 1;
    }
    Config config(argv[1]);

    IPC::FifoSet fifoSet;
    PushListener pushListener(config, fifoSet);
    Controller controller(config, fifoSet);
    TtsDispatcher ttsDispatcher(config, fifoSet);
    Announcer announcer(config, fifoSet);

    std::latch threadsInitializedLatch(4);
    std::latch memoryLockedLatch(1);

    std::thread pushListenerThread([&]() {
        pthread_setname_np(pthread_self(), "PushListener");
        threadsInitializedLatch.count_down();
        memoryLockedLatch.wait();
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
        threadsInitializedLatch.count_down();
        memoryLockedLatch.wait();
        controller.threadFunc(keepRunning);
    });
    std::thread ttsDispatcherThread([&]() {
        pthread_setname_np(pthread_self(), "TtsDispatcher");
        threadsInitializedLatch.count_down();
        memoryLockedLatch.wait();
        ttsDispatcher.threadFunc(keepRunning);
    });
    std::thread announcerThread([&] {
        pthread_setname_np(pthread_self(), "Announcer");
        threadsInitializedLatch.count_down();
        memoryLockedLatch.wait();
        announcer.threadFunc(keepRunning);
    });

    threadsInitializedLatch.wait();
    // Lock all memory pages required for physical I/O and processing
    // May need root or CAP_IPC_LOCK
    if (mlockall(MCL_CURRENT) != 0) {
        std::cerr << "mlockall failed with " << errno << std::endl;
    }
    memoryLockedLatch.count_down();

    pushListenerThread.join();
    controllerThread.join();
    ttsDispatcherThread.join();
    announcerThread.join();

    return 0;
}
