#pragma once

#include "guest_values.hpp"

namespace cm::aot::staff_contracts::detail {
using guest::Cpu;

// Keep the original straight-line budget boundaries for a stop inside a
// partially completed operation. The algorithm between them uses C++ values.
inline void block(Cpu& c, std::uint32_t pc, unsigned instructions) {
    c.s.step(pc);
    c.s.charge(instructions - 1);
}

struct ContractRecord {
    static constexpr unsigned bonus = 0x10;
    static constexpr unsigned option = 0x1c;
    static constexpr unsigned release_amount = 0x24;
    static constexpr unsigned end_date = 0x30;
    static constexpr unsigned packed_kind = 0x38;
    static constexpr unsigned status = 0x48;
    static constexpr unsigned terms_flag = 0x54;
    static constexpr unsigned terms_marker = 0x58;
};

inline std::int32_t unpack_kind(std::uint8_t packed) {
    return std::int8_t(packed & 0xfc) / 4;
}
}
