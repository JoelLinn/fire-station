#include <iostream>

#include "Format.hpp"
#include "process.hpp"
#include "simple_ng.h"

static bool keepRunning{true};
using namespace FireStation;
static Process::State state;

static void processCallbackProxy(const void *in, void *out) {
    Process::process(*reinterpret_cast<const Process::Inputs *>(in), *reinterpret_cast<Process::Outputs *>(out), state);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: fire-station IFNAME\n"
               "IFNAME is the NIC interface name, e.g. 'eth0'\n");
        return 1;
    }
    const auto *ifname = argv[1];

    return plc_thread(ifname, processCallbackProxy, &keepRunning);

    return 0;
}
