#pragma once

#include <chrono>
#include <optional>
#include <type_traits>

#include "disruptorplus/blocking_wait_strategy.hpp"
#include "disruptorplus/ring_buffer.hpp"
#include "disruptorplus/sequence_barrier.hpp"
#include "disruptorplus/single_threaded_claim_strategy.hpp"
#include "disruptorplus/spin_wait_strategy.hpp"

namespace FireStation {
template <typename T, bool BLOCKING>
class GeneralizedQueue {
  private:
    using WaitStrategy = std::conditional_t<BLOCKING, disruptorplus::blocking_wait_strategy, disruptorplus::spin_wait_strategy>;

    disruptorplus::ring_buffer<T> _buffer;
    WaitStrategy _waitStrategy;
    disruptorplus::single_threaded_claim_strategy<WaitStrategy> _claimStrategy;
    disruptorplus::sequence_barrier<WaitStrategy> _consumed;

    disruptorplus::sequence_t _nextToRead;

  public:
    GeneralizedQueue() = delete;

    explicit GeneralizedQueue(size_t size)
        : _buffer(size), _waitStrategy(), _claimStrategy(size, _waitStrategy), _consumed(_waitStrategy), _nextToRead(0) {
        _claimStrategy.add_claim_barrier(_consumed);
    }

    void Put(T t) {
        // Claim a slot in the ring buffer, waits if buffer is full
        auto seq = _claimStrategy.claim_one();

        // Write to the slot in the ring buffer
        _buffer[seq] = std::move(t);

        // Publish the event to the consumer
        _claimStrategy.publish(seq);
    }

    bool TryPut(T t) {
        disruptorplus::sequence_range range;
        if (!_claimStrategy.try_claim(1, range) || !range.size()) {
            return false;
        }
        _buffer[range.first()] = std::move(t);
        _claimStrategy.publish(range.first());
        return true;
    }

    T Get() {
        // Wait until more items available
        [[maybe_unused]] auto available = _claimStrategy.wait_until_published(_nextToRead);

        auto t = std::move(_buffer[_nextToRead]);

        // Notify producer we've finished consuming one item
        _consumed.publish(_nextToRead++);

        return t;
    }

    std::optional<T> TryGet() {
        auto available = _claimStrategy.wait_until_published(_nextToRead, std::chrono::microseconds(-1));

        if (disruptorplus::difference(available, _nextToRead) < 0) {
            return std::nullopt;
        }
        auto t = std::move(_buffer[_nextToRead]);
        _consumed.publish(_nextToRead++);
        return t;
    }
};
} // namespace FireStation
