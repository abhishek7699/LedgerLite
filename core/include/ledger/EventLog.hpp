#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace ledger {

// Append-only event log. Events are moved in; take() moves them out.
template <typename T>
class EventLog {
public:
    void record(T event) { events_.push_back(std::move(event)); }

    [[nodiscard]] std::size_t size() const noexcept { return events_.size(); }
    [[nodiscard]] const std::vector<T>& events() const noexcept { return events_; }

    // Empties the log and hands its events to the caller.
    [[nodiscard]] std::vector<T> take() { return std::exchange(events_, std::vector<T>{}); }

private:
    std::vector<T> events_;
};

}  // namespace ledger
