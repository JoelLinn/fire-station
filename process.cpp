#include "process.hpp"

using namespace std::chrono_literals;
using namespace FireStation::Process;

constexpr auto LIGHT_GARAGE_TOGGLE_DURATION = 500ms;

void FireStation::Process::process(const Inputs &inputs, Outputs &outputs, State &state) {
    const auto now = Clock::now();

    outputs.LightYard = inputs.Gate1NotClosed || inputs.Gate2NotClosed;

    outputs.LightGarageToggle = state.LightGarageToggleOff > now;
    // Do not prolong toggling
    if (!outputs.LightGarageToggle) {
        // If any gate transitioned from closed to not closed
        if ((inputs.Gate1NotClosed && !state.Gate1NotClosedLast) ||
            (inputs.Gate2NotClosed && !state.Gate2NotClosedLast)) {
            // Only toggle if off
            if (inputs.LightGarageOff) {
                state.LightGarageToggleOff = now + LIGHT_GARAGE_TOGGLE_DURATION;
            }
        }
    }

    state.Gate1NotClosedLast = inputs.Gate1NotClosed;
    state.Gate2NotClosedLast = inputs.Gate2NotClosed;
}
