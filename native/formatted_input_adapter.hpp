#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t, 7> formatted_input_entries{
    0x1077b7a0, 0x1077bf20, 0x1077d960, 0x1077da70, 0x1077dad0,
    0x1077d8d0, 0x1077d9f0};
struct FormattedInputBoundaryObserver {
    virtual ~FormattedInputBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t entry, bool original) = 0;
    virtual void leave(State&, std::uint32_t entry, bool original) = 0;
};
bool try_modern_formatted_input(State&, std::uint32_t entry);
void reset_formatted_input_diagnostics(State&);
std::string formatted_input_call_summary(const State&);
}
