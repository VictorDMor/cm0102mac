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
inline void copy_offer_fields(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    // Copy the fields in declaration order, leaving the original gaps intact.
    for (auto offset : {0u, 4u, 8u, 12u}) c.put32(destination + offset, c.u32(source + offset));
    c.put8(destination + 0x10, c.u8(source + 0x10)); c.put8(destination + 0x11, c.u8(source + 0x11));
    c.put16(destination + 0x16, c.u16(source + 0x16)); c.put16(destination + 0x14, c.u16(source + 0x14));
    c.put8(destination + 0x18, c.u8(source + 0x18)); c.put8(destination + 0x1c, c.u8(source + 0x1c));
    c.put32(destination + 0x20, c.u32(source + 0x20)); c.put8(destination + 0x24, c.u8(source + 0x24));
    for (auto offset : {0x28u, 0x2cu, 0x30u}) c.put32(destination + offset, c.u32(source + offset));
    for (auto offset : {0x34u, 0x35u, 0x36u, 0x37u, 0x38u}) c.put8(destination + offset, c.u8(source + offset));
    c.put32(destination + 0x3c, c.u32(source + 0x3c)); c.put32(destination + 0x40, c.u32(source + 0x40));
    c.put8(destination + 0x44, c.u8(source + 0x44)); c.put32(destination + 0x48, c.u32(source + 0x48));
    c.put8(destination + 0x4c, c.u8(source + 0x4c)); c.put8(destination + 0x50, c.u8(source + 0x50));
    c.put8(destination + 0x51, c.u8(source + 0x51));
}

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
