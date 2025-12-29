#pragma once

#include <variant>

#include "Alarms.hpp"
#include "Announcements.hpp"
#include "SpinQueue.hpp"

namespace FireStation::IPC {
// Send by terminating thread to outgoing Fifos with blocking consumers
struct Bye {};

struct NewAlarm {
    std::shared_ptr<const std::string> Text;
    std::chrono::steady_clock::time_point Time;
};

struct TtsOrderData {
    std::shared_ptr<const std::string> Text;
    TtsHash Hash;
};
using TtsOrder = std::variant<Bye, TtsOrderData>;

using Announcement = std::variant<Bye, StaticAnnouncement, TtsHash>;

using NewAlarmFifo = SpinQueue<NewAlarm>;
using TtsOrderFifo = SpinQueue<TtsOrder>; // TODO Use blocking Fifo but first remove unnecessary lock for submission in disruptorplus
using TtsReadyFifo = SpinQueue<TtsHash>;
using AnnouncementFifo = SpinQueue<Announcement>; // TODO Use blocking Fifo but first remove unnecessary lock for submission in disruptorplus

struct FifoSet {
    NewAlarmFifo NewAlarm{4};
    TtsOrderFifo TtsOrder{4};
    TtsReadyFifo TtsReady{8};
    AnnouncementFifo Announcement{8};
};
} // namespace FireStation::IPC
