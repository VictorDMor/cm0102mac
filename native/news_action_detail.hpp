#pragma once
#include "news_detail.hpp"

namespace cm::aot::news::detail {
struct ActionItem {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t value(unsigned slot) const { return c.u32(address+8+slot*4); }
    std::int8_t byte(unsigned slot) const { return std::int8_t(value(slot)); }
    std::int16_t short_value(unsigned slot) const { return std::int16_t(value(slot)); }
    std::uint32_t club(unsigned slot, std::uint32_t clubs_slot) const {
        return c.u32(clubs_slot)+value(slot)*0x248;
    }
};
inline void copy_action_date(Cpu& c,std::uint32_t destination,std::uint32_t source) {
    c.put16(destination,c.u16(source)); c.put16(destination+2,c.u16(source+2)); c.put8(destination+4,c.u8(source+4));
}
}
