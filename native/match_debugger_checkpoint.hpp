#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::match_debugger {
// Process-local checkpoints share unchanged guest pages. No history is written
// to the game directory or SSD, and no ordinary game save is involved.
class Checkpoint {
    struct Page {
        std::uint32_t address;
        std::shared_ptr<const std::vector<std::uint8_t>> bytes;
    };
    std::vector<Page> pages_;
    decltype(State::registers) registers_;
    decltype(State::unique) unique_;
    decltype(State::allocated) allocated_, available_;
    std::uint32_t pc_, heap_end_;
    std::uint64_t executed_, budget_, native_calls_;
    std::int16_t memory_error_;
    std::shared_ptr<NativePlatform> platform_;
public:
    Checkpoint(const State&, const Checkpoint* previous = nullptr);
    std::unique_ptr<State> restore() const;
    // A session operation captures before its temporary instruction budget is
    // unwound; restore the caller budget, not the operation's temporary limit.
    void caller_budget(std::uint64_t value) { budget_ = value; }
    std::uint64_t budget() const { return budget_; }
    std::size_t newly_owned_bytes(const Checkpoint* previous = nullptr) const;
};
}
