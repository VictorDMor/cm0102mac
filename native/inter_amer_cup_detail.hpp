#pragma once
#include "inter_amer_cup.hpp"
#include "guest_values.hpp"

namespace cm::aot::inter_amer_cup::detail {
using namespace guest;
namespace slot {
constexpr int pool = -0x4090, vtable = -0x5cc0, clubs = -0x7be8,
    club_count = -0x7b50, competitions = -0x7bbc, continents = -0x7520,
    north_america = -0x6b98, south_america = -0x6b9c,
    north_champion = -0x68f8, south_champion = -0x6820;
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
struct Club {
    Cpu& c;
    std::uint32_t address;
    std::int16_t reputation() const { return c.i16(address + 0x1ec); }
    std::uint32_t nation() const { return c.u32(address + 0x2c); }
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
    report_assertion(c, {at, pool, pool + 0x13, pool + 0x20, f.at(name), f.at(extension), line});
}
}
