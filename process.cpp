#include "process.hpp"

using namespace std::chrono_literals;
using namespace FireStation::Process;

void FireStation::Process::process(const Inputs &inputs, Outputs &outputs, State &state) {
    constexpr auto LightGarageToggleDuration = 500ms;

    outputs.LightYard = inputs.Gate1NotClosed || inputs.Gate2NotClosed;

    outputs.LightGarageToggle = state.LightGarageToggleOff > Clock::now();
    // Do not prolong toggling
    if (!outputs.LightGarageToggle) {
        // If any gate transitioned from closed to not closed
        if ((inputs.Gate1NotClosed && !state.Gate1NotClosedLast) ||
            (inputs.Gate2NotClosed && !state.Gate2NotClosedLast)) {
            // Only toggle if off
            if (inputs.LightGarageOff) {
                state.LightGarageToggleOff = Clock::now() + LightGarageToggleDuration;
            }
        }
    }

    state.Gate1NotClosedLast = inputs.Gate1NotClosed;
    state.Gate2NotClosedLast = inputs.Gate2NotClosed;
}
