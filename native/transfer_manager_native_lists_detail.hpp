#pragma once
// Shared by the "lists" block: the game's five-byte dates and the compares the original makes of them.
#include "guest_abi.hpp"
#include "transfer_manager_native_detail.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native::lists {
// A date is a signed halfword (+0), an unsigned halfword (+2: the year) and a byte (+4).
struct Date {
    std::int16_t first;
    std::uint16_t year;
    std::uint8_t last;
};
inline Date read_date(guest::Cpu& c, std::uint32_t at) { return {c.i16(at), c.u16(at + 2), c.u8(at + 4)}; }
inline void write_date(guest::Cpu& c, std::uint32_t at, const Date& date) {
    c.put16(at + 2, date.year);
    c.put16(at, std::uint16_t(date.first));
    c.put8(at + 4, date.last);
}
// The original's order of two dates: by the unsigned year, then by the signed halfword at +0.
inline bool date_after(const Date& a, const Date& b) {
    if (a.year != b.year) return a.year > b.year;
    return a.first > b.first;
}

// The 0x28-byte dated entries of the history (the manager's table at +0x994 and the yearly files): the date is at +0x20.
inline constexpr std::uint32_t entry_size = 0x28, entry_date = 0x20;
inline Date entry_date_of(guest::Cpu& c, std::uint32_t entry) { return read_date(c, entry + entry_date); }
// The copy the original makes field by field (words to +0x18, a byte at +0x1c, the date's year and first halfword,
// its byte): the bytes between are not copied.
inline void copy_entry(guest::Cpu& c, std::uint32_t to, std::uint32_t from) {
    for (std::uint32_t at = 0; at <= 0x18; at += 4) c.put32(to + at, c.u32(from + at));
    c.put8(to + 0x1c, c.u8(from + 0x1c));
    c.put16(to + 0x22, c.u16(from + 0x22));
    c.put16(to + 0x20, c.u16(from + 0x20));
    c.put8(to + 0x24, c.u8(from + 0x24));
}
}
