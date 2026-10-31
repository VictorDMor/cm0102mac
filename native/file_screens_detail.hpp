#pragma once
#include "file_screens.hpp"
#include "guest_values.hpp"

namespace cm::aot::file_screens::detail {
using namespace guest;
namespace slot {
inline constexpr std::int32_t screens = -0x7bb0, text = -0x7bb8, message = -0x7ba8, success = -0x7bac, pool = -0x55d4,
    key = -0x74b0, plain_colour = -0x7bc8, folder_colour = -0x7bc4, marked_colour = -0x7bd0, row_colour = -0x7bdc,
    list_colour = -0x7bd8, heading_colour = -0x7bd4, secondary_text = -0x79e0, title_text = -0x79dc,
    message_render = -0x55f0, message_action = -0x55f4, info_render = -0x55e0, info_action = -0x55e4,
    dialog_render = -0x55d8, dialog_action = -0x55dc, list_layout = -0x55e8, integer_bases = -0x55ec, leagues = -0x7b44,
    last_time = -0x74b8, total_time = -0x74b4, game_date = -0x7b80, person_count = -0x7be4, people = -0x7be0;
}
// Offsets into the string pool.
namespace text {
inline constexpr std::uint32_t assertion_format = 0x11, assertion_title = 0x1e, file_label = 0x24, any_file = 0x2b, cancel = 0x2d,
    info = 0x34, ok = 0x39, cannot_create_title = 0x3c, cannot_create = 0x4d, no_folder_title = 0x84, create_folder = 0x98,
    no_folder = 0xe5, blank_file_title = 0x11a, blank_file = 0x128, no_file_title = 0x148, no_file = 0x15b, general_data = 0x182,
    no_info_title = 0x18e, no_info = 0x1a4, blank_folder_title = 0x1d0, blank_folder = 0x1e3, file_exists_title = 0x208,
    file_exists = 0x214, confirm_title = 0x24f, confirm_delete = 0x25e, load_assertion = 0x27e, no_general_data = 0x290,
    load_error_title = 0x2d9, load_error = 0x2e9, no_version = 0x394, no_summary = 0x3e2, info_title = 0x42d, file_name = 0x43c,
    last_saved = 0x446, saved_at = 0x451, version = 0x4cc, date = 0x4d4, game_time = 0x4d9, time_played = 0x4e3,
    addiction = 0x564, addiction_over_10_days = 0x578, addiction_over_8_days = 0x5c4, addiction_over_7_days = 0x606,
    addiction_over_6_days = 0x629, addiction_over_5_days = 0x64c, addiction_over_4_days = 0x661, addiction_over_3_days = 0x671,
    addiction_over_2_days = 0x688, addiction_over_1_day = 0x6a1, addiction_over_12_hours = 0x6b2, addiction_mild = 0x6d4,
    foreground_leagues = 0x6e4, background_leagues = 0x6f7, managers = 0x70a, manager_with_club = 0x713, no = 0x738, yes = 0x73b,
    league_separator = 0x78e, two_clubs = 0x7b0;
}
// Routines of other modules.
inline constexpr std::uint32_t history_value = 0x1073ef70, set_history_value = 0x1073f1e0, append_history = 0x1073e4b0,
    localized_text = 0x10727890, format_text = 0x1077b720, split_path = 0x10739890, make_path = 0x10739b40,
    file_exists_at = 0x10739d60, create_directory = 0x10739cd0, usable_path = 0x10746340, compare_nocase = 0x107393f0,
    find_first = 0x10739690, find_next = 0x107397d0, close_find = 0x107156a0, pointer_glue = 0x10781320,
    string_length = 0x1077dbd0, string_copy = 0x1077dbf0, string_copy_limited = 0x1077dc10, string_append = 0x1077dc60,
    allocate = 0x107735f0, release = 0x10773610, allocate_object = 0x10773520, version_text = 0x10768810, error_dialog = 0x10769b60,
    current_language = 0x10727880, open_entry = 0x107475b0, read_entry = 0x10747a80, close_entry = 0x10747900, absolute = 0x10775550,
    begin_screen = 0x10712d00, begin_plain_screen = 0x10712e60, set_screen_colours = 0x10744250, set_screen_title = 0x107442f0,
    set_panel = 0x102a1070, add_table = 0x107198c0, add_table_field = 0x10719740, select_row = 0x107145f0, add_cell = 0x107445a0,
    add_button = 0x107444e0, add_box = 0x10744390, add_frame = 0x103cbb30, make_colour = 0x1076bb20, row_for_item = 0x10743510,
    default_action = 0x102a54b0, common_action = 0x102a83a0, make_date = 0x10715880, date_text = 0x10716eb0, month_name = 0x107189d0,
    day_name = 0x107187d0, local_time = 0x1077f940, clock_time = 0x1077f900, person_name = 0x100caeb0,
    text_object = 0x1074b730, init_text = 0x1074b6e0, destroy_text = 0x1074b7c0, set_text = 0x1074b830, draw_text = 0x1074bcd0,
    add_text = 0x1074a350, add_text_object = 0x1074a500;
// What a dialog leaves for the screen below it (screens + 0x260000 - offset):
// its answer, the value it hands back, the pending action.
inline constexpr std::uint32_t screens_high = 0x260000, returned_value = 0x7e60, answer_field = 0x7e5c, pending_action = 0x7e62;
inline constexpr std::uint32_t none = 0xffffffff;
// What a dialog returns to the screens when it closes, and the answers it leaves.
inline constexpr std::int32_t closed = -9;
inline constexpr std::uint32_t no_answer = 1, yes_answer = 2, ok_answer = 3;

// A league's record, a person's record, and one manager's row of the summary.
struct League {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint8_t in_foreground = 2, in_background = 1;
    std::uint8_t flags() const { return c.u8(address + 0x120); }
    std::uint32_t name() const { return address + 0xe1; }
};
inline constexpr std::uint32_t league_count = 0x22, league_record_size = 0x4c;
struct Person {
    Cpu& c;
    std::uint32_t address;
    std::uint8_t role() const { return c.u8(address + 0x6b); }
    std::uint32_t club() const { return c.u32(address + 0x24); }
    std::uint32_t other_club() const { return c.u32(address + 0x20); }
};
inline constexpr std::uint32_t person_record_size = 0x70, manager_role = 1, club_name = 0x221;
struct ManagerRow {
    std::uint32_t address;
    std::uint32_t name() const { return address; }
    std::uint32_t club() const { return address + 0x3c; }
};
// The summary saved in general.dat, after the save's version word: 0xf70 bytes
// below version 0x3e027 (the file saves no time then), 0xf74 from it on.
struct SaveSummary {
    Cpu& c;
    std::uint32_t address;
    static constexpr std::uint32_t old_size = 0xf70, size = 0xf74, manager_count = 0x10, manager_size = 0x78;
    std::uint32_t foreground() const { return address; }
    std::uint32_t background() const { return address + 0x3e8; }
    std::uint32_t version() const { return address + 0x7d0; }
    ManagerRow manager(std::uint32_t offset) const { return {address + 0x7e4 + offset}; }
    std::uint32_t date() const { return address + 0xf64; }
    Field<std::uint32_t> played() const { return {c, address + 0xf6c}; }
    Field<std::uint32_t> saved() const { return {c, address + 0xf70}; }
};

struct Program {
    Cpu& c;
    std::uint32_t call(std::uint32_t target, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) const {
        unsigned reg = 3;
        for (const auto value : arguments) c.set(reg++, value);
        c.call(target, back);
        return c.r(3);
    }
    // One value of the current screen's history.
    std::uint32_t value(std::uint32_t back, std::uint32_t screens, std::uint32_t index) const {
        return call(history_value, back, {screens, index});
    }
    void store(std::uint32_t back, std::uint32_t screens, std::uint32_t index, std::uint32_t item, std::uint32_t owned = 0) const {
        call(set_history_value, back, {screens, index, item, owned});
    }
    // sp+0x38 onwards: the arguments that do not fit in registers.
    void stack(std::uint32_t sp, std::initializer_list<std::uint32_t> values) const {
        auto at = sp + 0x38;
        for (const auto value : values) {
            c.put32(at, value);
            at += 4;
        }
    }
};
// The current window's controls.
inline std::uint32_t window(Cpu& c, std::uint32_t screens) {
    return c.u32(screens + 0x24 + std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0);
}
// The control's action; zero takes the pending one the screens hold.
inline std::int16_t control_action(Cpu& c, std::uint32_t screens, std::uint32_t control) {
    const auto id = std::int16_t(control);
    auto action = std::int16_t(0);
    const auto controls = window(c, screens);
    if (id != -1) action = std::int16_t(c.u32(controls + 0xc0000 + std::uint32_t(std::int32_t(id)) * 0x17c - 0x5794));
    return action ? action : c.i16(screens + screens_high - pending_action);
}
// Key presses the dialogs treat as their answers.
inline bool is_no_key(std::int32_t key) { return key == 'N' || key == 'n' || key == 0x1b; }
inline bool is_yes_key(std::int32_t key) { return key == 'Y' || key == 'y' || key == '\r' || key == '\n'; }
}
