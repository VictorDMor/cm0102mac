#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// The native UI enables audited runtime shortcuts independently of the chosen
// presentation speed. Headless reference checks can still toggle this policy.
// The original speed selector owns the separate Turbo presentation choice.
bool match_turbo_enabled();
void set_match_turbo_enabled(bool enabled);
// Sparse diagnostic samples; enabled only for a separate profile run.
bool match_pc_profile_enabled();
std::uint64_t match_pc_sample_threshold(std::uint64_t executed);
std::uint64_t record_match_pc_sample(std::uint32_t address);
bool match_replay_turbo_enabled();

template <bool Turbo, bool Finite = false> class MatchPageCounter {
public:
    explicit MatchPageCounter(State& state) : state_(state), count_(state.executed) {
        if constexpr (Turbo) {
            profiling_ = match_pc_profile_enabled();
            if (profiling_) next_sample_ = match_pc_sample_threshold(count_);
        }
    }
    ~MatchPageCounter() {
        if constexpr (Turbo) state_.executed = count_;
    }
    void publish(std::uint32_t address) {
        if constexpr (Turbo) {
            state_.pc = address;
            state_.executed = count_;
        }
    }
    // A semantic call can change State's instruction charge or throw after
    // doing so. Publish before entry and adopt its charge on either outcome;
    // the page destructor must not overwrite it with a stale local count.
    template <typename Operation> bool semantic_boundary(std::uint32_t address, Operation&& operation) {
        publish(address);
        try {
            const bool handled = operation();
            if constexpr (Turbo) count_ = state_.executed;
            return handled;
        } catch (...) {
            if constexpr (Turbo) count_ = state_.executed;
            throw;
        }
    }
    void step(std::uint32_t address) {
        if constexpr (Turbo) {
            state_.pc = address;
            if constexpr (Finite) {
                if (count_++ >= state_.budget) instruction_budget_exhausted();
            } else {
                if (count_++ == UINT64_MAX) instruction_budget_exhausted();
            }
            if (profiling_ && count_ >= next_sample_)
                next_sample_ = record_match_pc_sample(address);
        } else {
            state_.step(address);
        }
    }
private:
    State& state_;
    std::uint64_t count_;
    std::uint64_t next_sample_ = UINT64_MAX;
    bool profiling_ = false;
};

}
