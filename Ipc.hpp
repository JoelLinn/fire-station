#pragma once

#include <variant>

#include "Alarms.hpp"
#include "Announcements.hpp"
#include "GeneralizedQueue.hpp"

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

using NewAlarmFifo = GeneralizedQueue<NewAlarm, false>;
using TtsOrderFifo = GeneralizedQueue<TtsOrder, true>;
using TtsReadyFifo = GeneralizedQueue<TtsHash, false>;
using AnnouncementFifo = GeneralizedQueue<Announcement, true>;

struct FifoSet {
    NewAlarmFifo NewAlarm{4};
    TtsOrderFifo TtsOrder{4};
    TtsReadyFifo TtsReady{8};
    AnnouncementFifo Announcement{4};
};
} // namespace FireStation::IPC
