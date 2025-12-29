#include "Controller.hpp"
#include "Announcements.hpp"
#include "simple_ng.h"

#include <iostream>

using namespace std::chrono_literals;

constexpr auto ALARM_MAX_AGE = 20min;
constexpr auto LIGHT_GARAGE_TOGGLE_DURATION = 500ms;

FireStation::Controller::Controller(IPC::FifoSet &fifoSet, const char *iface /*TODO conf*/)
    : Gate1NotClosedLast(false), Gate2NotClosedLast(false), LightGarageToggleOff(Clock::now()), FifoSet(fifoSet) {
    IoFieldbus = fieldbus_alloc();
    if (!IoFieldbus) {
        throw std::runtime_error("Failed to allocate fieldbus");
    }
    fieldbus_initialize(IoFieldbus, iface, [](void *userdata, const void *in, void *out) {
        if (!userdata || !in || !out) {
            return;
        }
        auto process = reinterpret_cast<Controller *>(userdata);
        process->process(*reinterpret_cast<const Controller::Inputs *>(in), *reinterpret_cast<Controller::Outputs *>(out)); }, this);
    if (!fieldbus_start(IoFieldbus)) {
        throw std::runtime_error("Failed to start fieldbus");
    }

    Alarms.reserve(4);
}

FireStation::Controller::~Controller() {
    if (IoFieldbus) {
        fieldbus_free(IoFieldbus);
        IoFieldbus = nullptr;
    }
}

void FireStation::Controller::process(const Inputs &inputs, Outputs &outputs) {
    const auto now = Clock::now();

    // Clear old alarms
    {
        std::erase_if(Alarms, [now](const auto &alarm) {
            return alarm.Time + ALARM_MAX_AGE <= now;
        });
    }

    // New alarms
    for (;;) {
        auto alarm = FifoSet.NewAlarm.TryGet();
        if (!alarm) {
            break;
        }

        TtsHash alarmHash;
        calculateSha256(alarm->Text->data(), alarm->Text->size(), alarmHash.data());
        if (Alarms.size() + 1 == Alarms.capacity()) {
            std::cerr << "Too many alarms, removing oldest" << std::endl;
            Alarms.erase(Alarms.begin());
        }
        // Save to our List
        Alarms.push_back({alarmHash, alarm->Time, false});
        FifoSet.TtsOrder.TryPut(IPC::TtsOrderData{std::move(alarm->Text), alarmHash});

        // TODO implement full announcement logic
        FifoSet.Announcement.TryPut(StaticAnnouncement::GONG_SHORT);
    }

    // Update alarm tts status
    for (;;) {
        auto ready = FifoSet.TtsReady.TryGet();
        if (!ready) {
            break;
        }

        const auto alarmIt = std::ranges::find_if(Alarms, [&ready](const auto &alarm) {
            return std::equal(alarm.Hash.begin(), alarm.Hash.end(), ready->begin());
        });
        if (alarmIt == Alarms.end()) {
            std::cerr << "Tts ready for unknown alarm" << std::endl;
            continue;
        }
        alarmIt->TtsReady = true;

        // TODO implement full announcement logic
        FifoSet.Announcement.TryPut(alarmIt->Hash);
    }

    {
        outputs.LightYard = inputs.Gate1NotClosed || inputs.Gate2NotClosed;

        outputs.LightGarageToggle = LightGarageToggleOff > now;
        // Do not prolong toggling
        if (!outputs.LightGarageToggle) {
            // If any gate transitioned from closed to not closed
            if ((inputs.Gate1NotClosed && !Gate1NotClosedLast) ||
                (inputs.Gate2NotClosed && !Gate2NotClosedLast)) {
                // Only toggle if off
                if (inputs.LightGarageOff) {
                    LightGarageToggleOff = now + LIGHT_GARAGE_TOGGLE_DURATION;
                }
            }
        }

        outputs.LightParking = Alarms.size() > 0;

        if (inputs.AlarmBtn && !AlarmBtnLast) {
            // TODO implement full announcement logic
            FifoSet.Announcement.TryPut(StaticAnnouncement::GONG_SHORT);
        }

        AlarmBtnLast = inputs.AlarmBtn;
        Gate1NotClosedLast = inputs.Gate1NotClosed;
        Gate2NotClosedLast = inputs.Gate2NotClosed;
    }
}

void FireStation::Controller::threadFunc(const bool &keepRunning) {
    fieldbus_loop(IoFieldbus, &keepRunning);

    FifoSet.TtsOrder.Put(IPC::Bye{});
    FifoSet.Announcement.Put(IPC::Bye{});
}
