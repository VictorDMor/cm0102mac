#pragma once
#include "news.hpp"
#include "guest_values.hpp"

namespace cm::aot::news::detail {
using namespace guest;
namespace slot {
constexpr int pool = -0x5100, message = -0x7ba8, success = -0x7bac,
              today = -0x7b80, language = -0x79ec, first_manager = -0x7be4;
}
struct Item {
    Cpu& c;
    std::uint32_t address;
    static constexpr unsigned size = 0xe4, payload_size = 200, payload_words = 50;
    Field<std::int32_t> type() const { return {c, address}; }
    Field<std::uint8_t> priority() const { return {c, address + 4}; }
    std::uint32_t payload(unsigned index = 0) const { return address + 8 + index * 4; }
    Field<std::uint32_t> id() const { return {c, address + 0xdc}; }
};
struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back,
                       std::initializer_list<std::uint32_t> arguments = {}) const {
        unsigned reg = 3;
        for (auto argument : arguments) c.set(reg++, argument);
        c.call(target, back);
        return c.r(3);
    }
};
struct Queue {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint32_t> items() const { return {c, address}; }
    Field<std::int32_t> item_count() const { return {c, address + 4}; }
    Field<std::uint32_t> managers() const { return {c, address + 8}; }
    Field<std::int32_t> manager_count() const { return {c, address + 12}; }
    std::uint32_t item(std::uint32_t index) const { return items().get() + index * Item::size; }
};
struct Inbox {
    Cpu& c;
    std::uint32_t address;
    static constexpr unsigned size = 12, capacity = 100;
    Field<std::int32_t> first() const { return {c, address}; }
    Field<std::int32_t> next() const { return {c, address + 4}; }
    Field<std::int32_t> unread() const { return {c, address + 8}; }
};
inline std::uint32_t manager_index(Cpu& c, std::uint32_t manager) {
    const auto id = c.u32(manager);
    return id - c.u32(c.toc(slot::first_manager)) + 16;
}
// A news copy deliberately leaves the item's padding untouched. The paired
// word loads also preserve the original forward-copy behavior on aliases.
inline void copy_item(Cpu& c, std::uint32_t destination, std::uint32_t source,
                      unsigned value_register, unsigned source_register,
                      unsigned destination_register) {
    c.set(3, c.u32(source)); c.put32(destination, c.r(3));
    c.set(3, c.u8(source + 4)); c.put8(destination + 4, c.r(3));
    c.set(source_register, source + 4); c.set(destination_register, destination + 4);
    for (unsigned offset = 8; offset < 0xd0; offset += 8) {
        c.set(3, c.u32(c.r(source_register) + 4));
        const auto second = c.u32(c.r(source_register) + 8);
        c.set(source_register, c.r(source_register) + 8);
        c.put32(c.r(destination_register) + 4, c.r(3));
        c.set(destination_register, c.r(destination_register) + 8);
        c.put32(c.r(destination_register), second);
    }
    c.put16(destination + 0xd2, c.u16(source + 0xd2));
    c.put16(destination + 0xd0, c.u16(source + 0xd0));
    c.put8(destination + 0xd4, c.u8(source + 0xd4));
    c.put8(destination + 0xd8, c.u8(source + 0xd8));
    c.put32(destination + 0xdc, c.u32(source + 0xdc));
    c.put8(destination + 0xe0, c.u8(source + 0xe0));
    c.put8(destination + 0xe1, c.u8(source + 0xe1));
    c.set(value_register, 1);
}
inline void assertion(Cpu& c, const Frame& f, std::uint32_t at,
                      unsigned name, unsigned extension, unsigned line) {
    const auto pool = c.toc(slot::pool);
    report_assertion(c, {at, pool, pool + 9, pool + 0x16, f.at(name), f.at(extension), line});
}
inline bool valid_slot(Cpu& c, std::uint32_t value) {
    const auto index = std::int8_t(value);
    return compare(c, index) >= 0 && compare(c, index, Item::payload_words) < 0;
}
}
