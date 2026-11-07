#pragma once
// DATABASE.CPP's resolve block: the record tables the loader links together and the stored indexes it turns into
// pointers (0x100af8a0). Table meanings other than clubs and people are not established, so they are named by record
// size ("inferred": from the strides and counts the routine pairs up).
#include <array>
#include <cstdint>

namespace cm::aot::database::resolve {
// A global table: the TOC word at base_slot holds the address of a word that holds the first record, the TOC word at
// count_slot the address of the record count. The record tables are 32-bit-indexed from 0.
struct Table {
    std::int32_t base_slot, count_slot;
    std::uint32_t stride;
};
inline constexpr Table clubs{-0x7be8, -0x7b50, 0x248}, people{-0x7be0, -0x7be4, 0x70},
    table_128{-0x7b98, -0x7b04, 0x128}, table_6c{-0x7bbc, -0x7b64, 0x6c}, table_50{-0x7548, -0x7568, 0x50},
    table_3c{-0x7544, -0x7564, 0x3c}, table_38{-0x7538, -0x7558, 0x38}, table_c8{-0x7520, -0x7524, 0xc8},
    table_48{-0x7b38, -0x74d4, 0x48}, table_34{-0x74bc, -0x74c0, 0x34}, table_44{-0x79f8, -0x79f4, 0x44},
    table_2c{-0x7ab4, -0x7528, 0x2c}, table_68{-0x753c, -0x755c, 0x68}, table_14{-0x74c4, -0x74c8, 0x14},
    table_1c{-0x7a34, -0x7a38, 0x1c},
    // Targets only: their counts are never read here.
    table_2c_a{-0x752c, 0, 0x2c}, table_2c_b{-0x7530, 0, 0x2c}, table_2c_c{-0x7534, 0, 0x2c};

// How far a stored index may go before it counts as invalid: not checked (only negative values are cleared), at most
// the table's count ("inclusive": the original accepts index == count), or below it.
enum class Bound { unbounded, inclusive, exclusive };
// What an index past its bound reports before it is cleared: nothing, a line through the shared formatted-output
// routine (the value alone, or the record number and the value), or the same plus the game's assertion macro.
enum class Report { silent, text, text_indexed, assertion };

// One stored index of a record: the field, the table it points into, and the original's handling of invalid values.
struct Link {
    std::uint32_t field;
    const Table& target;
    Bound bound = Bound::unbounded;
    Report report = Report::silent;
    // The original tests the bound before the sign (only the single club fields at 0x80 and 0x94 do).
    bool bound_first = false;
    // After the bound test failed silently r5 holds the row table's first record (it was loaded before the compare).
    bool leaves_row_base = false;
    // The field the original zeroes for an index past its bound; two sites zero the wrong one (inferred: a typo).
    std::uint32_t cleared = 0;
    // Call site of the formatted-output call and the format's offset in the string pool (or, for an assertion, its
    // number in assertion_sites, which holds both).
    std::uint32_t call_at = 0, format = 0;
    unsigned assertion = 0;
};
constexpr Link plain(std::uint32_t field, const Table& target) { return {field, target}; }
constexpr Link silent(std::uint32_t field, const Table& target, Bound bound, bool leaves_row_base = false,
                      std::uint32_t cleared = 0) {
    return {.field = field, .target = target, .bound = bound, .leaves_row_base = leaves_row_base, .cleared = cleared};
}
constexpr Link texted(std::uint32_t field, const Table& target, Bound bound, std::uint32_t call_at,
                      std::uint32_t format, std::uint32_t cleared = 0) {
    return {.field = field, .target = target, .bound = bound, .report = Report::text, .cleared = cleared,
            .call_at = call_at, .format = format};
}
constexpr Link listed(std::uint32_t field, const Table& target, Bound bound, std::uint32_t call_at,
                      std::uint32_t format, bool bound_first) {
    return {.field = field, .target = target, .bound = bound, .report = Report::text_indexed,
            .bound_first = bound_first, .call_at = call_at, .format = format};
}
constexpr Link asserted(std::uint32_t field, const Table& target, unsigned assertion) {
    return {.field = field, .target = target, .bound = Bound::exclusive, .report = Report::assertion,
            .assertion = assertion};
}

// The three assertions of the person loop: the call of the line's formatted output, the first call of the macro
// (0x10739890), its line number and its two stack buffers.
struct AssertionSite {
    std::uint32_t formatted_call, format, at, line, name, extension;
};
inline constexpr std::array assertion_sites{AssertionSite{0x100b20c8, 0x7b3, 0x100b20e0, 0x18d8, 0x90, 0x48},
                                            AssertionSite{0x100b21d4, 0x7d9, 0x100b21ec, 0x18e9, 0x70, 0x40},
                                            AssertionSite{0x100b22dc, 0x804, 0x100b22f4, 0x18fa, 0x50, 0x38}};
// Offsets in the string pool (TOC -0x5618): the assertion's file name, format and title.
constexpr std::uint32_t assertion_format = 0xd, assertion_title = 0x1a;

// An inline array of person indexes in a club record, `count` entries from `first`, reported by `call_at`.
struct PersonArray {
    std::uint32_t first;
    unsigned count;
    std::uint32_t call_at, format;
};
// The club's squad is checked differently (see resolve_squad); the other arrays follow it.
inline constexpr PersonArray squad{0x98, 50, 0x100b05ac, 0x65e};
inline constexpr std::array person_arrays{PersonArray{0x174, 7, 0x100b06a8, 0x686}, PersonArray{0x160, 5, 0x100b07a4, 0x6a7},
                                          PersonArray{0x190, 3, 0x100b08a0, 0x6c8}, PersonArray{0x84, 3, 0x100b099c, 0x6ea}};
inline constexpr PersonArray last_array{0x19c, 20, 0x100b0ca8, 0x782};
// The squad's leniency: indexes from the count up to count + this are cleared without a report (when the argument is 0).
constexpr std::int32_t squad_slack = 0x52d;

// ---- the stored indexes, record table by record table, in the original's order ----
inline constexpr std::array club_links{
    texted(0x2c, table_128, Bound::inclusive, 0x100af964, 0x5bb),
    silent(0x30, table_6c, Bound::inclusive, true),
    texted(0x34, table_6c, Bound::inclusive, 0x100afa90, 0x5f2, 0x38),
    silent(0x38, table_6c, Bound::inclusive, true),
    texted(0x3c, table_50, Bound::inclusive, 0x100afbd4, 0x62b),
    silent(0x40, table_50, Bound::inclusive, true),
    silent(0x44, table_3c, Bound::inclusive),
    silent(0x4c, table_3c, Bound::inclusive),
    silent(0x54, table_3c, Bound::inclusive),
    silent(0x48, table_3c, Bound::inclusive),
    silent(0x50, table_3c, Bound::inclusive),
    silent(0x58, table_3c, Bound::inclusive),
    silent(0x5c, people, Bound::inclusive),
    silent(0x60, people, Bound::inclusive),
    silent(0x64, people, Bound::inclusive),
    silent(0x68, people, Bound::inclusive),
    silent(0x6c, people, Bound::inclusive),
    silent(0x70, people, Bound::inclusive),
    silent(0x74, clubs, Bound::inclusive, true),
    silent(0x78, clubs, Bound::inclusive, true),
    silent(0x7c, clubs, Bound::inclusive, true),
};
// After the arrays: single person fields of the club, then the last array.
inline constexpr std::array club_person_links{
    listed(0x90, people, Bound::exclusive, 0x100b0a98, 0x70e, false),
    listed(0x80, people, Bound::exclusive, 0x100b0b30, 0x731, true),
    listed(0x94, people, Bound::exclusive, 0x100b0be0, 0x755, true),
};
inline constexpr std::array table_128_links{
    plain(0x74, table_c8), plain(0x78, table_38), plain(0x7c, table_50), plain(0x80, table_3c),
    plain(0x88, table_3c), plain(0x90, table_3c), plain(0x84, table_3c), plain(0x8c, table_3c),
    plain(0x94, table_3c), plain(0x98, table_128), plain(0x9c, table_128), plain(0xa0, table_128),
};
inline constexpr std::array table_2c_links{
    plain(0x4, table_2c_a), plain(0x8, table_2c_b), plain(0xc, table_128), plain(0x10, table_38),
};
inline constexpr std::array table_50_links{
    silent(0x10, table_38, Bound::inclusive, true),
    silent(0x14, table_50, Bound::inclusive, true, 0x10),
};
inline constexpr std::array table_34_links{
    plain(0x4, clubs), plain(0x8, clubs), plain(0xc, clubs), plain(0x10, clubs), plain(0x14, clubs),
    plain(0x18, clubs), plain(0x1c, people), plain(0x20, people), plain(0x24, people), plain(0x28, people),
    plain(0x2c, people), plain(0x30, people),
};
inline constexpr std::array table_44_links{
    silent(0x4, people, Bound::exclusive), silent(0x8, people, Bound::exclusive),
    silent(0xc, people, Bound::exclusive), silent(0x10, people, Bound::exclusive),
    silent(0x14, people, Bound::exclusive), silent(0x18, people, Bound::exclusive),
    silent(0x1c, people, Bound::exclusive), silent(0x20, people, Bound::exclusive),
};
inline constexpr std::array person_links{
    plain(0x28, table_2c_a), plain(0x2c, table_2c_b), plain(0x30, table_2c_c), plain(0xc, table_128),
    plain(0x24, clubs),      plain(0x10, table_128),  plain(0x20, clubs),
    asserted(0x14, table_48, 0), asserted(0x18, table_34, 1), asserted(0x1c, table_44, 2),
};
inline constexpr std::array table_68_links{
    silent(0x4, table_c8, Bound::exclusive, true),
    silent(0x8, table_128, Bound::exclusive, true),
    silent(0xc, table_3c, Bound::exclusive),
    silent(0x10, table_3c, Bound::exclusive),
};
inline constexpr std::array table_14_links{plain(0x4, people), plain(0x8, clubs)};
inline constexpr std::array table_38_links{plain(0x18, table_128)};
inline constexpr std::array table_1c_links{
    plain(0x4, table_6c), plain(0x8, clubs), plain(0xc, clubs),
    silent(0x10, clubs, Bound::inclusive, true),
    silent(0x14, clubs, Bound::inclusive, true),
};
inline constexpr std::array table_6c_links{
    plain(0x8, table_128), plain(0x4, table_c8), plain(0xc, table_3c), plain(0x10, table_3c),
};
}
