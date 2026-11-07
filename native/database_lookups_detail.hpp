#pragma once
// DATABASE.CPP lookups block: record offsets and TOC slots as the routines read
// them. Meanings marked "inferred" come from how the routines and their callers
// use the values; none is established beyond that.
#include "database_detail.hpp"
#include "guest_values.hpp"
#include <array>
#include <cstdint>

namespace cm::aot::database::lookups {
using guest::Cpu;

// A person-like record (the first argument of most queries here).
namespace holder {
constexpr std::uint32_t id = 0x0;                // the word the long id chain 0x100ca800 compares
constexpr std::uint32_t first_club = 0xc;        // inferred: a club the holder belongs to
constexpr std::uint32_t second_club = 0x10;      // inferred: a second club, may be null
constexpr std::uint32_t preferences = 0x18;
constexpr std::uint32_t score = 0x1c;            // record of 16-bit values at +0x28/+0x2a/+0x2c
constexpr std::uint32_t current_club = 0x24;     // inferred
constexpr std::uint32_t linked = 0x2c;           // a record with a nation word at +0x68 and a status byte at +0x11d
constexpr std::uint32_t role = 0x62;             // byte, 0xf tested by 0x100d3040
constexpr std::uint32_t pair_field = 0x6a;       // two 2-bit fields (0x100ce200)
constexpr std::uint32_t squad = 0x98;            // 50 person pointers (0x100db120)
}
// A club-like record: the nation word and the linked records.
namespace club_record {
constexpr std::uint32_t id = 0x0;
constexpr std::uint32_t nation = 0x68;           // compared with the words TOC slots point at
constexpr std::uint32_t nation_record = 0x74;    // inferred: the nation's record, whose +8 holds the nation word
constexpr std::uint32_t status = 0x11d;          // byte, 2 tested
constexpr std::uint32_t rating = 0x1ec;          // 16-bit value the club sorters compare
}
constexpr std::uint32_t nation_word = 0x8;       // inside the record at club_record::nation_record
namespace preferences_record {
constexpr std::uint32_t favourite_clubs = 0x4, disliked_clubs = 0x10, favourite_staff = 0x1c,
                        disliked_staff = 0x28;
constexpr std::uint32_t staff_key = 0x24;        // of a listed staff record: the word 0x100d9f10 counts matches of
}

// The 0x64-byte rows indexed by a club's first word (TOC -0x7b9c is the address of the
// pointer to the rows, -0x7be4 of the row count) and the 0x70-byte rows the links point at
// (TOC -0x7be0). A row holds two links of (record index, key) at +0x3c/+0x4c and +0x50/+0x60.
namespace link_rows {
constexpr std::int32_t rows_slot = -0x7b9c, count_slot = -0x7be4, records_slot = -0x7be0;
constexpr std::uint32_t row_size = 0x64, record_size = 0x70;
constexpr std::uint32_t first_index = 0x3c, first_key = 0x4c, second_index = 0x50, second_key = 0x60;
constexpr std::uint32_t link_stride = 0x14;
}
constexpr std::int32_t manager_slot = -0x7b7c, last_primary_slot = -0x7b78;

// Loads into a register as `lwz`, `lbz`, `lha`, `lhz` do.
inline std::uint32_t load32(Cpu& c, unsigned r, std::uint32_t at) { auto v = c.u32(at); c.set(r, v); return v; }
inline std::uint32_t load8(Cpu& c, unsigned r, std::uint32_t at) { std::uint32_t v = c.u8(at); c.set(r, v); return v; }
inline std::uint32_t load16(Cpu& c, unsigned r, std::uint32_t at) { std::uint32_t v = c.u16(at); c.set(r, v); return v; }
inline std::int32_t load16s(Cpu& c, unsigned r, std::uint32_t at) {
    auto v = std::int32_t(c.i16(at));
    c.set(r, std::uint32_t(v));
    return v;
}
inline std::uint32_t sext8(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int8_t(v))); }
inline std::uint32_t sext16(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
// `lwz rS,slot(r2)`: the address of a global word.
inline std::uint32_t slot(Cpu& c, unsigned r, std::int32_t offset) { return load32(c, r, c.r(2) + std::uint32_t(offset)); }
}
