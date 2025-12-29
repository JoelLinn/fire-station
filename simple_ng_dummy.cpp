#include "Controller.hpp"
#include "simple_ng.h"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

struct SFieldbus {
    process_callback_t process_callback;
    void *userdata;
};

Fieldbus *
fieldbus_alloc(void) {
    return new Fieldbus();
}

void fieldbus_free(Fieldbus *fieldbus) {
    delete fieldbus;
}

void fieldbus_initialize(Fieldbus *fieldbus, const char *, process_callback_t process_callback, void *userdata) {
    fieldbus->process_callback = process_callback;
    fieldbus->userdata = userdata;
}

bool fieldbus_start(Fieldbus *) { return true; }

void fieldbus_loop(Fieldbus *fieldbus, const volatile bool *keep_running) {
    FireStation::Controller::Inputs in{};
    FireStation::Controller::Outputs out{};

    while (*keep_running) {
        fieldbus->process_callback(fieldbus->userdata, &in, &out);
        std::this_thread::sleep_for(100ms);
    }
}