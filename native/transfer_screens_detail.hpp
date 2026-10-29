#pragma once
#include "transfer_screens.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_screens::detail {
using namespace guest;
constexpr std::uint32_t missing = 0xffffffff;
namespace slot {
constexpr int screens = -0x7bb0, current_contract = -0x7b78,
    contracts = -0x7b7c, clubs = -0x7be8, people = -0x7be0,
    transfers = -0x7b40, transfer_context = -0x7b80, pool = -0x4ad0;
}
struct Offer {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> club() const { return {c, address + 8}; }
    Field<std::uint32_t> person() const { return {c, address + 12}; }
    Field<std::uint32_t> revision() const { return {c, address + 0x2c}; }
    Field<std::uint8_t> status() const { return {c, address + 0x34}; }
    Field<std::uint8_t> response() const { return {c, address + 0x35}; }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> args = {}) const {
        unsigned r = 3;
        for (auto value : args) c.set(r++, value);
        c.call(target, back);
        return c.r(3);
    }
    void history(std::uint32_t back, std::uint32_t screens,
                 unsigned index, std::uint32_t value) const {
        call(0x1073f1e0, back, {screens, index, value, 0});
    }
};
inline bool byte_result(Cpu& c, std::uint32_t value) {
    return compare(c, std::int8_t(value)) != 0;
}
inline std::uint32_t indirect(Cpu& c, int offset) { return c.u32(c.toc(offset)); }
// Control actions use the signed low halfword; zero selects the screen default.
inline std::int16_t control_action(Cpu& c, std::uint32_t screens, std::uint32_t control) {
    const bool default_control = compare(c, std::int16_t(control), -1) == 0;
    c.set(3, c.u32(screens + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0 + 0x24));
    std::int16_t action = 0;
    if (!default_control) {
        c.set(3, c.r(3) + 0xc0000 + std::uint32_t(std::int16_t(control)) * 0x17c);
        action = std::int16_t(c.u32(c.r(3) - 0x5794));
    }
    if (compare(c, action) == 0) {
        c.set(3, screens + 0x260000); action = c.i16(c.r(3) - 0x7e62);
    }
    return action;
}
}
