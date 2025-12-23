#include <future>
#include <iostream>

#include <signal.h>

#include "Format.hpp"
#include "process.hpp"
#include "simple_ng.h"

static bool keepRunning{true};
using namespace FireStation;
static Process::State state;

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
        printf("Usage: fire-station IFNAME\n"
               "IFNAME is the NIC interface name, e.g. 'eth0'\n");
        return 1;
    }
    const auto *ifname = argv[1];

    // TODO gotify websocket stream thread
    // Will put new alarms in ring buffer to process thread

    const auto processCallbackProxy = [](const void *in, void *out) {
        Process::process(*reinterpret_cast<const Process::Inputs *>(in), *reinterpret_cast<Process::Outputs *>(out), state);
    };
    auto plcFuture = std::async(plc_thread, ifname, processCallbackProxy, &keepRunning);

    // TODO audio announcement thread
    // will receive announcement and audio file play requests from process thread

    return plcFuture.get();
}
