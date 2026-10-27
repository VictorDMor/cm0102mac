#pragma once
#include "eng_prm.hpp"
#include "guest_values.hpp"

namespace cm::aot::eng_prm::detail {
using namespace guest;
constexpr std::uint32_t missing = 0xffffffff;
namespace slot {
constexpr int pool = -0x451c, vtable = -0x6020, divisions = -0x7b5c,
    premier = -0x67a0, first = -0x67a4, second = -0x67a8, third = -0x67ac,
    conference = -0x6874, club_count = -0x7b50, clubs = -0x7be8,
    records = -0x7bbc, date = -0x7b80;
}
struct Competition {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> definition() const { return {c, address + 4}; }
    Field<std::uint32_t> fixtures() const { return {c, address + 8}; }
    Field<std::uint32_t> standings() const { return {c, address + 0xb0}; }
    Field<std::uint32_t> schedule() const { return {c, address + 0xbc}; }
    Field<std::int16_t> club_count() const { return {c, address + 0x3e}; }
    Field<std::int16_t> year() const { return {c, address + 0x40}; }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> args = {}) const {
        unsigned n = 3;
        for (const auto value : args) c.set(n++, value);
        c.call(target, back);
        return c.r(3);
    }
    std::uint32_t virtual_call(std::uint32_t object, unsigned offset, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + offset));
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
        return c.r(3);
    }
    std::uint32_t division(int id_slot) const {
        return c.u32(c.u32(c.toc(slot::divisions)) + c.u32(c.toc(id_slot)) * 4);
    }
    std::uint32_t record(int id_slot) const {
        return c.u32(c.toc(slot::records)) + c.u32(c.toc(id_slot)) * 0x6c;
    }
};
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name,
                      unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool, pool + 0xc, pool + 0x19, f.at(name), f.at(extension), line});
}
inline bool byte_nonzero(Cpu& c, std::uint32_t value) {
    return compare(c, std::int8_t(value)) != 0;
}
}
