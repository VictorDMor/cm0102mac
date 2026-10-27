#pragma once
#include "screen_manager.hpp"
#include "guest_values.hpp"

namespace cm::aot::screen_manager::detail {
using namespace guest;
constexpr std::uint32_t missing = 0xffffffff;
namespace slot {
constexpr int pool = -0x3e5c, message = -0x7ba8, success = -0x7bac;
}
struct Manager {
    Cpu& c;
    std::uint32_t address;
    Field<std::int16_t> current() const { return {c, address + 0x2c58}; }
    std::uint32_t group(std::int32_t index) const { return address + std::uint32_t(index) * 0x2c0; }
    std::uint32_t group() const { return group(current()); }
    Field<std::int16_t> count(std::int32_t index) const { return {c, address + 0x2c38 + std::uint32_t(index) * 2}; }
    Field<std::uint32_t> history() const { return {c, group() + 0x10}; }
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
    std::uint32_t imported(std::uint32_t target, std::uint32_t back,
                           std::initializer_list<std::uint32_t> args = {}) const {
        auto value = call(target, back, args);
        c.set(2, c.u32(c.sp() + 0x14));
        return value;
    }
    std::uint32_t callback(std::uint32_t target, std::uint32_t back) const {
        c.set(12, target);
        c.call(0x10781320, back);
        c.set(2, c.u32(c.sp() + 0x14));
        return c.r(3);
    }
};
inline bool byte_nonzero(Cpu& c, std::uint32_t value) {
    return compare(c, std::int8_t(value)) != 0;
}
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at, unsigned name,
                      unsigned extension, unsigned line, unsigned file_offset = 0x78) {
    auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool + file_offset, pool + 0xa, pool + 0x17,
                         f.at(name), f.at(extension), line});
}
}
