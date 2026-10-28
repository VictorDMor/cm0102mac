#pragma once
#include "oceania_nations.hpp"
#include "guest_values.hpp"
namespace cm::aot::oceania_nations::detail {
using namespace guest;
struct Cup {
    Cpu& c; std::uint32_t address;
    Field<std::uint32_t> fixtures() const { return {c, address + 8}; }
    Field<std::uint32_t> stages() const { return {c, address + 0xc}; }
    Field<std::int16_t> year() const { return {c, address + 0x40}; }
    Field<std::int32_t> current_stage() const { return {c, address + 0x30}; }
    Field<std::uint32_t> entrants() const { return {c, address + 0xec}; }
    Field<std::int16_t> entrant_count() const { return {c, address + 0xf0}; }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> args = {}) const {
        unsigned n = 3; for (auto value : args) c.set(n++, value);
        c.call(target, back); return c.r(3);
    }
    std::uint32_t virtual_call(std::uint32_t object, unsigned method, std::uint32_t back) const {
        c.set(12, c.u32(c.u32(object) + method)); c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14)); return c.r(3);
    }
};
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name,
                      unsigned extension, unsigned line) {
    const auto pool = c.toc(-0x4094);
    report_assertion(c, {at, pool, pool + 0x14, pool + 0x21, f.at(name), f.at(extension), line});
}
inline std::uint32_t signed_byte(std::uint32_t n) { return std::uint32_t(std::int32_t(std::int8_t(n))); }
inline std::uint32_t signed_half(std::uint32_t n) { return std::uint32_t(std::int32_t(std::int16_t(n))); }
}
