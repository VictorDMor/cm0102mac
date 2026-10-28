#pragma once
#include "match_pl.hpp"

namespace cm::aot::match_pl {
// MATCH_PLAYER is a guest record, never a host struct. The source leaves many
// fields untouched in its default constructor; preserve those bytes.
struct Player {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t match() const { return c.u32(address + 0x1a4); }
    std::uint8_t side() const { return c.u8(address + 0x27); }
    std::int8_t column() const { return c.i8(address + 0x105); }
    std::int8_t row() const { return c.i8(address + 0x106); }
};
}
