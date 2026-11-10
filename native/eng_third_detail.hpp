#pragma once
#include "eng_third.hpp"
#include "guest_values.hpp"

namespace cm::aot::eng_third::detail {
using namespace guest;
constexpr std::uint32_t missing = 0xffffffff;
// Table-of-contents offsets of the globals ENG_THIRD.CPP reads.
namespace slot {
constexpr int pool = -0x450c, vtable = -0x6018, third = -0x67ac, second_rules = -0x67a8,
    scheduling_rules = -0x6874, club_count = -0x7b50, clubs = -0x7be8, date = -0x7b80;
}
// The competition object (0xec bytes); the common base is built by 0x1049e4f0.
struct Competition {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> definition() const { return {c, address + 4}; }
    Field<std::uint32_t> fixtures() const { return {c, address + 8}; }
    Field<std::uint32_t> stages() const { return {c, address + 0xc}; }
    Field<std::int32_t> last_stage() const { return {c, address + 0x30}; }
    Field<std::int16_t> club_count() const { return {c, address + 0x3e}; }
    Field<std::int16_t> year() const { return {c, address + 0x40}; }
    Field<std::uint32_t> standings() const { return {c, address + 0xb0}; }
    Field<std::uint32_t> schedule() const { return {c, address + 0xbc}; }
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
    std::uint32_t virtual_call(std::uint32_t object, unsigned offset, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + offset));
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
        return c.r(3);
    }
};
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name,
                      unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool, pool + 0xe, pool + 0x1b, f.at(name), f.at(extension), line});
}
}
