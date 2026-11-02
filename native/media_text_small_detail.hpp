#pragma once
// What the short press texts share: the story item's value getter, the text
// buffer calls and the strings' pool offsets.
#include "media_detail.hpp"
#include "media_values_detail.hpp"

namespace cm::aot::media::detail::small {
namespace library {
constexpr std::uint32_t item_value = 0x102cb570, copy_text = 0x1074b830, link_text = 0x1074bab0, person_name = 0x100cb590,
    person_known = 0x100cbde0, begin_item = 0x102bdf90, fetch_item = 0x102ccdd0, world_value = 0x1073ef70, set_field = 0x1073f1e0;
}
// The 0x70-byte person rows (-0x7be0) the texts name, apart from the full person records.
constexpr int rows_slot = -0x7be0;
constexpr std::uint32_t row_size = 0x70;
// `extsb`: a byte as the original holds it in a register.
inline std::uint32_t sign8(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int8_t(value))); }
inline std::uint32_t sign16(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int16_t(value))); }
// Value `index` of the story item (`bl 0x102cb570`).
inline std::uint32_t item_value(Cpu& c, std::uint32_t item, std::uint32_t index, std::uint32_t back) {
    c.set(3, item);
    c.set(4, index);
    c.call(library::item_value, back);
    return c.r(3);
}
// The shared text buffer's tail: appends `text` to the output (0x1074b830).
inline void copy_text(Cpu& c, std::uint32_t output, std::uint32_t text, std::uint32_t back) {
    c.set(3, output);
    c.set(4, text);
    c.call(library::copy_text, back);
}
// Links the span `text` in the output to a record (0x1074bab0): kind 0x7d5 clubs, 0x7d7 persons.
inline void link_text(Cpu& c, std::uint32_t output, std::uint32_t text, std::uint32_t kind, std::uint32_t record,
                      std::uint32_t back) {
    c.set({{3, output}, {4, text}, {5, kind}, {6, record}});
    c.call(library::link_text, back);
}
// The text of a person row's name into `buffer` (0x100cb590); `mode` 1 is the full form, 5 the short.
inline void person_name(Cpu& c, std::uint32_t first, std::uint32_t second, std::uint32_t third, std::uint32_t buffer,
                        std::uint32_t mode, std::uint32_t index, std::uint32_t back, std::uint32_t capacity = 0x190) {
    c.set({{3, first}, {4, second}, {5, third}, {6, buffer}, {7, capacity}, {8, mode}, {9, index}});
    c.call(library::person_name, back);
}
// Whether the row matches the key (0x100cbde0), as the signed byte it returns.
inline std::int32_t person_listed(Cpu& c, std::uint32_t row, std::uint32_t first, std::uint32_t second, std::uint32_t third,
                                  std::uint32_t back) {
    c.set({{3, row}, {4, first}, {5, second}, {6, third}, {7, 0}});
    c.call(library::person_known, back);
    const auto known = std::int8_t(c.r(3));
    compare(c, known);
    return known;
}
inline std::uint32_t person_row(Cpu& c, std::uint32_t index) { return c.u32(c.toc(rows_slot)) + index * row_size; }
inline std::uint32_t club_record(Cpu& c, std::uint32_t index) { return c.u32(c.toc(slot::clubs)) + index * club_size; }
// Value `index` of the working record the dialog and its handler share (`bl 0x1073ef70` on the world record, TOC -0x7bb0).
inline std::uint32_t world_value(Cpu& c, std::uint32_t world, std::uint32_t index, std::uint32_t back) {
    c.set({{3, world}, {4, index}});
    c.call(library::world_value, back);
    return c.r(3);
}
// The world record's windows (0x2c0 bytes each, the current one named by the halfword at +0x2c58).
constexpr std::uint32_t window_size = 0x2c0, window_index_at = 0x2c58;
inline std::uint32_t window_index(Cpu& c, std::uint32_t world) { return std::uint32_t(c.i16(world + window_index_at)); }
inline std::uint32_t window_record(Cpu& c, std::uint32_t world) { return world + window_index(c, world) * window_size; }
// `format` into the story's text buffer (the sentence under construction).
struct Say {
    Cpu& c;
    std::uint32_t text;
    void operator()(unsigned phrase, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) const {
        format(c, text, phrase, back, arguments);
    }
};
// The end of a story about a player: the sentence is appended to the output and, when the person row is the one listed
// under the key, the player's name in it is linked to the row.
struct Finish {
    Cpu& c;
    std::uint32_t output, text, name;
    void operator()(std::uint32_t player, std::uint32_t first, std::uint32_t second, std::uint32_t third, std::uint32_t copied,
                    std::uint32_t known, std::uint32_t linked) const {
        copy_text(c, output, text, copied);
        if (person_listed(c, person_row(c, player), first, second, third, known) != 0)
            link_text(c, output, name, 0x7d7, person_row(c, player), linked);
    }
};
// A club record's fields the texts name.
constexpr std::uint32_t club_name = 0x221, club_article = 0x23b, club_short_name = 0x1ee, club_short_article = 0x242;
}
