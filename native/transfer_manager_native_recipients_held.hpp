#pragma once
// The routines of block "recipients" keep flags and records in non-volatile registers; callees save r13-r31 in
// their frames and later code reads those frames as stale stack, so each call holds the original's values there.
#include "guest_abi.hpp"
#include <array>
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// The original's r13-r31 as the routine holds them at its next call. `value[n]` is r<n>: assign where the original
// does, `publish` before each call.
struct RecipientsHeld {
    std::array<std::uint32_t, 32> value{};
    explicit RecipientsHeld(const guest::Cpu& c) { for (unsigned n = 13; n < 32; ++n) value[n] = c.r(n); }
    std::uint32_t& operator[](unsigned n) { return value[n]; }
    void publish(guest::Cpu& c) const { for (unsigned n = 13; n < 32; ++n) c.set(n, value[n]); }
};
}
