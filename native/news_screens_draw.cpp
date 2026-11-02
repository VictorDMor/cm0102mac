#include "news_screens_detail.hpp"
#include <array>

namespace cm::aot::news_screens {
using namespace detail;

namespace {
// The tabs in the frame: 25 label records of 0xd4 bytes (only eight are filled).
constexpr std::uint32_t label_size = 0xd4, label_count = 0x19;
constexpr std::array<std::uint32_t, 8> tab_phrases{phrase::all, phrase::messages, phrase::competitions, phrase::injuries,
    phrase::contracts, phrase::transfers, phrase::jobs, phrase::records};
constexpr std::int32_t row_address = 0x16f8, table_list = 0x16f4, y = 0x80, bottom = 0x7c, list_count = 0x84, row_y = 0x78,
    rendered = 0xa8, rendered_second = 0xa0, labels = 0x240, action_text = 0x74;
}

// The whole screen, rebuilt: the list of headlines for the tab, and the selected item below it.
void draw_news(Cpu& c) {
    Frame f(c, 0x102cea30, 0x1750, {.first_gpr = 13});
    Kept<19> first_listed(c, none);
    Kept<28> ink_slot(c, c.toc(slot::ink));
    Kept<29> text(c, c.toc(slot::text));
    Kept<30> pool(c, c.toc(slot::pool));
    Kept<31> screens(c, c.toc(slot::screens));
    const History history{c, screens};
    const auto set_stack = [&](std::int32_t offset, std::uint32_t value) { c.put32(f.at(offset), value); };
    const auto stack = [&](std::int32_t offset) { return c.u32(f.at(offset)); };
    const auto outgoing = [&](std::initializer_list<std::uint32_t> values) {
        std::int32_t at = 0x38;
        for (const auto value : values) { set_stack(at, value); at += 4; }
    };
    const auto ink = [&] { return std::uint32_t(c.u16(ink_slot)); };
    const auto table_of_screen = [&] { return c.u32(stack(table_list) + std::uint32_t(std::int32_t(c.i16(stack(row_address)))) * 0x2c0); };
    bl(c, construct_array, 0x102cea6c, {f.at(labels), c.toc(slot::label_construct), c.toc(slot::label_destroy), label_size, label_count});
    c.set(5, c.u16(c.r(2) - 0x38d4));
    c.put16(f.at(0x6c), c.r(5));
    c.put16(f.at(0x68), c.u16(c.r(2) - 0x38d0));
    bl(c, init_text, 0x102cea88, {f.at(rendered), 0});
    bl(c, init_text, 0x102cea94, {f.at(rendered_second), 0});
    set_stack(row_address, screens + 0x2c58);
    Kept<24> manager(c, screen_manager_person(c, screens));
    Kept<16> check_flag(c, std::uint32_t(std::int32_t(std::int8_t(history.value(0, 0x102ceabc)))));
    Kept<13> tab(c, std::uint32_t(std::int32_t(std::int8_t(history.value(1, 0x102ceacc)))));
    Kept<23> list(c, history.value(2, 0x102ceadc));
    set_stack(list_count, history.value(3, 0x102ceaf0));
    Kept<14> reload(c, std::uint32_t(std::int32_t(std::int8_t(history.value(4, 0x102ceb00)))));
    Kept<15> filter(c, history.value(8, 0x102ceb10));
    if (compare_logical(c, bl(c, find_text, 0x102ceb1c, {filter, pool + phrase::filter_placeholder})) != 0)
        bl(c, format_text, 0x102ceb30, {filter, pool + phrase::empty});
    c.set(4, tab * 3);
    Kept<21> tab_values(c, tab * 3 + 9);
    const auto selected_id = history.value(tab_values, 0x102ceb44);
    const bool checking = byte_flag(c, check_flag);
    Kept<20> selected(c, selected_id);
    if (checking) {
        if (!byte_flag(c, bl(c, requires_attention, 0x102ceb64, {c.u32(c.toc(slot::news)), manager, 1})) &&
            !byte_flag(c, bl(c, screen_flag, 0x102ceb78, {screens, manager}))) {
            bl(c, begin_plain_screen, 0x102ceb88, {0});
            bl(c, destroy_text, 0x102ceb94, {f.at(rendered_second), none});
            bl(c, destroy_text, 0x102ceba0, {f.at(rendered), none});
            bl(c, destroy_array, 0x102cebb4, {f.at(labels), c.toc(slot::label_destroy), label_size, label_count});
            return;
        }
        history.set(0, 0, 0x102cebcc);
    }
    if (byte_flag(c, reload)) {
        if (compare_logical(c, list) != 0) {
            bl(c, release_list, 0x102cebe4, {list});
            list = 0;
            history.set(2, 0, 0x102cebfc);
            set_stack(list_count, 0);
            history.set(3, 0, 0x102cec18);
        }
        history.set(4, 0, 0x102cec2c);
    }
    if (compare_logical(c, list) == 0) {
        list = bl(c, recent_items, 0x102cec4c, {c.u32(c.toc(slot::news)), manager, f.at(list_count), tab});
        history.set(2, list, 0x102cec68);
        history.set(3, stack(list_count), 0x102cec7c);
    }
    bl(c, begin_screen, 0x102cec80);

    // The manager's colours: the club's, the nation's, or the screen's.
    const auto owner_colours = [&](std::uint32_t offset, std::uint32_t back) {
        const auto owner = c.u32(manager + offset);
        c.set(3, owner);
        if (compare_logical(c, owner) == 0) return false;
        bl(c, person_colours, back, {owner, 1, f.at(0x72), f.at(0x70)});
        return true;
    };
    if (!owner_colours(0x24, 0x102cec9c) && !owner_colours(0x20, 0x102cecbc)) {
        c.put16(f.at(0x72), bl(c, screen_colour, 0x102cecc8, {1}));
        c.put16(f.at(0x70), bl(c, screen_colour, 0x102cecd4, {0}));
    }
    bl(c, person_name, 0x102cecec, {manager, f.at(0xb0), 0x190, 1});
    if (compare(c, std::int8_t(bl(c, current_language, 0x102cecf0)), 2) == 0) {
        const auto letter = std::int8_t(c.u8(f.at(0xb0)));
        compare_logical(c, std::uint32_t(std::int32_t(letter)) - 0x41, 0x38);
        if (starts_with_vowel(letter)) bl(c, format_text, 0x102ced34, {text, pool + phrase::title_vowel, f.at(0xb0)});
        else bl(c, format_text, 0x102ced48, {text, pool + phrase::title_consonant, f.at(0xb0)});
    } else {
        bl(c, localized_text, 0x102ced5c, {text, pool + phrase::title, f.at(0xb0)});
    }
    bl(c, set_text, 0x102ced68, {f.at(rendered), text});
    bl(c, link_person, 0x102ced7c, {f.at(rendered), f.at(0xb0), 0x7d7, manager});
    // The title's table and the text in it.
    set_stack(table_list, screens + 0x24);
    outgoing({0, 0x30, c.u16(f.at(0x70)), none});
    bl(c, add_table, 0x102cedd8, {table_of_screen(), 0x64, 0xa, 0x316, 0x46, 1, 0, 1});
    set_stack(row_y, 0);
    outgoing({0, 1, c.u16(f.at(0x70)), none});
    const auto title_table = bl(c, add_table, 0x102cee30, {table_of_screen(), 0x64, 0x19, 0x316, 0x37, 1, 0, 1});
    set_stack(0x38, 0);
    bl(c, draw_text, 0x102cee5c, {f.at(rendered), title_table, 0x64, 0x316, f.at(row_y), 7, c.u16(f.at(0x72)), 3});
    bl(c, set_screen_colours, 0x102cee70, {screens, c.u16(f.at(0x70)), c.u16(f.at(0x72)), text});

    // The tabs: the label's text, then enabled and its value, set when the next one starts.
    keep(c, {{14, 0}});
    bl(c, localized_text, 0x102cee84, {f.at(labels + 4), pool + tab_phrases[0], pool + phrase::empty});
    set_stack(labels + 0xcc, 1);
    Kept<18> value_field(c, f.at(labels + 0xd0));
    Kept<17> enabled_field(c, f.at(labels + 0xcc));
    set_stack(labels + 0xd0, 0);
    constexpr std::array<std::uint32_t, 7> backs{0x102ceebc, 0x102ceee8, 0x102cef18, 0x102cef48, 0x102cef78, 0x102cefa8, 0x102cefd8};
    for (std::uint32_t n = 1; n <= 7; ++n) {
        keep(c, {{14, n}, {16, n * label_size}});
        bl(c, localized_text, backs[n - 1], {f.at(labels) + n * label_size + 4, pool + tab_phrases[n]});
        c.put32(enabled_field + n * label_size, 1);
        c.put32(value_field + n * label_size, n);
    }
    set_stack(y, 0x50);
    set_stack(bottom, 0x221);
    bl(c, add_tabs, 0x102cf018, {f.at(labels), 8, tab, f.at(y), f.at(bottom), 1});

    // The list of headlines: as many rows as fit.
    const auto line = bl(c, line_scale, 0x102cf020, {2});
    const auto rows_fitting = std::uint32_t(divide_signed<4>(0x5a, line));
    outgoing({0, 1, 0, none});
    value_field = rows_fitting & 0xff;
    const auto top = stack(y);
    Kept<27> list_table(c, bl(c, add_table, 0x102cf07c, {table_of_screen(), 0x6e, top, 0x30c, top + 0x5a, 2, f.at(0x6c), value_field}));
    set_stack(row_y, 0);
    keep(c, {{13, none}, {14, 0}});
    history.set(5, 0, 0x102cf0a4);
    Kept<18>& visible = value_field;
    const auto matches = [&](std::uint32_t first_text, std::uint32_t back) {
        return byte_flag(c, bl(c, text_contains, back, {first_text, filter, 0}));
    };

    if (compare_logical(c, list) != 0) {
        // No selection on this tab yet: pick the item the player most needs to see.
        if (compare(c, std::int32_t(std::uint32_t(selected))) == 0) {
            keep(c, {{25, 0}, {17, 0}, {26, 0}, {13, 0}});
            while (compare(c, std::int32_t(c.r(25)), std::int32_t(stack(list_count))) < 0) {
                const NewsItem item{c, list + c.r(26)};
                c.set(22, item.address);
                keep(c, {{16, none}});
                bool listed = true;
                if (compare_logical(c, bl(c, string_length, 0x102cf0d8, {filter})) != 0) {
                    bl(c, render_message, 0x102cf0f8, {item.address, manager, f.at(rendered), 0, 0});
                    if (!matches(c.u32(f.at(rendered)), 0x102cf108)) {
                        bl(c, clear_text, 0x102cf118, {f.at(rendered)});
                        bl(c, render_message, 0x102cf130, {item.address, manager, f.at(rendered), 1, 1});
                        if (!matches(c.u32(f.at(rendered)), 0x102cf140)) {
                            bl(c, clear_text, 0x102cf150, {f.at(rendered)});
                            listed = false;
                        }
                    }
                }
                if (listed) {
                    if (compare(c, std::int32_t(std::uint32_t(first_listed)), -1) == 0) first_listed = item.id();
                    if (!byte_flag(c, item.read())) {
                        if (compare_logical(c, history.value(8, 0x102cf178)) != 0) {
                            keep(c, {{16, 1}});
                        } else {
                            const auto priority = item.priority();
                            c.set(3, priority);
                            const bool flagged = byte_flag(c, priority);
                            keep(c, {{16, priority}});
                            if (!flagged &&
                                compare(c, std::int8_t(bl(c, action_label, 0x102cf1b0, {item.address, manager, text, 0x7d0, f.at(action_text)})), 2) == 0)
                                keep(c, {{16, 1}});
                        }
                    }
                    if (compare(c, std::int8_t(bl(c, action_label, 0x102cf1d8, {item.address, manager, text, 0x7d0, f.at(action_text)})), 2) == 0)
                        keep(c, {{16, 1}});
                    if (compare(c, std::int8_t(c.r(16)), std::int8_t(c.r(17))) >= 0) {
                        selected = item.id();
                        bl(c, set_history_value, 0x102cf210, {screens, tab_values, selected, 0});
                        keep(c, {{13, c.r(25)}, {17, c.r(16)}});
                    }
                }
                keep(c, {{26, c.r(26) + 0xe4}, {25, c.r(25) + 1}});
            }
        }
        if (compare(c, std::int32_t(std::uint32_t(selected))) == 0) {
            selected = first_listed;
            bl(c, set_history_value, 0x102cf24c, {screens, tab_values, first_listed, 0});
        }
        // The rows.
        keep(c, {{19, 0}, {17, 0}});
        while (compare(c, std::int32_t(c.r(19)), std::int32_t(stack(list_count))) < 0) {
            const NewsItem item{c, list + c.r(17)};
            c.set(22, item.address);
            bl(c, render_message, 0x102cf274, {item.address, manager, f.at(rendered), 0, 0});
            bool shown = true;
            if (compare_logical(c, bl(c, string_length, 0x102cf27c, {filter})) != 0 && !matches(c.u32(f.at(rendered)), 0x102cf294)) {
                bl(c, render_message, 0x102cf2b4, {item.address, manager, f.at(rendered_second), 1, 1});
                if (!matches(c.u32(f.at(rendered_second)), 0x102cf2c4)) {
                    bl(c, clear_text, 0x102cf2d4, {f.at(rendered_second)});
                    bl(c, clear_text, 0x102cf2dc, {f.at(rendered)});
                    shown = false;
                }
            }
            if (shown) {
                // The date, short in this year.
                const auto year = item.year();
                c.set(8, year);
                c.put16(f.at(0x9a), year);
                c.set(9, extended(item.day()));
                c.set(7, item.date_flag());
                c.put16(f.at(0x98), c.r(9));
                c.put8(f.at(0x9c), c.r(7));
                const bool this_year = compare_logical(c, year, c.u16(c.toc(slot::today) + 2)) == 0;
                if (this_year) {
                    c.set(6, item.weekday());
                    c.put16(f.at(0x90), c.r(9));
                    c.put16(f.at(0x92), c.r(8));
                    c.put8(f.at(0x94), c.r(7));
                    bl(c, short_date, 0x102cf32c, {f.at(0x90), text, 0x7d0});
                } else {
                    c.put16(f.at(0x88), c.r(9));
                    c.put16(f.at(0x8a), c.r(8));
                    c.put8(f.at(0x8c), c.r(7));
                    bl(c, long_date, 0x102cf384, {f.at(0x88), text, 0x7d0, 0xff});
                }
                set_stack(0x38, ink());
                set_stack(0x3c, text);
                bl(c, add_cell, this_year ? 0x102cf360 : 0x102cf3b8, {screens, list_table, 0, stack(row_y), 0x30, halfword(c, slot::table_font), 0xc, 1});

                if (byte_flag(c, item.read())) keep(c, {{16, halfword(c, slot::read_ink)}});
                else keep(c, {{16, ink()}});
                if (compare(c, std::int32_t(std::uint32_t(selected)), std::int32_t(item.id())) == 0) {
                    // The selected row: its headline, acknowledged, its scroll position kept in view.
                    bl(c, render_message, 0x102cf3f8, {item.address, manager, f.at(rendered), 0, 0});
                    bl(c, format_text, 0x102cf408, {text, pool + phrase::headline, c.u32(f.at(rendered))});
                    set_stack(0x38, c.r(16));
                    set_stack(0x3c, text);
                    bl(c, add_cell, 0x102cf438, {screens, list_table, 1, stack(row_y), 0x210, halfword(c, slot::item_font), 1, 2});
                    if (!byte_flag(c, item.read())) {
                        bl(c, acknowledge, 0x102cf45c, {c.u32(c.toc(slot::news)), manager, item.id(), 1});
                        history.set(4, 1, 0x102cf470);
                    }
                    keep(c, {{14, item.address}});
                    history.set(5, item.address, 0x102cf488);
                    const auto scroll = tab_values + 1;
                    while (compare(c, std::int32_t(history.value(scroll, 0x102cf4bc)), std::int32_t(stack(row_y))) > 0)
                        bl(c, set_history_value, 0x102cf4b0, {screens, scroll, history.value(scroll, 0x102cf498) - 1, 0});
                    while (compare(c, std::int32_t(visible + (history.value(scroll, 0x102cf500) - 1)), std::int32_t(stack(row_y))) < 0)
                        bl(c, set_history_value, 0x102cf4f4, {screens, scroll, history.value(scroll, 0x102cf4dc) + 1, 0});
                } else {
                    bl(c, format_text, 0x102cf528, {text, pool + phrase::headline, c.u32(f.at(rendered))});
                    outgoing({c.r(16), text, 2, item.id()});
                    bl(c, add_button, 0x102cf564, {screens, list_table, 1, stack(row_y), 2, 0, 1, 2});
                }
                set_stack(row_y, stack(row_y) + 1);
            }
            keep(c, {{17, c.r(17) + 0xe4}, {19, c.r(19) + 1}});
        }
    }
    // Blank rows to fill the table.
    while (compare(c, std::int32_t(stack(row_y)), std::int32_t(visible)) < 0) {
        c.set(6, stack(row_y));
        outgoing({ink(), 0});
        bl(c, add_cell, 0x102cf5b8, {screens, list_table, 0, stack(row_y), 2, 0, 1, 2});
        outgoing({ink(), 0});
        bl(c, add_cell, 0x102cf5ec, {screens, list_table, 1, stack(row_y), 2, 0, 1, 2});
        set_stack(row_y, stack(row_y) + 1);
    }
    c.set(6, stack(row_y));

    // Keep the selected row within the scroll range.
    const auto scroll = tab_values + 1;
    if (compare(c, std::int32_t(c.r(13)), -1) > 0) {
        const auto selected_row = std::int32_t(c.r(13));
        const auto first_visible = history.value(scroll, 0x102cf618);
        c.set(4, c.r(13) - visible);
        keep(c, {{16, c.r(13) - visible + 1}});
        const auto lowest = std::int32_t(c.r(16));
        std::int32_t wanted = compare(c, std::int32_t(first_visible), lowest) > 0
            ? std::int32_t(history.value(scroll, 0x102cf634)) : result(c, std::uint32_t(lowest));
        if (compare(c, wanted, selected_row) < 0) {
            wanted = compare(c, std::int32_t(history.value(scroll, 0x102cf650)), lowest) > 0
                ? std::int32_t(history.value(scroll, 0x102cf664)) : result(c, std::uint32_t(lowest));
        } else {
            wanted = selected_row;
        }
        bl(c, set_history_value, 0x102cf688, {screens, scroll, std::uint32_t(wanted), 0});
    }
    // Scroll the table and remember which it is.
    const auto scroll_to = history.value(scroll, 0x102cf694);
    const auto table_id = std::int16_t(std::uint32_t(list_table));
    c.set(5, std::uint32_t(std::int32_t(c.i16(stack(row_address)))) * 0x2c0);
    c.set(6, c.u32(stack(table_list) + c.r(5)));
    c.set(4, stack(table_list));
    if (compare(c, table_id, -1) != 0) {
        c.set(5, std::uint32_t(std::int32_t(table_id)) * 0xbf0);
        bl(c, select_row, 0x102cf6c8, {c.r(6) + c.r(5) + 4, scroll_to});
    }
    bl(c, set_history_value, 0x102cf6dc, {screens, tab_values + 2, extended(table_id), 0});

    // The filter box and "next unread".
    const auto filter_y = stack(y) + 0x5f;
    set_stack(y, filter_y);
    keep(c, {{13, filter_y}});
    outgoing({0, 2, 0, none});
    Kept<17> filter_table(c, bl(c, add_table, 0x102cf738, {table_of_screen(), 0x195, filter_y, 0x28f, filter_y + 0x14, 2, f.at(0x68), 1}));
    bl(c, localized_text, 0x102cf748, {text, pool + phrase::filter});
    outgoing({ink(), text});
    bl(c, add_cell, 0x102cf778, {screens, filter_table, 0, 0, 1, 0, 0xc, 1});
    keep(c, {{16, 1}});
    outgoing({1, 0, 0, 1, 1, halfword(c, slot::filter_font), filter, 0x19, 9, 0, filter_table});
    bl(c, add_table_field, 0x102cf7f0, {table_of_screen(), 8, 0, 0, 0, 0, 1, 0});
    bl(c, localized_text, 0x102cf7fc, {text, pool + phrase::next_unread});
    if (nonzero(c, std::int32_t(bl(c, unread_count, 0x102cf810, {c.u32(c.toc(slot::news)), manager, 0})))) {
        const auto top_y = stack(y);
        outgoing({1, ink(), text, 5, 0});
        bl(c, add_labelled_box, 0x102cf864, {screens, 0x28f, top_y, 0x30c, top_y + 0x14, 0x30, halfword(c, slot::button_font), 0xc});
    } else {
        const auto top_y = stack(y);
        outgoing({1, ink(), text});
        bl(c, add_box, 0x102cf8a4, {screens, 0x28f, top_y, 0x30c, top_y + 0x14, 0x30, halfword(c, slot::button_font), 0x2c});
    }

    // The selected item: headline, then the story.
    const auto headline_y = stack(y) + 0x19;
    set_stack(y, headline_y);
    outgoing({0, 2, 0, none});
    Kept<15> body_table(c, bl(c, add_table, 0x102cf8fc, {table_of_screen(), 0x6e, headline_y, 0x30c, headline_y + 0x23, 1, 0, 1}));
    set_stack(row_y, 0);
    const std::uint32_t shown = c.r(14);
    if (compare_logical(c, shown) != 0) {
        bl(c, render_message, 0x102cf928, {shown, manager, f.at(rendered), 0, 0});
        set_stack(0x38, 0);
        bl(c, draw_text, 0x102cf958, {f.at(rendered), body_table, 0x78, 0x302, f.at(row_y), 3, halfword(c, slot::headline_font), 3});
    }
    c.set(4, stack(y));
    set_stack(y, stack(y) + 0x28);
    const auto scale = bl(c, line_scale, 0x102cf96c, {3});
    const auto story_y = stack(y);
    const auto story_bottom = stack(bottom);
    c.set(8, story_bottom - story_y - 0x10);
    const auto rows = std::uint32_t(divide_signed<4>(c.r(8), scale));
    outgoing({0, 2, 0, none});
    body_table = bl(c, add_table, 0x102cf9c8, {table_of_screen(), 0x6e, story_y, 0x30c, story_bottom, 1, 0, rows & 0xff});
    set_stack(row_y, 0);
    if (compare_logical(c, shown) != 0) {
        bl(c, render_message, 0x102cf9f4, {shown, manager, f.at(rendered), 1, 0});
        set_stack(0x38, 0);
        bl(c, draw_text, 0x102cfa20, {f.at(rendered), body_table, 0x78, 0x302, f.at(row_y), 3, ink(), 0});
        bl(c, release_block, 0x102cfa28, {c.u32(f.at(rendered))});
    }

    // The item's action button.
    const auto button_y = std::uint32_t(c.r(13));
    body_table = 0;
    if (compare_logical(c, shown) != 0) {
        const auto kind = bl(c, action_label, 0x102cfa4c, {shown, manager, text, 0x7d0, f.at(action_text)});
        if (compare(c, std::int8_t(kind)) != 0) {
            if (compare(c, std::int8_t(kind), 2) == 0) {
                const auto row = current_row(c, screens);
                if (compare(c, c.i16(screens + row + 0x14)) == 0) {
                    const auto preferences = c.u32(c.u32(c.toc(slot::preferences)) + 4);
                    const bool highlighted = compare_logical(c, preferences & 0x20000) != 0;
                    const auto font = halfword(c, slot::button_font);
                    keep(c, {{14, 1}});
                    const auto colour = ink();
                    keep(c, {{13, colour}});
                    outgoing({0x30, font, font, 0xc, 1, colour, text, 0, 3, stack(action_text), none});
                    bl(c, add_table_field, highlighted ? 0x102cfb0c : 0x102cfb8c,
                       {c.u32(stack(table_list) + row), highlighted ? 0x802u : 0x2u, 0x6e, button_y, 0xeb, button_y + 0x14, 0, 0});
                    body_table = 1;
                } else {
                    outgoing({1, ink(), text});
                    bl(c, add_box, 0x102cfbd0, {screens, 0x6e, button_y, 0xeb, button_y + 0x14, 0x30, halfword(c, slot::button_font), 0x2c});
                }
            } else {
                outgoing({1, ink(), text, 3, stack(action_text)});
                bl(c, add_labelled_box, 0x102cfc20, {screens, 0x6e, button_y, 0xeb, button_y + 0x14, 0x30, halfword(c, slot::button_font), 0xc});
            }
        }
    }
    bl(c, set_panel, 0x102cfc2c, {4, body_table});
    const auto at_end = std::int8_t(bl(c, history_at_end, 0x102cfc34, {screens}));
    keep(c, {{13, std::uint32_t(at_end == 0)}});
    const auto at_start = std::int8_t(bl(c, history_at_start, 0x102cfc50, {screens, 0}));
    bl(c, set_history_buttons, 0x102cfc68, {std::uint32_t(at_start == 0), c.r(13)});
    history.set(6, bl(c, unread_count, 0x102cfc7c, {c.u32(c.toc(slot::news)), manager, 0}), 0x102cfc90);
    bl(c, destroy_text, 0x102cfc9c, {f.at(rendered_second), none});
    bl(c, destroy_text, 0x102cfca8, {f.at(rendered), none});
    bl(c, destroy_array, 0x102cfcbc, {f.at(labels), c.toc(slot::label_destroy), label_size, label_count});
}
}
