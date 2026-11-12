#pragma once
// The bans block's records (offsets are the original's; see discipline_detail.hpp for the manager).
#include "discipline_detail.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// The manager's two arrays beside the person table:
//   +0x04 the ban records (pointer), +0x08 their capacity and +0x0a their count (signed halfwords)
//   +0x0c the sorted index (pointer) of 8-byte entries {person, key}, +0x10 capacity, +0x12 count;
//         sorted by the comparator at TOC -0x560c (0x100e9290: by key, then by person; person -1
//         matches any); key is the word at +0x68 of the record at person+0xc
namespace manager_arrays {
inline constexpr std::uint32_t bans = 0x04, ban_capacity = 0x08, ban_count = 0x0a, sorted = 0x0c, sorted_capacity = 0x10,
                               sorted_count = 0x12;
inline constexpr std::uint32_t sorted_entry_size = 8;
}
// A ban record (0x18 bytes). Free: matches == -1 and days == -1.
//   +0x00 matches (signed byte; -1: a days ban), +0x01 matches served
//   +0x02 days (signed halfword; -1 unused)
//   +0x04 date (halfword at +4 is its high half, the unsigned halfword at +6 its low half):
//         a days ban's date plus its days is when it ends (0x10717890); a matches ban's date is compared with today
//   +0x0c four ruling body indices (signed bytes, -1 unused), +0x14 flags (bit 7: merged)
namespace ban {
inline constexpr std::uint32_t size = 0x18, matches = 0, served = 1, days = 2, date_high = 4, date_low = 6,
                               bodies = 0xc, flags = 0x14;
inline constexpr std::uint8_t merged = 0x80;
inline constexpr int body_slots = 4;
}
// What RULING_BODY.CPP's lookup (0x105bc2b0) returns: the ban's record index is the halfword at +6.
inline constexpr std::uint32_t body_entry_index = 6;
// The ruling body object: +0x2c points at the record whose word at +0x68 is its key in the sorted index.
inline constexpr std::uint32_t ruling_body_key_source = 0x2c;
// The person record (TOC -0x7be0 table, 0x70 bytes): +0x0c and +0x10 point at records whose word
// at +0x68 is the key of the sorted index.
namespace person_record {
inline constexpr std::uint32_t size = 0x70, key_source = 0x0c, second_key_source = 0x10, key = 0x68;
}
// Table-of-contents words (Cpu::toc(offset): the word at r2+offset) and the code outside the block.
namespace toc_slot {
inline constexpr std::int32_t people = -0x7be0, assert_flag = -0x7bac, message_buffer = -0x7ba8, today = -0x7b80,
                              strings = -0x5608, comparator = -0x560c;
}
namespace callee {
// RULING_BODY.CPP (body object, person, ...)
inline constexpr std::uint32_t find_entry = 0x105bc2b0, add_entry = 0x105bc4f0, drop_entry = 0x105bc5c0,
                               touch_entry = 0x105bd4f0;
// The runtime library: bsearch(key, base, count, size, cmp) and qsort(base, count, size, cmp), malloc, realloc.
inline constexpr std::uint32_t bsearch = 0x10775600, qsort = 0x10768300, allocate = 0x10775130, reallocate = 0x107751b0;
inline constexpr std::uint32_t add_days = 0x10717890;  // (out date, date, days)
// The list object (0x10738c00): construct, release(list, -1), append(list, item, size), next(list).
inline constexpr std::uint32_t list_construct = 0x10738c00, list_release = 0x10738c20, list_append = 0x10738d10,
                               list_next = 0x10738e00;
inline constexpr std::uint32_t record_history = 0x100e7160;  // table block: (manager, person, ban record)
inline constexpr std::uint32_t compare_entries = 0x100e9290;
}
// The ruling body object in slot `body` (a signed byte, as `extsb` then `rlwinm 2`) of the manager.
inline std::uint32_t ruling_body(const guest::Cpu& c, std::uint32_t manager, std::int8_t body) {
    return c.u32(c.u32(manager + manager::bodies) + (std::uint32_t(std::int32_t(body)) << 2));
}
// The ban record `index` (a signed halfword) of the manager's array, read now.
inline std::uint32_t ban_record(const guest::Cpu& c, std::uint32_t manager, std::int16_t index) {
    return c.u32(manager + manager_arrays::bans) + std::uint32_t(std::int32_t(index)) * ban::size;
}
// Copies a ban record field by field in the original's order (bytes 0x09-0x0b and 0x11 stay as they were).
inline void copy_ban(guest::Cpu& c, std::uint32_t from, std::uint32_t to) {
    c.put8(to, c.u8(from));
    c.put8(to + 1, c.u8(from + 1));
    c.put16(to + 2, c.u16(from + 2));
    c.put16(to + 6, c.u16(from + 6));
    c.put16(to + 4, c.u16(from + 4));
    c.put8(to + 8, c.u8(from + 8));
    c.put16(to + 0xc, c.u16(from + 0xc));
    c.put16(to + 0xe, c.u16(from + 0xe));
    c.put8(to + 0x10, c.u8(from + 0x10));
    c.put16(to + 0x12, c.u16(from + 0x12));
    c.put32(to + 0x14, c.u32(from + 0x14));
}
}
