#include "news_screens_detail.hpp"

namespace cm::aot::news_screens {
using namespace detail;

void draw_message(Cpu& c) {
    Frame f(c, 0x102d0500, 0x160, {.first_gpr = 22});
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<27> button_font_slot(c, c.toc(slot::button_font));
    Kept<28> ink(c, c.toc(slot::ink));
    Kept<29> text(c, c.toc(slot::text));
    Kept<30> pool(c, c.toc(slot::pool));
    c.put16(f.at(0x68), c.u16(c.r(2) - 0x38cc));
    const History history{c, screens};
    Kept<25> recipient(c, history.value(0, 0x102d0538));
    Kept<24> message(c, history.value(1, 0x102d054c));
    bl(c, set_person_screen, 0x102d0564, {recipient, 0, 0});
    Kept<23> background(c, bl(c, screen_background, 0x102d0568));
    Kept<22> foreground(c, bl(c, screen_colour, 0x102d0574, {1}));
    bl(c, set_screen_colours, 0x102d0594, {screens, bl(c, screen_colour, 0x102d0580, {0}), foreground, background});
    bl(c, set_panel, 0x102d05a0, {4, 0});
    if (compare_logical(c, recipient) != 0) {
        bl(c, person_name, 0x102d05bc, {recipient, f.at(0x6c), 0xc8, 1});
        bl(c, localized_text, 0x102d05cc, {text, pool + phrase::send_to, f.at(0x6c)});
    } else {
        bl(c, localized_text, 0x102d05dc, {text, pool + phrase::send_to_all});
    }
    bl(c, set_screen_title, 0x102d05e8, {screens, text});

    const auto words = [&](std::initializer_list<std::uint32_t> values) {
        std::uint32_t at = 0x38;
        for (const auto value : values) { c.put32(f.at(std::int32_t(at)), value); at += 4; }
    };
    keep(c, {{26, screens + 0x2c58}, {25, screens + 0x24}, {23, 4}, {22, 0}});
    words({0x22, halfword(c, slot::table_font), 0, 0xc, 4, c.u16(ink), message, 0x64, 6, 0, none});
    bl(c, add_table_field, 0x102d0674, {screen_table(c, screens), 0x10018, 0x96, 0x136, 0x2e4, 0x172, 0, 0});
    words({0, 1, 0, none});
    Kept<22> table(c, bl(c, add_table, 0x102d06bc, {screen_table(c, screens), 0x64, 0x22b, 0x316, 0x24e, 2, f.at(0x68), 1}));
    bl(c, localized_text, 0x102d06cc, {text, pool + phrase::cancel});
    words({c.u16(ink), text, 7, 0});
    bl(c, add_button, 0x102d070c, {screens, table, 0, 0, 0x30, c.u16(button_font_slot), 0xc, 3});
    bl(c, localized_text, 0x102d0718, {text, pool + phrase::send});
    if (compare_logical(c, bl(c, string_length, 0x102d0720, {message})) != 0) {
        words({c.u16(ink), text, 8, 0});
        bl(c, add_button, 0x102d0768, {screens, table, 1, 0, 0x30, c.u16(button_font_slot), 0xc, 3});
    } else {
        words({c.u16(ink), text});
        bl(c, add_cell, 0x102d079c, {screens, table, 1, 0, 0x30, c.u16(button_font_slot), 0x2c, 3});
    }
}

std::uint32_t message_command(Cpu& c, std::uint32_t control) {
    Frame f(c, 0x102d07b0, 0x150, {.first_gpr = 23});
    Kept<31> screens(c, c.toc(slot::screens));
    Kept<23> id(c, control);
    Kept<28> person_count(c, c.toc(slot::person_count));
    Kept<29> people(c, c.toc(slot::people));
    Kept<24> answered(c, screens + 0x2c58);
    Kept<30> news(c, c.toc(slot::news));
    Kept<27> sender(c, screen_manager_person(c, screens));
    const History history{c, screens};
    Kept<26> recipient(c, history.value(0, 0x102d07f4));
    Kept<25> message(c, history.value(1, 0x102d0808));
    answered = history.value(2, 0x102d081c);
    const auto control_id = std::int16_t(control);
    // The table's address is what the first two tests leave in r3; the third goes through it.
    const auto table = c.u32(screens + 0x24 + current_row(c, screens));
    c.set(3, table);
    const auto release_message = [&](std::uint32_t back) {
        if (compare_logical(c, message) != 0) bl(c, release, back, {message});
    };
    if (compare(c, control_action(c, screens, table, control_id, 4), 6) == 0) return result(c, std::uint32_t(redraw));
    if (compare(c, control_action(c, screens, table, control_id, 4), 7) == 0) {
        release_message(0x102d08d0);
        return result(c, std::uint32_t(closed));
    }
    if (compare(c, control_action(c, screens, table, control_id, 3), 8) == 0) {
        const auto item = f.at(0x38);
        bl(c, construct_item, 0x102d092c, {item, 0x1388, 0});
        bl(c, set_item_word, 0x102d093c, {item, 0, c.u32(sender)});
        const bool to_one = compare_logical(c, recipient) != 0;
        bl(c, set_item_word, to_one ? 0x102d0954 : 0x102d0968, {item, 1, to_one ? 1u : 0u});
        bl(c, set_item_word, 0x102d0978, {item, 2, answered});
        bl(c, set_item_text, 0x102d098c, {item, 3, message, 0});
        if (compare_logical(c, recipient) != 0) {
            bl(c, publish_to_manager, 0x102d09a8, {c.u32(news), item, recipient, 0});
        } else {
            // The sixteen human managers, the last people in the table, last first: those flagged at +0x6b, not the sender.
            keep(c, {{23, 0}});
            for (std::int32_t n = 0; n < 0x10; ++n) {
                const auto person = c.u32(people) + (c.u32(person_count) - std::uint32_t(n) - 1) * 0x70;
                c.set({{3, c.u32(person_count) - std::uint32_t(n)}, {4, c.u32(people)}, {5, person}});
                if (compare(c, c.u8(person + 0x6b), 1) == 0 && compare_logical(c, person, sender) != 0)
                    bl(c, publish_to_manager, 0x102d09ec, {c.u32(news), item, person, 0});
                keep(c, {{23, std::uint32_t(n) + 1}});
                compare(c, n + 1, 0x10);
            }
        }
        release_message(0x102d0a08);
        return result(c, std::uint32_t(closed));
    }
    if (nonzero(c, std::int32_t(bl(c, default_action, 0x102d0a18, {id})))) return c.r(3);
    bl(c, common_action, 0x102d0a2c, {id});
    return common_action_result(c);
}
}
