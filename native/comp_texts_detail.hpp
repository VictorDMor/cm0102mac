#pragma once
// COMP.CPP, block `news`: what the text routines share. Every call site is named by the address of its `bl`
// (the call's return address is the next instruction).
#include "comp_detail.hpp"

namespace cm::aot::comp::texts {
// Other modules' routines the texts call.
inline constexpr std::uint32_t text_set = 0x1074b830,    // (text object, text): sets the news text
                               text_link = 0x1074bab0,   // (text object, name, tag, row): appends a link row (0xd0 bytes)
                               item_field = 0x102cb570,  // (item, n): the news item's field n
                               money_a = 0x10009f20,     // (pointer to a word, 6): CASH.CPP, the amount as a 64-bit value (r3:r4)
                               money_b = 0x1000b410,     // (buffer, size, high, low, 6): CASH.CPP, the amount's text
                               ordinal = 0x107467a0,     // (n, -1): the ordinal text of n
                               stage_group = 0x10464010,
                               stage_text = 0x10476f40,
                               stage_name = 0x104769d0,
                               stage_gender = 0x10476ec0,
                               club_text = 0x10192350,   // (n, club row, buffer, size)
                               fate_by_flag = 0x10469710, fate_by_stage = 0x10464d00,
                               fixture_rules_query = 0x10494d20, round_row = 0x1049ea00;
// The tags of a link row: a club's name, a competition's name.
inline constexpr std::uint32_t club_tag = 0x7d5, competition_tag = 0x7d0;

inline std::uint32_t format(Cpu& c, std::uint32_t at, std::initializer_list<std::uint32_t> arguments) {
    return invoke(c, callee::format, at + 4, arguments);
}
inline void set_text(Cpu& c, std::uint32_t at, std::uint32_t output, std::uint32_t text) { invoke(c, text_set, at + 4, {output, text}); }
inline void link(Cpu& c, std::uint32_t at, std::uint32_t output, std::uint32_t name, std::uint32_t tag, std::uint32_t row) {
    invoke(c, text_link, at + 4, {output, name, tag, row});
}
inline std::uint32_t field(Cpu& c, std::uint32_t at, std::uint32_t item, std::uint32_t index) {
    return invoke(c, item_field, at + 4, {item, index});
}
}
