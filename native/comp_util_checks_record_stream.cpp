// COMP_UTIL.CPP, block `record_stream`: cases for 0x10463be0 (a fixture reference written into a news item as
// eighteen words) and 0x10463dc0 (read back, the four pointers rebuilt).
//
// The reference is a copy of a real fixture row (the game's 0x50-byte match rows) staged in the scratch area
// (this block's area is scratch_base + 0x90000..0x92000), with the fields the routines move overwritten; the item
// is a staged 0xe4-byte news item whose fifty payload words hold a pattern, so a word that is not written (a failed
// index) shows. The cursor byte takes every value from 0 to 255: from 33 on the later words (index 50 and
// above) fail one by one in the item routines' assertion (answered by the checker's scripted dialog), from 128
// the index is negative, and a cursor near 256 wraps the byte. The reader's items carry ids that are -1, valid
// and far out of range, so each pointer rebuild takes both branches; its payload words are also 0 for an index the
// item's word routine rejects (it stores 0). The assertion's own text is NEWS.CPP's and is checked there.
// Labels start with "record_stream:" (CM_COMP_UTIL_BLOCK=record_stream).
#include "comp_util_checks.hpp"
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t write_reference = 0x10463be0, read_reference = 0x10463dc0;
constexpr std::uint32_t area = scratch_base + 0x90000;
constexpr std::uint32_t item_at = area, reference_at = area + 0x200, cursor_at = area + 0x300;
constexpr std::uint32_t payload = 8, payload_words = 50, item_size = 0xe4, reference_size = 0x50;
constexpr std::int32_t rows_slot = -0x7548;

Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
void append(std::vector<Write>& to, const std::vector<Write>& more) { to.insert(to.end(), more.begin(), more.end()); }

// The staged item: a header pattern and payload words 0x0a0a0000 + n (a word left alone keeps its value).
std::vector<Write> staged_item() {
    std::vector<Write> memory;
    for (std::uint32_t at = 0; at < item_size; at += 4) memory.push_back(w32(item_at + at, 0x5a5a0000u + at));
    for (std::uint32_t word = 0; word < payload_words; ++word) memory.push_back(w32(item_at + payload + word * 4, 0x0a0a0000u + word));
    return memory;
}

// The fields the routines move, in the item's order, as values of the row's own widths.
struct Values {
    std::uint32_t first_club_id, second_club_id, competition_id, row_id, word_08;
    std::uint32_t half_28, half_2a, half_30, half_32;
    std::uint32_t byte_3d, byte_3e, byte_42, byte_43, byte_44, byte_47, byte_48, byte_4b, byte_4c;
    const char* what;
};
std::vector<Write> staged_reference(const Scenario& scenario, const Values& v, unsigned real_row) {
    std::vector<Write> memory;
    // The rest of the row (the bytes that do not travel) from a real row, or a pattern when the table is empty.
    const auto table = scenario.global(rows_slot);
    for (std::uint32_t at = 0; at < reference_size; at += 4)
        memory.push_back(w32(reference_at + at, table ? scenario.r32(table + real_row * reference_size + at) : 0x33445566u + at));
    append(memory, {w32(reference_at + 0x00, v.competition_id), w32(reference_at + 0x04, v.row_id), w32(reference_at + 0x08, v.word_08),
                    w32(reference_at + 0x0c, v.first_club_id), w32(reference_at + 0x10, v.second_club_id),
                    w16(reference_at + 0x28, v.half_28), w16(reference_at + 0x2a, v.half_2a), w16(reference_at + 0x30, v.half_30),
                    w16(reference_at + 0x32, v.half_32), w8(reference_at + 0x3d, v.byte_3d), w8(reference_at + 0x3e, v.byte_3e),
                    w8(reference_at + 0x42, v.byte_42), w8(reference_at + 0x43, v.byte_43), w8(reference_at + 0x44, v.byte_44),
                    w8(reference_at + 0x47, v.byte_47), w8(reference_at + 0x48, v.byte_48), w8(reference_at + 0x4b, v.byte_4b),
                    w8(reference_at + 0x4c, v.byte_4c)});
    return memory;
}

// Field values that tell the widths and signs apart: negative bytes and halfwords (extsb / lha), halfwords with the
// top bit set (lhz does not extend), ids of -1 and of 0x7fffffff.
const Values value_sets[] = {
    {5, 7, 0x12, 3, 0x01020304, 0x1234, 0x5678, 0x0abc, 0x0def, 1, 2, 3, 4, 5, 6, 7, 8, 9, "small values"},
    {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, "all ones"},
    {0x7fffffff, 0x80000000, 0x7ffffffe, 0x80000001, 0x80000000, 0x8000, 0x7fff, 0x8000, 0x7fff, 0x80, 0x7f, 0x80, 0x7f, 0x80, 0x7f, 0x81, 0x7e, 0x80, "extremes"},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "zeros"},
    {0x30, 0xffffffff, 0x14, 0xffffffff, 0xdeadbeef, 0x4000, 0xc000, 0xff00, 0x00ff, 0x40, 0xc0, 0xfe, 0x01, 0xfd, 0x02, 0xfc, 0x03, 0xfb, "mixed ids"},
};
}

COMP_UTIL_CASES("record_stream") {
    // The writer: every start index of the cursor byte against each value set, a few real rows.
    for (unsigned set = 0; set < std::size(value_sets); ++set)
        for (std::uint32_t cursor = 0; cursor < 256; ++cursor) {
            auto memory = staged_item();
            append(memory, staged_reference(scenario, value_sets[set], set));
            memory.push_back(w8(cursor_at, cursor));
            cases.push_back({.entry = write_reference, .registers = {{3, reference_at}, {4, item_at}, {5, cursor_at}}, .memory = memory,
                             .label = "record_stream: write " + std::string(value_sets[set].what) + " from word " + std::to_string(cursor)});
        }
}

COMP_UTIL_CASES("record_stream") {
    // The reader: the item's words are the value set's words in the item's order; every cursor value, with the
    // item's other words intact. The reference starts as a real row with other pointers and bytes, which the
    // reader overwrites (the fields it reads) or keeps (the rest).
    for (unsigned set = 0; set < std::size(value_sets); ++set) {
        const auto& v = value_sets[set];
        const std::uint32_t words[] = {v.first_club_id, v.second_club_id, v.byte_3d, v.byte_3e, v.row_id, v.competition_id, v.byte_42,
                                       v.half_32, v.half_30, v.byte_43, v.byte_44, v.byte_4b, v.byte_4c, v.byte_47, v.byte_48, v.half_2a,
                                       v.half_28, v.word_08};
        for (std::uint32_t cursor = 0; cursor < 256; ++cursor) {
            auto memory = staged_item();
            // The eighteen words sit from the cursor (as a signed byte index), wherever the item has room for them.
            for (std::uint32_t n = 0; n < 18; ++n) {
                const auto index = std::int8_t(cursor + n);
                if (index >= 0 && index < std::int32_t(payload_words)) memory.push_back(w32(item_at + payload + std::uint32_t(index) * 4, words[n]));
            }
            append(memory, staged_reference(scenario, value_sets[(set + 1) % std::size(value_sets)], 1));
            memory.push_back(w8(cursor_at, cursor));
            cases.push_back({.entry = read_reference, .registers = {{3, item_at}, {4, reference_at}, {5, cursor_at}}, .memory = memory,
                             .label = "record_stream: read " + std::string(v.what) + " from word " + std::to_string(cursor)});
        }
    }
}
}
