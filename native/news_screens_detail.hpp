#pragma once
#include "news_screens.hpp"
#include "guest_values.hpp"
#include <initializer_list>

namespace cm::aot::news_screens::detail {
using namespace guest;

namespace slot {
constexpr int screens = -0x7bb0, news = -0x7bf0, text = -0x7bb8, pool = -0x50f0, today = -0x7b80,
    people = -0x7be0, person_count = -0x7be4, preferences = -0x7ac8, assert_buffer = -0x7ba8,
    label_destroy = -0x7a5c, label_construct = -0x7a58,
    draw_vector = -0x50e4, command_vector = -0x50e8, close_vector = -0x50ec,
    message_draw_vector = -0x50f8, message_command_vector = -0x50fc;
// Colours and fonts (a halfword behind each slot).
constexpr int ink = -0x7bc8, button_font = -0x7bc4, table_font = -0x7bdc, filter_font = -0x7bd8,
    headline_font = -0x7bd0, item_font = -0x7a80, read_ink = -0x7b10;
}
// NEWS_SCREENS.CPP's strings, at the pool.
namespace phrase {
constexpr std::uint32_t assert_format = 0x11, assert_title = 0x1e, filter_placeholder = 0x24, empty = 0x33,
    title_vowel = 0x34, title_consonant = 0x3f, title = 0x4b, all = 0x76, messages = 0x9e, competitions = 0xa7,
    injuries = 0xb4, contracts = 0xc6, transfers = 0xda, jobs = 0xe4, records = 0xe9, headline = 0xf1,
    filter = 0xf7, next_unread = 0x100, send_to = 0x10c, send_to_all = 0x130, cancel = 0x144, send = 0x14b;
}
// Routines of other modules.
inline constexpr std::uint32_t person_colours = 0x100c7a50, person_name = 0x100caeb0, set_panel = 0x102a1070,
    default_action = 0x102a54b0, common_action = 0x102a83a0, construct_item = 0x102bdf90, render_message = 0x102be040,
    action_label = 0x102c6b10, set_item_word = 0x102cb4b0, item_word = 0x102cb570, set_item_text = 0x102cb630,
    publish_to_manager = 0x102cbc10, recent_items = 0x102ccb20, find_item = 0x102ccdd0, unread_count = 0x102cd440,
    acknowledge = 0x102cd5f0, requires_attention = 0x102cd8b0, text_contains = 0x10317100,
    screen_background = 0x10355cf0, screen_colour = 0x10355d30, begin_screen = 0x10712d00, begin_plain_screen = 0x10712e60,
    set_person_screen = 0x107136d0, select_row = 0x107145f0, long_date = 0x10715e00, short_date = 0x10716b00,
    add_table_field = 0x10719740, add_table = 0x107198c0, current_language = 0x10727880, localized_text = 0x10727890,
    broadcast_history = 0x1073e350, push_history = 0x1073e4b0, history_at_end = 0x1073ea20, history_at_start = 0x1073eb00,
    history_value = 0x1073ef70, broadcast_history_value = 0x1073f0a0, set_history_value = 0x1073f1e0, screen_flag = 0x1073fdf0,
    row_for_item = 0x10743510, set_screen_colours = 0x10744250, set_screen_title = 0x107442f0, add_box = 0x10744390,
    add_labelled_box = 0x10744430, add_button = 0x107444e0, add_cell = 0x107445a0, add_tabs = 0x1074aa90,
    set_history_buttons = 0x1074b0a0, init_text = 0x1074b6e0, destroy_text = 0x1074b7c0, set_text = 0x1074b830,
    link_person = 0x1074bab0, draw_text = 0x1074bcd0, clear_text = 0x1074c570, line_scale = 0x1076d470,
    release_block = 0x10770d20, construct_array = 0x10773320, destroy_array = 0x10773420, allocate = 0x107735f0,
    release = 0x10773610, release_list = 0x10775170, format_text = 0x1077b720, string_length = 0x1077dbd0,
    find_text = 0x1077dda0, pointer_glue = 0x10781320;
inline constexpr std::uint32_t screens_high = 0x260000, none = 0xffffffff;
// The news record (NEWS.CPP's 0xe4 bytes) as these screens read it.
struct NewsItem {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t priority() const { return c.u8(address + 4); }
    std::int16_t day() const { return c.i16(address + 0xd0); }
    std::uint32_t year() const { return c.u16(address + 0xd2); }
    std::uint32_t date_flag() const { return c.u8(address + 0xd4); }
    std::uint32_t weekday() const { return c.u8(address + 0xd8); }
    std::uint32_t id() const { return c.u32(address + 0xdc); }
    std::uint32_t read() const { return c.u8(address + 0xe0); }
};
// What a screen's command returns to the screen manager.
inline constexpr std::int32_t redraw = -0xb, closed = -9, popup = -4, leave_screen = -5;

// The title's "d'" before a vowel: the jump table at TOC -0x50f4 goes to the first format for
// these letters. The letter is the name's first byte, signed.
constexpr bool starts_with_vowel(std::int8_t letter) {
    switch (letter) {
        case 'A': case 'E': case 'I': case 'O': case 'U': case 'Y':
        case 'a': case 'e': case 'i': case 'o': case 'u': case 'y': return true;
        default: return false;
    }
}
// The original's `bl`: the argument registers from r3 on, the return address; the result in r3.
inline std::uint32_t bl(Cpu& c, std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto value : arguments) c.set(n++, value);
    c.call(target, back);
    return c.r(3);
}
// After the common action: `neg r0,r3; or r0,r0,r3; srawi r0,r0,31; and r3,r3,r0` leaves r3 as it is
// and the shift's carry.
inline std::uint32_t common_action_result(Cpu& c) {
    const auto value = c.r(3);
    c.s.wr<1>(reg::xer_ca, value != 0 && value != 0x80000000);
    return value;
}
// The original's test of a boolean returned in a byte (`extsb.`).
inline bool byte_flag(Cpu& c, std::uint32_t value) { return compare(c, std::int8_t(value)) != 0; }
// The world's halfwords behind a TOC slot.
inline std::uint32_t halfword(Cpu& c, int slot_offset) { return c.u16(c.toc(slot_offset)); }
// The screen manager's per-screen values, through the manager object.
struct History {
    Cpu& c;
    std::uint32_t screens;
    std::uint32_t value(std::uint32_t index, std::uint32_t back) const { return bl(c, history_value, back, {screens, index}); }
    void set(std::uint32_t index, std::uint32_t value, std::uint32_t back) const {
        bl(c, set_history_value, back, {screens, index, value, 0});
    }
};
// The current screen's row in the manager and what it holds.
inline std::uint32_t current_row(Cpu& c, std::uint32_t screens) {
    return std::uint32_t(std::int32_t(c.i16(screens + 0x2c58))) * 0x2c0;
}
inline std::uint32_t screen_manager_person(Cpu& c, std::uint32_t screens) {
    c.set(5, screens + current_row(c, screens));
    return c.u32(c.r(5) + 4);
}
inline std::uint32_t screen_table(Cpu& c, std::uint32_t screens) { return c.u32(screens + 0x24 + current_row(c, screens)); }

// A control's two words in the current screen's table, and the action that falls back on the
// one the dialog left behind. `via` is the register the original addresses through; it keeps
// the address, and the caller's next `bl` may see it.
inline constexpr std::int32_t control_action_word = -0x5794, control_value_word = -0x5754;
inline std::uint32_t control_word(Cpu& c, std::uint32_t table, std::int16_t control, std::int32_t word, unsigned via) {
    if (compare(c, control, -1) == 0) return 0;
    const auto address = table + 0xc0000 + std::uint32_t(std::int32_t(control)) * 0x17c;
    c.set(via, address);
    return c.u32(address + std::uint32_t(word));
}
inline std::int32_t control_action(Cpu& c, std::uint32_t screens, std::uint32_t table, std::int16_t control, unsigned via) {
    auto action = std::int16_t(control_word(c, table, control, control_action_word, via));
    if (compare(c, action) == 0) {
        c.set(via, screens + screens_high);
        action = c.i16(screens + screens_high - 0x7e62);
    }
    return action;
}
}
