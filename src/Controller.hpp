#pragma once

#include <chrono>
#include <vector>

#include "Alarms.hpp"
#include "Ipc.hpp"

typedef struct SFieldbus Fieldbus;

namespace FireStation {

class Controller {
  public:
    using Clock = std::chrono::steady_clock;
    struct Inputs {
        // EL1008
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
        // EL2008
        bool LightParking : 1;
        bool LightYard : 1;
        bool Gate1Open : 1;
        bool Gate2Open : 1;
        bool LightGarageToggle : 1;
        bool Res6 : 1;
        bool Res7 : 1;
        bool Res8 : 1;
    };

    Controller(const Config &config, IPC::FifoSet &fifoSet);
    ~Controller();

    void threadFunc(const bool &keepRunning);

  private:
    void process(const Inputs &inputs, Outputs &outputs);

  private:
    bool AlarmBtnLast;
    bool AlarmActiveLast;
    bool Gate1NotClosedLast;
    bool Gate2NotClosedLast;
    Clock::time_point LightGarageToggleOff;

    const Config &Conf;
    IPC::FifoSet &FifoSet;
    Fieldbus *IoFieldbus;

    struct Alarm {
        AlarmId Id;
        TtsHash Hash{};
        IPC::GatesType Gates;
        Clock::time_point Time{};
        bool TtsReady{false};
    };
    std::vector<Alarm> Alarms;
};
} // namespace FireStation
