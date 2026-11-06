#pragma once

#include "aot_runtime.hpp"

namespace cm::aot::guest {

// Check the stack span from the deepest back chain through the saved link at
// caller SP + 8. The guarded calls write only within this span.
inline bool stack_frame_writes_observed(const Memory& memory, std::uint32_t caller, unsigned depth) {
    const auto* observer = memory.observer;
    return observer && (caller < depth || std::uint64_t(caller) + 12 > (UINT64_C(1) << 32) ||
                        observer->observes_write(caller - depth, depth + 12));
}

}
