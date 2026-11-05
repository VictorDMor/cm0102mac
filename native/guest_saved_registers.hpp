#pragma once

#include "guest_abi.hpp"

namespace cm::aot::guest {
// The exported multiword moves leave the transfer address live in TEA,
// including the address of the first word that faults.
template<unsigned First, bool Restore = false>
void transfer_saved_registers(Cpu& c, std::uint32_t caller_stack) {
    constexpr std::size_t tea = 2432;
    auto address = caller_stack - (32 - First) * 4;
    c.s.wr<4>(tea, address);
    for (unsigned r = First; r < 32; ++r) {
        if constexpr (Restore) c.set(r, c.u32(address));
        else c.put32(address, c.r(r));
        address += 4;
        c.s.wr<4>(tea, address);
    }
}

}
