#include "file_screens_detail.hpp"

namespace cm::aot::file_screens {
using namespace detail;
namespace {
// Both message boxes open one screen; only what its text slot holds differs.
void report_bad_message(Cpu& c, const Frame& f, std::uint32_t at, std::uint32_t pool, std::uint32_t line) {
    report_assertion(c, {at, pool, pool + text::assertion_format, pool + text::assertion_title, f.at(0x40), f.at(0x38), line});
    c.set(3, c.toc(slot::success));
}
bool open_message_screen(Cpu& c, std::uint32_t back, std::uint32_t screens) {
    return compare(c, std::int8_t(Program{c}.call(append_history, back, {screens, c.toc(slot::message_render),
                                                                         c.toc(slot::message_action), 1, 0, 0}))) != 0;
}
}

void open_message(Cpu& c, std::uint32_t heading, std::uint32_t title, std::uint32_t body, std::uint32_t confirm,
                  std::uint32_t text_colour, std::uint32_t back_colour, std::uint32_t parent_value) {
    Frame f(c, 0x100f05a0, 0x90, {.first_gpr = 22});
    const auto screens = c.toc(slot::screens), pool = c.toc(slot::pool);
    keep(c, {{22, heading}, {29, screens}, {30, c.toc(slot::message)}, {23, title}, {31, pool}, {24, body},
             {25, confirm}, {26, text_colour}, {27, back_colour}, {28, parent_value}});
    if (compare(c, std::int32_t(heading)) == 0 || compare_logical(c, title) == 0 || compare_logical(c, body) == 0)
        return report_bad_message(c, f, 0x100f0600, pool, 0x553);
    if (!open_message_screen(c, 0x100f0660, screens)) return;
    Program p{c};
    p.store(0x100f067c, screens, 0, heading, 1);
    p.store(0x100f0690, screens, 1, title, 1);
    p.store(0x100f06a4, screens, 2, body, 1);
    p.store(0x100f06b8, screens, 3, confirm & 0xff);
    p.store(0x100f06cc, screens, 4, extended(std::int16_t(text_colour)));
    p.store(0x100f06e0, screens, 5, extended(std::int16_t(back_colour)));
    p.store(0x100f06f4, screens, 6, 0);
    p.store(0x100f0708, screens, 7, parent_value);
}

void open_long_message(Cpu& c, std::uint32_t heading, std::uint32_t title, std::uint32_t body, std::uint32_t confirm,
                       std::uint32_t text_colour, std::uint32_t back_colour, std::uint32_t parent_value) {
    Frame f(c, 0x100f0720, 0x90, {.first_gpr = 22});
    const auto screens = c.toc(slot::screens), pool = c.toc(slot::pool);
    keep(c, {{22, heading}, {29, screens}, {23, title}, {31, pool}, {24, body}, {25, confirm}, {26, text_colour}, {27, back_colour},
             {28, parent_value}});
    Kept<30> text_object_or_buffer(c, c.toc(slot::message));
    if (compare(c, std::int32_t(heading)) == 0 || compare_logical(c, title) == 0 || compare_logical(c, body) == 0)
        return report_bad_message(c, f, 0x100f0780, pool, 0x57c);
    Program p{c};
    const auto object = p.call(allocate_object, 0x100f07cc, {8});
    text_object_or_buffer = object;
    if (compare(c, std::int32_t(object)) != 0) {
        c.set(4, body);
        c.call(text_object, 0x100f07dc);
    }
    if (!open_message_screen(c, 0x100f07f8, screens)) return;
    p.store(0x100f0814, screens, 0, heading, 1);
    p.store(0x100f0828, screens, 1, title, 1);
    p.store(0x100f083c, screens, 2, 0);
    p.store(0x100f0850, screens, 3, confirm & 0xff);
    p.store(0x100f0864, screens, 4, extended(std::int16_t(text_colour)));
    p.store(0x100f0878, screens, 5, extended(std::int16_t(back_colour)));
    p.store(0x100f088c, screens, 6, object);
    p.store(0x100f08a0, screens, 7, parent_value);
}

void render_message(Cpu& c) {
    Frame f(c, 0x100f08c0, 0xa0, {.first_gpr = 20});
    const auto screens = c.toc(slot::screens), label = c.toc(slot::text), pool = c.toc(slot::pool);
    const auto disabled = c.toc(slot::folder_colour), plain = c.toc(slot::plain_colour);
    keep(c, {{30, screens}, {27, disabled}, {28, plain}, {29, label}, {31, pool}});
    c.put16(f.at(0x68), c.u16(c.r(2) - 0x39b8));
    Program p{c};
    Kept<23> heading(c, p.value(0x100f08f8, screens, 0));
    Kept<22> title(c, p.value(0x100f090c, screens, 1));
    Kept<24> body_or_buttons(c, p.value(0x100f0920, screens, 2));
    Kept<25> confirm(c, p.value(0x100f0930, screens, 3) & 0xff);
    Kept<21> text_colour(c, extended(std::int16_t(p.value(0x100f0940, screens, 4))));
    Kept<20> back_colour(c, extended(std::int16_t(p.value(0x100f0950, screens, 5))));
    Kept<26> body_object(c, p.value(0x100f0960, screens, 6));
    c.call(begin_screen, 0x100f0968);
    p.call(set_panel, 0x100f0974, {4, 0});
    p.call(set_screen_colours, 0x100f0988, {screens, back_colour & 0xffff, text_colour & 0xffff, heading});
    p.call(set_screen_title, 0x100f0994, {screens, title});
    if (compare_logical(c, body_or_buttons) != 0) {
        p.stack(f.base(), {c.u16(plain), body_or_buttons, 0});
        p.call(add_text, 0x100f09d4, {0x6e, 0x91, 0x30c, 0x217, 2, 0, 0x10, 4});
    } else {
        p.stack(f.base(), {c.u16(plain), body_object});
        p.call(add_text_object, 0x100f0a08, {0x6e, 0x91, 0x30c, 0x217, 2, 0, 0x10, 4});
    }
    if (compare_logical(c, confirm) == 0) {
        p.call(localized_text, 0x100f0a1c, {label, pool + text::ok});
        const auto button = c.u16(disabled);
        p.stack(f.base(), {0x30, button, button, 0xc, 4, c.u16(plain), label, 0, ok_answer, 0, none});
        p.call(add_table_field, 0x100f0a98, {window(c, screens), 2, 0x64, 0x22b, 0x316, 0x24e, 0, 0});
    } else if (compare_logical(c, confirm, 1) == 0) {
        p.stack(f.base(), {0, 1, 0, none});
        body_or_buttons = p.call(add_table, 0x100f0af0, {window(c, screens), 0x64, 0x22b, 0x316, 0x24e, 2, f.at(0x68), 1});
        p.call(localized_text, 0x100f0b00, {label, pool + text::no});
        p.stack(f.base(), {c.u16(plain), label, no_answer, 0});
        p.call(add_button, 0x100f0b40, {screens, body_or_buttons, 0, 0, 0x30, c.u16(disabled), 0xc, 3});
        p.call(localized_text, 0x100f0b4c, {label, pool + text::yes});
        p.stack(f.base(), {c.u16(plain), label, yes_answer, 0});
        p.call(add_button, 0x100f0b8c, {screens, body_or_buttons, 1, 0, 0x30, c.u16(disabled), 0xc, 3});
    }
}

void message_action(Cpu& c, std::uint32_t control) {
    Frame f(c, 0x100f0ba0, 0x50, {.first_gpr = 28});
    const auto screens = c.toc(slot::screens), key = c.toc(slot::key);
    keep(c, {{31, screens}, {30, control}, {29, key}});
    Program p{c};
    const auto assigned = control_action(c, screens, control);
    Kept<28> action(c, extended(assigned));
    if (compare(c, assigned) == 0) {
        const std::int32_t pressed = c.i8(key);
        if (is_no_key(pressed)) {
            action = no_answer;
            c.put8(key, 0);
        } else if (is_yes_key(pressed)) {
            // The original's key switch leaves cr0 equal at this call. The key answers
            // Yes on a screen with Yes and No, and Ok on one with no question.
            compare(c, pressed, pressed);
            const auto confirm = p.value(0x100f0c94, screens, 3) & 0xff;
            c.put8(key, 0);
            c.set(3, confirm ? 0 : 1);
            action = confirm ? yes_answer : ok_answer;
        }
    }
    const auto chosen = std::int32_t(std::uint32_t(action));
    if (compare(c, chosen, ok_answer) == 0 || compare(c, chosen, no_answer) == 0 || compare(c, chosen, yes_answer) == 0) {
        if (compare_logical(c, p.value(0x100f0cd4, screens, 6)) != 0) {
            p.call(destroy_text, 0x100f0cf0, {p.value(0x100f0ce8, screens, 6), 1});
            p.store(0x100f0d04, screens, 6, 0);
        }
        const auto parent_value = p.value(0x100f0d10, screens, 7);
        const auto dialog = screens + screens_high;
        c.set(4, dialog);
        c.put32(dialog - returned_value, parent_value);
        c.put32(dialog - answer_field, std::uint32_t(chosen));
        c.set(3, std::uint32_t(closed));
        return;
    }
    if (compare(c, std::int32_t(p.call(default_action, 0x100f0d2c, {control}))) != 0) return;
    p.call(common_action, 0x100f0d40, {control});
}
}
