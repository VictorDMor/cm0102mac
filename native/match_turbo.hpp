#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// The native menu enables audited runtime shortcuts and reduces commentary
// presentation waits at maximum match speed. Original clocks, decisions,
// sound completion barriers, and persisted game data remain unchanged.
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
