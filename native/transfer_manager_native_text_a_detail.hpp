#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_a": what the four transfer news message texts share
// (a news item's numbered fields, the club records, the localized formatter and the output's line calls).
#include "transfer_manager_native_detail.hpp"

namespace cm::aot::transfer_manager_native::text_a {
using guest::Cpu;
using guest::Frame;

// Routines of the other modules these texts call.
inline constexpr std::uint32_t news_field = 0x102cb570,      // (item, index) -> the item's numbered field
    put_news_word = 0x102cd0c0,                              // (news, club slot, key, index, value, item): sets a word of a stored item
    describe_person = 0x100cb590,                            // (day, month, year, buffer, size, mode, person): person text
    person_known = 0x100cbde0,                               // (person, day, month, year, 0) -> whether the text may name him
    format_text = 0x10727890,                                // (buffer, format, values...): the localized formatter
    set_line = 0x1074b830, add_line = 0x1074b960,            // (output, text): starts / extends the message text
    add_link = 0x1074bab0;                                   // (output, name, kind, record): a name that links to a record
inline constexpr std::uint32_t link_club = 0x7d5, link_person = 0x7d7;
inline constexpr std::int32_t buffer_cell = -0x7bb8, text_cell = -0x4b9c;
inline constexpr std::uint32_t club_name_short = 0x1ee, club_name_long = 0x221, club_len_short = 0x242, club_len_long = 0x23b;
inline constexpr std::uint32_t text_page = 0x10000;

// The call sites are named by the address of their `bl`; the original's return address follows it.
inline std::uint32_t after(std::uint32_t site) { return site + 4; }

// The item's numbered field (NEWS.CPP 0x102cb570).
inline std::uint32_t field(Cpu& c, std::uint32_t item, std::uint32_t index, std::uint32_t site) {
    return invoke(c, news_field, after(site), {item, index});
}

// The club records: names and the length bytes the formatter takes beside them (signed bytes).
struct Clubs {
    Cpu& c;
    std::uint32_t cell;  // the address of the pointer to the club table
    std::uint32_t at(std::uint32_t index) const { return c.u32(cell) + index * club_size; }
    std::uint32_t short_len(std::uint32_t index) const { return std::uint32_t(std::int32_t(c.i8(at(index) + club_len_short))); }
    std::uint32_t long_len(std::uint32_t index) const { return std::uint32_t(std::int32_t(c.i8(at(index) + club_len_long))); }
    std::uint32_t short_name(std::uint32_t index) const { return at(index) + club_name_short; }
    std::uint32_t long_name(std::uint32_t index) const { return at(index) + club_name_long; }
};

// The localized formatter: the first six values go to r5-r10, the rest to the caller's parameter area
// (the original's stack words at sp+0x38, +0x3c, ...).
inline void say(Cpu& c, const Frame& frame, std::uint32_t site, std::uint32_t destination, std::uint32_t format,
                std::initializer_list<std::uint32_t> values) {
    unsigned reg = 5, stacked = 0;
    for (const auto value : values) {
        if (reg <= 10) c.set(reg++, value);
        else c.put32(frame.at(0x38 + 4 * stacked++), value);
    }
    c.set(3, destination);
    c.set(4, format);
    c.call(format_text, after(site));
}

// The person text the message names people by (100cb590); `mode` 1 or 5.
inline void describe(Cpu& c, std::uint32_t site, std::uint32_t day, std::uint32_t month, std::uint32_t year,
                     std::uint32_t buffer, std::uint32_t size, std::uint32_t mode, std::uint32_t person) {
    invoke(c, describe_person, after(site), {day, month, year, buffer, size, mode, person});
}
inline bool known(Cpu& c, std::uint32_t site, std::uint32_t person_record, std::uint32_t day, std::uint32_t month,
                  std::uint32_t year) {
    return std::int8_t(invoke(c, person_known, after(site), {person_record, day, month, year, 0})) != 0;
}
inline void begin_text(Cpu& c, std::uint32_t site, std::uint32_t output, std::uint32_t text) {
    invoke(c, set_line, after(site), {output, text});
}
inline void extend_text(Cpu& c, std::uint32_t site, std::uint32_t output, std::uint32_t text) {
    invoke(c, add_line, after(site), {output, text});
}
inline void link(Cpu& c, std::uint32_t site, std::uint32_t output, std::uint32_t name, std::uint32_t kind, std::uint32_t record) {
    invoke(c, add_link, after(site), {output, name, kind, record});
}
}
