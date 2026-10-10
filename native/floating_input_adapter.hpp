#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
inline constexpr std::array<std::uint32_t, 1> floating_input_entries{0x1077de10};
struct FloatingInputBoundaryObserver {
    virtual ~FloatingInputBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original) = 0;
};
bool try_modern_floating_input(State&, std::uint32_t entry);
void reset_floating_input_diagnostics(State&);
std::string floating_input_call_summary(const State&);
}
