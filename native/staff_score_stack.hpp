#pragma once

#include "aot_runtime.hpp"

namespace cm::aot {

inline bool can_forward_staff_score_stack(const State& s) {
    // A dispatch into the middle of the block has no prior stores to reuse.
    constexpr std::uint32_t entry = 0x101af000;
    if (s.pc != entry) return false;
    const auto& memory = s.memory;
    const auto frame = std::uint32_t(s.r<4>(4));
    return std::uint64_t(frame) + 0x1f0 <= (UINT64_C(1) << 32) &&
        !memory.date_observer && !memory.read_trace &&
        !(memory.observer && memory.observer->observes_write(frame, 0x1f0));
}

}
