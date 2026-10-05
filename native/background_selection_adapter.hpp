#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 4> background_selection_entries{
    0x107122e0, 0x10712a60, 0x10713960, 0x107139c0};

// Private contract-harness observation. Production leaves this null; it is
// independent of both existing memory observers and native drawing.
struct BackgroundBoundaryObserver {
    virtual ~BackgroundBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_background_selection(State&, std::uint32_t entry);
void reset_background_diagnostics(State&);
std::string background_selection_call_summary(const State&);
}
