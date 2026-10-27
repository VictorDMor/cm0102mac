#pragma once
#include "pol_cup.hpp"
#include "guest_values.hpp"

namespace cm::aot::pol_cup::detail {
using namespace guest;
namespace slot {
constexpr int pool = -0x40ec, vtable = -0x5d0c, clubs = -0x7be8,
    club_count = -0x7b50, excluded_count = -0x7b04, first_division = -0x7b58,
    second_division = -0x69fc, lower_division = -0x6a08;
}
struct Cup {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> fixtures() const { return {c, address + 8}; }
    Field<std::uint32_t> calendar() const { return {c, address + 0xa0}; }
    Field<std::uint32_t> entrants() const { return {c, address + 0xa4}; }
    Field<std::uint32_t> round_storage() const { return {c, address + 0xac}; }
    Field<std::int16_t> rounds() const { return {c, address + 0x3c}; }
    Field<std::int16_t> club_count() const { return {c, address + 0x3e}; }
    Field<std::int16_t> year() const { return {c, address + 0x40}; }
};
// League finalization (0x104aa340) records the division and finishing
// position in +0x34/+0x23c; +0x30 remains the club's current division.
struct Club {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t recorded_division() const { return c.u32(address + 0x34); }
    std::uint32_t current_division() const { return c.u32(address + 0x30); }
    std::int8_t recorded_position() const { return c.i8(address + 0x23c); }
    static constexpr unsigned size = 0x248;
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> args = {}) const {
        unsigned n = 3;
        for (auto value : args) c.set(n++, value);
        c.call(target, back);
        return c.r(3);
    }
    std::uint32_t virtual_call(std::uint32_t object, unsigned method, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + method));
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
        return c.r(3);
    }
};
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name,
                      unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool, pool + 0xc, pool + 0x19, f.at(name), f.at(extension), line});
}
}
