#include "ui_bridge.hpp"
#include "profiling.hpp"
#include <algorithm>
#include <chrono>

namespace cm::aot {
void UIEventQueue::push_back(UIEvent event) {
    {
        std::lock_guard lock(mutex_);
        // The window server used to coalesce these while the game was busy:
        // the cooperative pump stopped at the first pending event. Main now
        // translates every event as it arrives, so do the same here. Only the
        // newest of consecutive pointer movements matters (same window and
        // modifiers), and a held key repeats once until the game reads it.
        // Order relative to every other event is unchanged.
        if (!events_.empty()) {
            auto& last = events_.back();
            const bool same = last.kind == event.kind && last.window == event.window &&
                last.modifiers == event.modifiers;
            if (same && event.kind == UIEvent::Kind::mouse_moved) { last = std::move(event); return; }
            if (same && event.kind == UIEvent::Kind::auto_key && last.message == event.message) return;
        }
        events_.push_back(std::move(event));
    }
    changed_.notify_one();
}
bool UIEventQueue::pop(UIEvent& event, double timeout) {
    std::unique_lock lock(mutex_);
    if (events_.empty() && timeout > 0) {
        CM_PROFILE_ZONE("Input queue wait");
        changed_.wait_for(lock, std::chrono::duration<double>(timeout), [&] { return !events_.empty(); });
    }
    if (events_.empty()) return false;
    event = std::move(events_.front());
    events_.pop_front();
    return true;
}
bool UIEventQueue::take(UIEvent& event, UIEvent::Kind kind) {
    std::lock_guard lock(mutex_);
    const auto found = std::find_if(events_.begin(), events_.end(), [kind](const auto& item) { return item.kind == kind; });
    if (found == events_.end()) return false;
    event = std::move(*found); events_.erase(found); return true;
}
bool UIEventQueue::contains(UIEvent::Kind kind) const {
    std::lock_guard lock(mutex_);
    return std::any_of(events_.begin(), events_.end(), [kind](const auto& item) { return item.kind == kind; });
}
bool UIEventQueue::empty() const { std::lock_guard lock(mutex_); return events_.empty(); }
UIEvent UIEventQueue::front() const { std::lock_guard lock(mutex_); return events_.front(); }
void UIEventQueue::remove_window(std::uint32_t window) {
    std::lock_guard lock(mutex_);
    std::erase_if(events_, [window](const auto& event) { return event.window == window; });
}
bool UIEventQueue::flush(std::uint16_t which, std::uint16_t stop) {
    std::lock_guard lock(mutex_);
    for (auto it = events_.begin(); it != events_.end();) {
        const auto mask = legacy_event_mask(*it);
        if (mask & stop) return true;
        if (mask & which) it = events_.erase(it); else ++it;
    }
    return false;
}
}
