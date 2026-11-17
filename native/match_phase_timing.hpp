#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>
#include <type_traits>

#ifndef CM_MATCH_PHASE_TIMING
#define CM_MATCH_PHASE_TIMING 0
#endif

namespace cm::aot::match_phase_timing {
enum class Phase { advance, clock_and_restarts, tactics_and_ball, marking, actor_turns, reports,
                   event_simulation, actor_dispatch, context_action, secondary_action, tendencies, count };
struct Total { std::uint64_t calls = 0, nanoseconds = 0; };
class Recorder;
inline thread_local Recorder* active = nullptr;

// Replay-only host timing: no guest hooks, memory observers, finite budgets or
// PC sampler. The normal game's native paths stay eligible while recording.
class Recorder {
    friend class Scope;
    Recorder* previous_;
    std::uint64_t (*now_)();
    std::array<Total, static_cast<unsigned>(Phase::count)> totals_{};
public:
    explicit Recorder(std::uint64_t (*clock)() = nullptr);
    ~Recorder();
    Recorder(const Recorder&) = delete;
    Recorder& operator=(const Recorder&) = delete;
    const auto& totals() const { return totals_; }
    void report(std::ostream&) const;
};

class Scope {
    Recorder* recorder_;
    Phase phase_;
    std::uint64_t started_ = 0;
    void finish(std::uint64_t now);
public:
    explicit Scope(Phase phase) : recorder_(active), phase_(phase) {
        if (recorder_) started_ = recorder_->now_();
    }
    ~Scope() { if (recorder_) finish(recorder_->now_()); }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    void next(Phase phase) {
        if (!recorder_) return;
        const auto now = recorder_->now_();
        finish(now);
        phase_ = phase;
        started_ = now;
    }
};
class DisabledScope {
public:
    explicit constexpr DisabledScope(Phase) noexcept {}
    constexpr void next(Phase) noexcept {}
};
// The regular app/replay erase every timing scope. A separate diagnostic
// executable compiles the same selected owners with real scopes; the classes and
// shared inline helpers retain identical definitions in both builds.
template<bool Enabled>
using OptionalScope = std::conditional_t<Enabled, Scope, DisabledScope>;
}
