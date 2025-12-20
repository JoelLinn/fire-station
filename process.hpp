#pragma once

#include <chrono>

namespace FireStation::Process {
struct Inputs {
    bool AlarmBtn : 1;
    bool AlarmActive : 1;
    bool Gate1NotClosed : 1;
    bool Gate2NotClosed : 1;
    bool LightGarageOff : 1;
    bool Res6 : 1;
    bool Res7 : 1;
    bool Res8 : 1;
};

struct Outputs {
    bool LightParking : 1;
    bool LightYard : 1;
    bool Gate1Open : 1;
    bool Gate2Open : 1;
    bool LightGarageToggle : 1;
    bool Res6 : 1;
    bool Res7 : 1;
    bool Res8 : 1;
};

using Clock = std::chrono::steady_clock;

struct State {
    State() : Gate1NotClosedLast(false), Gate2NotClosedLast(false), LightGarageToggleOff(Clock::now()) {
    }

    bool Gate1NotClosedLast;
    bool Gate2NotClosedLast;
    Clock::time_point LightGarageToggleOff;
};

void process(const Inputs &inputs, Outputs &outputs, State &state);
} // namespace FireStation::Process
