#pragma once
#include "awol.hpp"
#include "guest_values.hpp"
#include <initializer_list>

namespace cm::aot::awol::detail {
using namespace guest;
// The records object.
inline constexpr std::uint32_t primary_list = 0, secondary_list = 4, pending_list = 8, flags_buffer = 12;
inline constexpr std::uint32_t primary_bit = 1, secondary_bit = 2;
// Staff (0x70 bytes) and club (0x248 bytes) tables.
inline constexpr std::uint32_t staff_size = 0x70, club_size = 0x248;
inline constexpr std::uint32_t staff_profile = 0x14, staff_club = 0x24;
inline constexpr std::uint32_t club_second = 0x90, club_roster = 0x98, club_coaches = 0x160;
// Calls to the rest of the game.
inline constexpr std::uint32_t list_next = 0x10738e00, list_remove = 0x10738e60, list_append = 0x10738d10,
                               list_construct = 0x10738c00, list_destroy = 0x10738c20, contract_of = 0x10082f20,
                               date_construct = 0x10715880, reseed = 0x1077b770;

inline std::uint32_t call(Cpu& c, std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto value : arguments) c.set(n++, value);
    c.call(target, back);
    return c.r(3);
}
inline std::uint32_t signed_byte(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
inline std::int32_t multiply_high(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); }
// `lwz r0,4(list); stw r0,12(list)`: restart the list's cursor at its first node.
inline void rewind(Cpu& c, std::uint32_t list) { c.put32(list + 12, c.u32(list + 4)); }
inline std::int32_t list_count(Cpu& c, std::uint32_t list) { return c.i16(list); }

// The club a contract belongs to; null without a contract. The original keeps
// the contract in r3 until it replaces it with the club table.
inline std::uint32_t club_of_contract(Cpu& c, std::uint32_t contract) {
    if (compare_logical(c, contract) == 0) return 0;
    const auto index = c.u32(contract + 4);
    c.set(3, c.u32(c.toc(-0x7be8)));
    return c.r(3) + index * club_size;
}

// The shared table of random words. The cursor steps through it two bytes at
// a time; past its end it restarts and the reseed routine gives a new salt.
struct Pool {
    Cpu& c;
    std::uint32_t constants, salt, base, end, cursor;
    static Pool at(Cpu& c) { return {c, c.toc(-0x5a0c), c.toc(-0x7b90), c.toc(-0x7b8c), c.toc(-0x7b88), c.toc(-0x7b84)}; }
    std::uint32_t position() const { return c.u32(cursor); }
    std::uint32_t bias() const { return c.u32(salt); }
    void advance_from(std::uint32_t current, std::uint32_t back) const {
        const auto next = current + 2;
        c.set(3, next);
        const auto order = compare_logical(c, next, c.u32(end));
        c.put32(cursor, next);
        if (order > 0) {
            c.put32(cursor, base);
            c.call(reseed, back);
            c.put32(salt, c.r(3) & 0xffff);
        }
    }
    void advance(std::uint32_t back) const { advance_from(position(), back); }
};
inline std::uint32_t remainder_of(std::uint32_t value, std::uint32_t bound) {
    return value - std::uint32_t(divide_signed<4>(value, bound)) * bound;
}
// One inline draw. Every bound is a sign-extended byte or halfword, or a byte
// plus ten (tools/test_awol_audit.py pins each), so the original's arm for bounds
// beyond 65535 in size, and its call of sample, cannot run from these sites.
struct Draw {
    std::uint32_t start;     // the draw's first instruction
    unsigned bound, result;  // the non-volatile registers holding them
    bool cached = false;     // advances the cursor still in r3
};
// Draws in [0, bound) (toward the bound's sign), inlined at 31 sites. Leaves
// the result in its register and returns it.
std::int32_t draw(Cpu&, const Pool&, const Draw&);
// The compiler's `bound ? draw : 0`, tested with the record form just made.
inline std::int32_t draw_or_zero(Cpu& c, const Pool& pool, const Draw& site) {
    if (compare(c, std::int32_t(c.r(site.bound))) == 0) { c.set(site.result, 0); return 0; }
    return draw(c, pool, site);
}
}
