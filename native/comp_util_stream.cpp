// COMP_UTIL.CPP, block `record_stream`: a fixture reference (the game's 0x50-byte match row) to and from a news
// item (NEWS.CPP's 0xe4-byte item with fifty payload words). 0x10463be0 stores eighteen of the row's fields as
// consecutive words of the item, from the index the byte at `cursor` names, and advances the byte by eighteen;
// 0x10463dc0 reads them back through the same order and then rebuilds the four pointers (+0x14, +0x18, +0x1c,
// +0x20) from the ids it read. The item routines (0x102cb4b0 set word, 0x102cb570 word) take the index as a
// signed byte and report the game's assertion for one outside 0..49: the cursor byte near the end of the payload
// (33 and above) or past 127 makes later words fail one by one, a value of 256 or more wraps back to the start.
// The writer leaves each word failed that way unwritten; the reader stores 0 for it.
//
// Neither routine is read for its registers after the call (tools/register_contract.py --after): only the memory
// they write is observed.
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
namespace f = fixture_reference;
// The eighteen words, in the order they sit in the item: the field's offset, its width in bytes and whether the
// original loads a sign-extended value (bytes: always `lbz`+`extsb`; halfwords: `lha`) or a zero-extended one (`lhz`).
struct Field { std::uint32_t offset; unsigned width; bool sign; };
constexpr Field fields[f::fields] = {
    {f::first_club_id, 4, false}, {f::second_club_id, 4, false}, {f::byte_3d, 1, true}, {f::byte_3e, 1, true},
    {f::row_id, 4, false},        {f::competition_id, 4, false}, {f::byte_42, 1, true},  {f::half_32, 2, true},
    {f::half_30, 2, true},        {f::byte_43, 1, true},         {f::byte_44, 1, true},  {f::byte_4b, 1, true},
    {f::byte_4c, 1, true},        {f::byte_47, 1, true},         {f::byte_48, 1, true},  {f::half_2a, 2, false},
    {f::half_28, 2, false},       {f::word_08, 4, false},
};
// The `bl` of each item call: the return addresses the callee's prologue saves.
constexpr std::uint32_t write_back[f::fields] = {0x10463c20, 0x10463c34, 0x10463c4c, 0x10463c64, 0x10463c78, 0x10463c8c,
                                                 0x10463ca4, 0x10463cb8, 0x10463ccc, 0x10463ce4, 0x10463cfc, 0x10463d14,
                                                 0x10463d2c, 0x10463d44, 0x10463d5c, 0x10463d70, 0x10463d84, 0x10463d98};
constexpr std::uint32_t read_back[f::fields] = {0x10463df0, 0x10463e04, 0x10463e18, 0x10463e2c, 0x10463e40, 0x10463e54,
                                                0x10463e68, 0x10463e7c, 0x10463e90, 0x10463ea4, 0x10463eb8, 0x10463ecc,
                                                0x10463ee0, 0x10463ef4, 0x10463f08, 0x10463f1c, 0x10463f30, 0x10463f44};

std::uint32_t load(Cpu& c, std::uint32_t reference, const Field& field) {
    const auto at = reference + field.offset;
    switch (field.width) {
    case 1: return std::uint32_t(c.i8(at));  // every byte field is signed (`lbz` + `extsb`)
    case 2: return field.sign ? std::uint32_t(std::int32_t(c.i16(at))) : c.u16(at);
    default: return c.u32(at);
    }
}
void store(Cpu& c, std::uint32_t reference, const Field& field, std::uint32_t value) {
    const auto at = reference + field.offset;
    switch (field.width) {
    case 1: c.put8(at, value); break;
    case 2: c.put16(at, value); break;
    default: c.put32(at, value);
    }
}
// `id` of a table of `size`-byte rows: the row's address, 0 for the id -1.
std::uint32_t row_of(std::uint32_t table, std::uint32_t id, std::uint32_t size) { return id == 0xffffffffu ? 0 : table + id * size; }
}

void write_fixture_reference(Cpu& c, std::uint32_t reference, std::uint32_t item, std::uint32_t cursor) {
    Frame frame(c, 0x10463be0, 0x50, {.first_gpr = 28});
    const std::uint32_t first = c.u8(cursor);
    // r28 the reference, r29 the item, r30 the cursor's address, r31 the next index (the item routine reports an
    // assertion, which reaches imports where the replays compare the non-volatile registers).
    c.set({{28, reference}, {29, item}, {30, cursor}});
    for (unsigned n = 0; n < f::fields; ++n) {
        c.set(31, first + n + 1);
        const auto value = load(c, reference, fields[n]);
        invoke(c, callee::set_item_word, write_back[n], {item, first + n, value});
    }
    c.put8(cursor, first + f::fields);
}

void read_fixture_reference(Cpu& c, std::uint32_t item, std::uint32_t reference, std::uint32_t cursor) {
    Frame frame(c, 0x10463dc0, 0x50, {.first_gpr = 27});
    const auto clubs = c.toc(toc::clubs);  // the club table's cell, read once at entry
    const std::uint32_t first = c.u8(cursor);
    c.set({{27, item}, {28, cursor}, {30, clubs}, {31, reference}});
    for (unsigned n = 0; n < f::fields; ++n) {
        c.set(29, first + n + 1);
        const auto value = invoke(c, callee::item_word, read_back[n], {item, first + n});
        store(c, reference, fields[n], value);
    }
    c.put8(cursor, first + f::fields);
    // The pointers follow from the ids just stored (the original re-reads them from the record, and the tables'
    // cells each time: the row, club and record tables are other modules' memory).
    c.put32(reference + f::first_club, row_of(c.u32(clubs), c.u32(reference + f::first_club_id), comp::club::size));
    c.put32(reference + f::second_club, row_of(c.u32(clubs), c.u32(reference + f::second_club_id), comp::club::size));
    c.put32(reference + f::row, row_of(c.u32(c.toc(f::row_table)), c.u32(reference + f::row_id), f::row_size));
    c.put32(reference + f::competition_record, row_of(c.u32(c.toc(toc::records)), c.u32(reference + f::competition_id), comp::record::size));
}
}
