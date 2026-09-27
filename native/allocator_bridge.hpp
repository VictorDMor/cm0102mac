#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace cm::aot {
// Called only after the original pool allocation failed. Largest-free-block
// metadata is an optimistic hint; it must not make a retry repeat or grow.
inline std::uint32_t next_pool_attempt(std::uint32_t previous, std::uint32_t candidate) {
    if (previous < 10) throw std::runtime_error("Pool retry reached outside the audited allocation loop");
    return std::min(previous - 1, candidate);
}
}
