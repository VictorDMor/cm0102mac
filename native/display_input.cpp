// DISPLAY.CPP block "input": the display table's pointer and keyboard handling. See display_input.hpp.
//
// Registers: the routines name their values for what they are; `regs.hold` records what the original's
// r13-r31 hold next to each change and `regs.apply()` writes them before a dispatched or direct call, so
// callees see the register image the original leaves them. Everything else is native.
#include "display_input.hpp"
#include "display_detail.hpp"
#include "aot_runtime.hpp"

namespace cm::aot::display {
namespace {
using u32 = std::uint32_t;

// `extsb.` then beq/bne.
constexpr bool b(u32 v) { return std::int8_t(v & 0xff) != 0; }  // `extsb.` then beq/bne
constexpr std::int32_t w(u32 v) { return std::int32_t(v); }

// Layout of the table's input state (names inferred from use).
namespace tbl {
[[maybe_unused]] inline constexpr u32 mode = 0x0,                       // byte: 1 press on an area, 2 on a scroll bar, 3 on a drag source
    high = 0x130000,
    area_order = 0x129da8, area_order_count = 0x12a8fc,     // halfword area indexes in drawing order, and their count
    guio_order = 0x129f9c, guio_order_count = 0x12a8fe,     // halfword control indexes, and their count
    press_x = 0x12a90c, press_y = 0x12a910,                 // words: the pointer at a press
    press_rect = 0x12a914, press_rect_end = 0x12a924,       // four words, then a zero word
    pressed_area = 0x12a930, pressed_scroll_area = 0x12a932, drag_control = 0x12a934, text_focus = 0x12a936;  // halfwords
}
// An area (0xbf0 bytes from the table + index * 0xbf0).
namespace area {
[[maybe_unused]] inline constexpr u32 parent = 0x212, child = 0x214,        // halfword control indexes (-1: none)
    entries = 0x216,                                       // halfword control indexes
    count = 0xb78, start = 0xb7a, cursor = 0xb7c,          // halfwords: the entries' count, first entry, walk position
    scroll = 0xb80,                                        // the scroll bar's 0x34 bytes
    step = 0xbb5, enabled = 0xbf2, has_scroll = 0xbf3;     // bytes
}
// The scroll bar of an area (words unless noted), from area + 0xb80.
namespace bar {
[[maybe_unused]] inline constexpr u32 left = 0x0, top = 0x4, right = 0x8, bottom = 0xc,  // the control's rectangle
    track_top = 0x10, track_bottom = 0x14, steps = 0x18, position = 0x1c, thumb = 0x20,
    thumb_top = 0x24, thumb_bottom = 0x28, image = 0x2c, hot = 0x30 /* byte */;
}
// A control (GUIO object, 0x17c bytes from table + 0xba864 + index * 0x17c).
namespace control {
[[maybe_unused]] inline constexpr u32 size = 0x17c, base = 0xba864,
    flags = 0xc, left = 0x10, top = 0x14, right = 0x18, bottom = 0x1c,
    line = 0x24, tracks = 0x8, max_length = 0x40, caret = 0x44, overlay = 0x4c, stamp = 0x50,
    font = 0x6e, owner_area = 0x72, area = 0x74, text = 0x78,
    changed = 0x178, hot = 0x17a;                          // bytes
[[maybe_unused]] inline constexpr u32 f_hover = 0x2, f_edit = 0x8, f_drag = 0x40, f_keep_focus = 0x20, f_pressed_flag = 0x100,
    f_path = 0x8000, f_wildcard = 0x10000;
}
inline u32 control_at(u32 table, u32 index) { return table + control::base + index * control::size; }

struct Rect { u32 left, top, right, bottom; };
inline Rect rect_of(const Cpu& c, u32 control) {
    return {c.u32(control + control::left), c.u32(control + control::top), c.u32(control + control::right),
            c.u32(control + control::bottom)};
}
// The pointer in the control's rectangle (the original's call to 0x1071c190 with the four words).
inline u32 pointer_in(Cpu& c, u32 control) {
    const auto r = rect_of(c, control);
    c.set(7, r.bottom);  // the original passes the bottom in r7 and the callee leaves it there
    return pointer_inside(c, r.left, r.top, r.right, r.bottom);
}
// The global byte DAT_108b84d6 (TOC -0x35ba): set to 2 when the pointer touches something that reacts.
inline void touch(Cpu& c) { c.put8(c.r(2) - 0x35ba, 2); }

// The area's cursor walk (0x1071a828..0x1071a864 and its copies): the next entry of the area at `order_at`, or -1.
u32 next_entry(Cpu& c, u32 table, u32 order_at) {
    const u32 a = area_at(table, sign16(c.i16(table + order_at)));
    const auto cursor = c.i16(a + area::cursor), count = c.i16(a + area::count);
    if (cursor < count) {
        c.put16(a + area::cursor, u32(cursor + 1));
        return sign16(c.i16(a + area::entries + (sign16(cursor) << 1)));
    }
    return 0xffffffffu;
}
// Whether entry `e` lies within the area's visible rows: its line less the first entry's line is at most step - 1.
bool within_rows(Cpu& c, u32 table, u32 order_at, u32 e) {
    const u32 a = area_at(table, sign16(c.i16(table + order_at)));
    const u32 step = c.u8(a + area::step);
    const u32 first = sign16(c.i16(a + area::entries + (sign16(c.i16(a + area::start)) << 1)));
    const u32 mine = c.u32(control_at(table, e) + control::line);
    const u32 base = c.u32(control_at(table, first) + control::line);
    return w(mine - base) <= w(step - 1);
}
}

// 0x1071a280: the pointer pass. Two passes (the second only repeats what is not a press) over the controls in
// drawing order (hover, press), then the areas' own entries, ending with the control the pointer is on.
// The original keeps its loop state in r18-r31; `regs.hold` records each value next to the change.
u32 pointer_press(Cpu& c, u32 table) {
    const Frame frame(c, 0x1071a280, 0x70, {.first_gpr = 18});
    CallRegisters regs(c);
    constexpr u32 none = 0xffffffffu;
    const u32 pressed_ptr = c.toc(-0x5c0c), success_ptr = c.toc(toc::success), list_base = table + tbl::high;
    u32 pass = 1, hovered = none;
    regs.hold({{24, pressed_ptr}, {25, success_ptr}, {27, list_base}, {28, table}, {29, pass}, {31, hovered}});
    u32 order_at = 0;  // the current control's offset in the order list (first loop)
    // The hover/press step of one control: `from_order` reads the control again from the order list as the
    // first loop does; the areas' loop keeps it in `control_index`.
    const auto hover = [&](u32 control_index, bool from_order, u32 back_enter, u32 back_leave) {
        const auto current = [&] { return from_order ? sign16(c.i16(table + order_at)) : control_index; };
        const u32 record = control_at(table, control_index);
        const u32 flags = c.u32(record + control::flags);
        if (!(flags & control::f_hover) || half(pass) != 1) return;
        const u32 button_down = c.u8((flags & control::f_pressed_flag) ? pressed_ptr : success_ptr);
        const u32 pointer_over = pointer_in(c, record);
        regs.hold({{18, button_down}, {26, pointer_over}});
        if (b(pointer_over) && b(button_down)) {
            if (!b(c.u8(control_at(table, current()) + control::hot))) {
                regs.apply();
                invoke(c, 0x10726710, back_enter, {control_at(table, current())});
            }
            hovered = current();
            regs.hold({{31, hovered}});
        } else if (b(c.u8(control_at(table, current()) + control::hot))) {
            regs.apply();
            invoke(c, 0x107267e0, back_leave, {control_at(table, current())});
        }
        if (b(pointer_over)) {
            const u32 g = control_at(table, current());
            if (c.u32(g + control::tracks) != 0 || c.i16(g + control::area) != -1) touch(c);
        }
    };
    // A text field (0x1071b520), called directly with the registers the original holds there.
    const auto edit = [&](u32 control_index, u32 back) {
        regs.apply();
        c.link(back);
        return text_field_key(c, table, control_index);
    };

    for (; half(pass) <= 2; ++pass, regs.hold({{29, pass}})) {
        // The controls in drawing order.
        u32 order_index = 0, order_offset = 0;
        for (; half(order_index) < c.i16(table + table::guio_limit); order_offset += 2, ++order_index) {
            order_at = order_offset + tbl::guio_order;
            regs.hold({{19, order_index}, {23, order_offset}, {22, order_at}});
            hover(sign16(c.i16(table + order_at)), true, 0x1071a350, 0x1071a37c);
            const u32 edited = sign16(c.i16(table + order_at));
            if (!(c.u32(control_at(table, edited) + control::flags) & control::f_edit)) continue;
            if (!b(edit(edited, 0x1071a3d8))) continue;
            // An edit consumed the event: let the other text fields refresh, then stop.
            u32 refresh_offset = sign16(half(order_index)) << 1;
            const u32 consumed = sign16(c.i16(table + refresh_offset + table::index_list));
            regs.hold({{21, list_base}, {22, refresh_offset}, {20, consumed}});
            for (; half(order_index) < c.i16(table + table::guio_limit); refresh_offset += 2, ++order_index) {
                regs.hold({{19, order_index}, {22, refresh_offset}});
                const u32 field = sign16(c.i16(table + refresh_offset + tbl::guio_order));
                if (c.u32(control_at(table, field) + control::flags) & control::f_edit) edit(field, 0x1071a424);
            }
            regs.hold({{19, order_index}, {22, refresh_offset}});
            if (half(pass) == 1) {
                u32 field_index = 0, field_offset = 0;
                regs.hold({{19, list_base}, {18, field_index}, {21, field_offset}});
                for (; half(field_index) < c.i16(table + table::guio_limit); field_offset += 2, ++field_index) {
                    regs.hold({{18, field_index}, {21, field_offset}});
                    const u32 field = sign16(c.i16(table + field_offset + tbl::guio_order));
                    if (c.u32(control_at(table, field) + control::flags) & control::f_edit) edit(field, 0x1071a484);
                }
            }
            return consumed;
        }
        regs.hold({{19, order_index}, {23, order_offset}});
        // The areas without a parent, and each one's entries.
        u32 area_index = 0, area_offset = 0;
        for (; half(area_index) < c.i16(list_base - 0x5704); area_offset += 2, ++area_index) {
            const u32 area_order_at = area_offset + tbl::area_order;
            regs.hold({{30, area_index}, {22, area_offset}, {19, area_order_at}});
            const u32 a = area_at(table, sign16(c.i16(table + area_order_at)));
            if (b(c.u8(a + area::enabled)) && c.i16(a + area::parent) == -1) {
                c.put16(a + area::cursor, c.u16(a + area::start));
                for (;;) {
                    const u32 entry = next_entry(c, table, area_order_at);
                    regs.hold({{23, entry}});
                    if (half(entry) == -1) break;
                    const u32 entry_row = control_at(table, sign16(half(entry))) - control::base;
                    regs.hold({{21, entry_row}});
                    if (!within_rows(c, table, area_order_at, sign16(half(entry)))) break;
                    const u32 entry_flags_base = entry_row + 0xc0000;
                    regs.hold({{20, entry_flags_base}});
                    hover(entry, false, 0x1071a580, 0x1071a5a0);
                    if (!(c.u32(entry_flags_base - 0x5790) & control::f_edit)) continue;
                    if (!b(edit(entry, 0x1071a5e4))) continue;
                    // An edit consumed the event: the following areas' text fields refresh.
                    u32 follow_offset = sign16(half(area_index)) << 1;
                    regs.hold({{19, list_base}, {24, follow_offset}});
                    for (; half(area_index) < c.i16(list_base - 0x5704); follow_offset += 2, ++area_index) {
                        const u32 follow_order_at = follow_offset + tbl::area_order;
                        regs.hold({{30, area_index}, {24, follow_offset}, {20, follow_order_at}});
                        const u32 followed = area_at(table, sign16(c.i16(table + follow_order_at)));
                        if (!(b(c.u8(followed + area::enabled)) && c.i16(followed + area::parent) == -1)) continue;
                        c.put16(followed + area::cursor, c.u16(followed + area::start));
                        for (;;) {
                            const u32 field = next_entry(c, table, follow_order_at);
                            if (half(field) == -1) break;
                            if (!within_rows(c, table, follow_order_at, sign16(half(field)))) break;
                            if (c.u32(control_at(table, sign16(half(field))) + control::flags) & control::f_edit)
                                edit(field, 0x1071a650);
                        }
                    }
                    // (The original repeats the loop for pass 1 at 0x1071a710; its condition is the one that
                    // just ended the loop above, with no call or store between, so it never runs.)
                    return entry;
                }
            }
            const u32 scrolled_area = sign16(c.i16(table + area_order_at));
            if (b(c.u8(area_at(table, scrolled_area) + area::has_scroll)) && half(pass) == 1) {
                regs.apply();
                c.link(0x1071a8f0);
                scroll_bar(c, table, scrolled_area);
            }
        }
        regs.hold({{30, area_index}, {22, area_offset}});
    }
    // The control the pointer is on.
    if (half(hovered) != -1) {
        const u32 hovered_row = sign16(half(hovered)) * control::size;
        const u32 hovered_base = table + 0xc0000 + hovered_row;
        regs.hold({{20, hovered_row}, {19, hovered_base}});
        const std::int16_t parent = c.i16(hovered_base - 0x5728);
        if (parent != -1) {
            regs.apply();
            invoke(c, 0x10713f70, 0x1071a94c, {area_at(table, sign16(parent)) + 4});
            c.put8(table + tbl::mode, 1);
            c.put16(table + tbl::pressed_area, c.u16(hovered_base - 0x5728));
            c.put8(success_ptr, 0);
            if (c.u32(hovered_base - 0x5794) == 0) hovered = none;
        }
    }
    if (half(hovered) != -1) {
        const u32 g = table + 0xc0000 + sign16(half(hovered)) * control::size;
        if (c.u32(g - 0x5790) & control::f_drag) {
            c.put8(table + tbl::mode, 3);
            c.put32(table + tbl::press_x, c.u32(c.toc(-0x5c10)));
            c.put32(table + tbl::press_y, c.u32(c.toc(-0x5c14)));
            c.put16(table + tbl::drag_control, hovered);
            hovered = none;
            for (u32 n = 0; n < 4; ++n) c.put32(table + tbl::press_rect + n * 4, c.u32(g - 0x578c + n * 4));
            c.put32(table + tbl::press_rect_end, 0);
        }
    }
    return hovered;
}

// 0x1071aa10: the pointer pass over the areas that have a parent: hover chains up through the parents, and
// the 300-tick delay before a control's child area opens or closes.
u32 pointer_nested(Cpu& c, u32 table) {
    const Frame frame(c, 0x1071aa10, 0x70, {.first_gpr = 21});
    CallRegisters regs(c);
    constexpr u32 none = 0xffffffffu;
    const u32 timer_ptr = c.toc(-0x5c18), success_ptr = c.toc(toc::success), list_base = table + tbl::high;
    u32 pass = 1, best = none;  // best: the control the pointer is on (r25)
    regs.hold({{21, list_base}, {22, table}, {23, pass}, {25, best}, {30, timer_ptr}, {31, success_ptr}});
    for (; half(pass) <= 2; ++pass, regs.hold({{23, pass}})) {
        u32 area_index = 0, area_offset = 0;
        for (; half(area_index) < c.i16(list_base - 0x5704); area_offset += 2, ++area_index) {
            const u32 area_order_at = area_offset + tbl::area_order;
            regs.hold({{24, area_index}, {28, area_offset}, {26, area_order_at}});
            const u32 a = area_at(table, sign16(c.i16(table + area_order_at)));
            if (!(b(c.u8(a + area::enabled)) && c.i16(a + area::parent) != -1)) continue;
            c.put16(a + area::cursor, c.u16(a + area::start));
            for (;;) {
                const u32 entry = next_entry(c, table, area_order_at);
                regs.hold({{29, entry}});
                if (half(entry) == -1) break;
                const u32 entry_row = control_at(table, sign16(half(entry))) - control::base;
                regs.hold({{27, entry_row}});
                if (!within_rows(c, table, area_order_at, sign16(half(entry)))) break;
                const u32 g = control_at(table, sign16(half(entry)));
                if (!(c.u32(g + control::flags) & control::f_hover)) continue;
                if (b(pointer_in(c, g))) {
                    if (!b(c.u8(g + control::hot))) {
                        const u32 own = area_at(table, sign16(c.i16(table + area_order_at)));
                        const u32 child = sign16(c.i16(own + area::child));
                        if (child != none && b(c.u8(control_at(table, child) + control::hot))) {
                            regs.apply();
                            invoke(c, 0x107267e0, 0x1071aaf8, {control_at(table, child)});
                        }
                        regs.apply();
                        invoke(c, 0x10726710, 0x1071ab04, {g});
                        const u32 owner = sign16(c.i16(area_at(table, sign16(c.i16(table + area_order_at))) + area::parent));
                        const u32 owner_control = control_at(table, owner);
                        if (!b(c.u8(owner_control + control::hot))) {
                            const u32 grand = area_at(table, sign16(c.i16(owner_control + control::owner_area)));
                            const u32 sibling = sign16(c.i16(grand + area::child));
                            if (sibling != none && b(c.u8(control_at(table, sibling) + control::hot))) {
                                regs.apply();
                                invoke(c, 0x107267e0, 0x1071ab6c, {control_at(table, sibling)});
                            }
                            const u32 again = sign16(c.i16(area_at(table, sign16(c.i16(table + area_order_at))) + area::parent));
                            regs.apply();
                            invoke(c, 0x10726710, 0x1071ab90, {control_at(table, again)});
                        }
                    }
                    if (b(c.u8(success_ptr))) {
                        best = entry;
                        regs.hold({{25, best}});
                    }
                    if (c.u32(g + control::tracks) != 0 || c.i16(g + control::area) != -1) touch(c);
                } else if (b(c.u8(g + control::hot)) && c.i16(g + control::area) == -1) {
                    regs.apply();
                    invoke(c, 0x107267e0, 0x1071abec, {g});
                }
                const u32 flags_base = entry_row + 0xc0000;  // r29 is reused for it
                regs.hold({{29, flags_base}});
                if (!b(c.u8(flags_base - 0x5622))) {
                    if (c.u32(timer_ptr) - c.u32(flags_base - 0x574c) >= 0x12c) {
                        const std::int16_t open = c.i16(flags_base - 0x5728);
                        if (open != -1) {
                            const u32 target = area_at(table, sign16(open));
                            if (b(c.u8(target + area::enabled)) && half(pass) == 1) {
                                regs.apply();
                                invoke(c, 0x10714000, 0x1071ac44, {target + 4});
                            }
                        }
                    }
                }
                if (b(c.u8(flags_base - 0x5622))) {
                    if (c.u32(timer_ptr) - c.u32(entry_row + 0xc0000 - 0x574c) >= 0x12c) {
                        const std::int16_t open = c.i16(entry_row + 0xc0000 - 0x5728);
                        if (open != -1) {
                            const u32 target = area_at(table, sign16(open));
                            if (!b(c.u8(target + area::enabled)) && half(pass) == 2) {
                                regs.apply();
                                invoke(c, 0x10713f70, 0x1071aca0, {target + 4});
                            }
                        }
                    }
                }
            }
        }
        regs.hold({{24, area_index}, {28, area_offset}});
    }
    const bool leave = (half(best) != -1 && c.i16(table + 0xc0000 + sign16(half(best)) * control::size - 0x5728) == -1) ||
                       b(c.u8(success_ptr));
    if (leave) {
        regs.apply();
        invoke(c, 0x10714000, 0x1071adac, {area_at(table, sign16(c.i16(list_base - 0x56d0))) + 4});
        c.put8(table + tbl::mode, 0);
        c.put8(success_ptr, 0);
    }
    return best;
}

// 0x1071b520: one text field's keyboard handling. `control_arg` is the control (a halfword). The field's text is
// edited in a 256-byte copy on the stack (frame + 0x160) that 0x107268c0 draws back; the caret is control + 0x44
// (-1: not editing). Returns 1 when the field's text changed on a path that ends the edit, else 0.
u32 text_field_key(Cpu& c, u32 table, u32 control_arg) {
    const Frame frame(c, 0x1071b520, 0x290, {.first_gpr = 21});
    CallRegisters regs(c);
    const u32 key_ptr = c.toc(-0x5c1c), char_ptr = c.toc(-0x74b0), success_ptr = c.toc(toc::success);
    const u32 message = c.toc(toc::message), pool = c.toc(toc::pool);
    const u32 slot = sign16(half(control_arg)) * control::size;
    const u32 flags_base = table + 0xc0000 + slot;  // control + 0x579c + flags: r24, and later r23, r29, r31
    const u32 buffer = frame.at(0x160);
    regs.hold({{21, table}, {22, message}, {23, control_arg}, {24, flags_base}, {25, slot}, {26, key_ptr}, {27, char_ptr},
               {28, pool}, {29, success_ptr}});
    if (!(c.u32(flags_base - 0x5790) & control::f_edit)) {
        regs.apply();
        report(c, frame, {0x1071b57c, 0x40, 0x38, 0x5e7});
        return 0;
    }
    const u32 control_row = table + slot;
    const u32 G = control_row + 0xc0000 - 0x579c;  // the control
    regs.hold({{22, control_row}, {30, G}});
    regs.apply();
    invoke(c, 0x1077dbf0, 0x1071b5d8, {buffer, G + control::text});
    regs.hold({{31, flags_base}});
    const Rect rect = rect_of(c, G);
    c.set(7, rect.bottom);  // left in r7 by the original's argument load
    const u32 inside = pointer_inside(c, rect.left, rect.top, rect.right, rect.bottom);
    if (b(inside)) touch(c);
    const u32 pressed = c.u8(success_ptr);
    // The text redrawn: the overlay the control keeps (+0x4c) is cleared by 0x1076b140 first.
    const auto overlay_only = [&](u32 back_overlay) {
        const u32 overlay = c.u32(G + control::overlay);
        if (overlay != 0) {
            regs.apply();
            invoke(c, 0x1076b140, back_overlay, {c.u32(G + control::left), c.u32(G + control::top), overlay});
        }
    };
    const auto refresh = [&](u32 back_overlay, u32 back_text) {
        overlay_only(back_overlay);
        regs.apply();
        invoke(c, 0x107268c0, back_text, {G, buffer, 1});
    };
    const auto clear_input = [&] {
        c.put32(key_ptr, 0);
        c.put8(char_ptr, 0);
    };
    const auto repaint = [&](u32 back_clear, u32 back_draw) {  // 0x10725ec0, then the control's rectangle
        regs.apply();
        invoke(c, 0x10725ec0, back_clear, {G, 0});
        const auto r = rect_of(c, G);
        regs.apply();
        invoke(c, 0x107692f0, back_draw, {r.left, r.top, r.right, r.bottom});
    };
    const auto changed_result = [&] { return b(c.u8(G + control::changed)) ? 1u : 0u; };

    if (b(pressed) && b(inside)) {
        regs.hold({{29, flags_base}});
        if (w(c.u32(G + control::caret)) == -1) {
            // The pointer entered an idle field: the caret goes to the end of the text.
            regs.apply();
            const u32 length = invoke(c, 0x1077dbd0, 0x1071b638, {buffer});
            c.put32(G + control::caret, length);
            c.put8(G + control::changed, 0);
            refresh(0x1071b664, 0x1071b674);
            clear_input();
            return 0;
        }
    }
    if (b(pressed) && !b(inside) && w(c.u32(G + control::caret)) > -1) {
        // The pointer left an editing field: the caret goes away.
        c.put32(G + control::caret, 0xffffffffu);
        refresh(0x1071b6cc, 0x1071b6dc);
        clear_input();
        return changed_result();
    }
    const u32 key = c.u32(key_ptr);
    if ((w(key) == 0x30 || w(key) == 0x24) && w(c.u32(G + control::caret)) > -1) {
        c.put32(G + control::caret, 0xffffffffu);
        overlay_only(0x1071b754);
        repaint(0x1071b760, 0x1071b774);
        if (!(c.u32(flags_base - 0x5790) & control::f_keep_focus)) c.put16(table + tbl::text_focus, control_arg);
        clear_input();
        regs.apply();
        invoke(c, 0x107268c0, 0x1071b7a4, {G, buffer, 1});
        return changed_result();
    }
    const std::int16_t focus = c.i16(table + tbl::text_focus);
    if (focus != -1 && half(control_arg) != focus) {
        regs.hold({{23, flags_base}});
        if (w(c.u32(G + control::caret)) == -1) {
            regs.apply();
            const u32 length = invoke(c, 0x1077dbd0, 0x1071b800, {buffer});
            c.put32(G + control::caret, length);
            c.put8(G + control::changed, 0);
            refresh(0x1071b82c, 0x1071b83c);
            c.put16(table + tbl::text_focus, 0xffff);
            clear_input();
            return 0;
        }
    }
    u32 caret = c.u32(G + control::caret);
    regs.hold({{23, flags_base}, {29, caret}});
    if (w(caret) > -1 && key != 0 && w(c.u32(G + control::max_length)) < 0) {
        // The first key in a field whose limit is negative: the field is cleared.
        regs.apply();
        invoke(c, 0x107268c0, 0x1071b894, {G, pool + 0x43, 0});
        c.put32(G + control::caret, 0);
        c.put8(G + control::changed, 1);
        regs.apply();
        const u32 limit = invoke(c, 0x10775550, 0x1071b8b4, {c.u32(G + control::max_length)});
        c.put32(G + control::max_length, limit);
        overlay_only(0x1071b8d8);
        repaint(0x1071b8e4, 0x1071b8f8);
        return 0;
    }
    if (w(caret) > 0 && w(key) == 0x7b) {  // left arrow
        c.put32(G + control::caret, caret - 1);
        refresh(0x1071b938, 0x1071b948);
        clear_input();
        return 0;
    }
    if (w(caret) > -1) {  // right arrow
        regs.apply();
        const u32 length = invoke(c, 0x1077dbd0, 0x1071b96c, {buffer});
        if (w(caret) < w(length) && w(c.u32(key_ptr)) == 0x7c) {
            c.put32(G + control::caret, c.u32(G + control::caret) + 1);
            refresh(0x1071b9ac, 0x1071b9bc);
            clear_input();
            return 0;
        }
    }
    caret = c.u32(G + control::caret);
    regs.hold({{29, caret}});
    if (w(caret) > 0 && w(c.u32(key_ptr)) == 0x33) {  // backspace: move the tail one byte left
        u32 shift_index = caret;
        for (;;) {
            regs.hold({{22, shift_index}});
            regs.apply();
            const u32 length = invoke(c, 0x1077dbd0, 0x1071ba0c, {buffer});
            if (!(w(shift_index) <= w(length))) break;
            c.put8(buffer + shift_index - 1, c.u8(buffer + shift_index));
            ++shift_index;
        }
        c.put32(G + control::caret, c.u32(G + control::caret) - 1);
        c.put8(G + control::changed, 1);
        refresh(0x1071ba48, 0x1071ba58);
        clear_input();
        return 0;
    }
    if (w(caret) > -1) {  // forward delete
        regs.apply();
        const u32 length = invoke(c, 0x1077dbd0, 0x1071ba7c, {buffer});
        if (w(caret) < w(length) && w(c.u32(key_ptr)) == 0x75) {
            u32 shift_index = c.u32(G + control::caret);
            for (;;) {
                regs.hold({{22, shift_index}});
                regs.apply();
                const u32 length_now = invoke(c, 0x1077dbd0, 0x1071bab4, {buffer});
                if (!(w(shift_index) <= w(length_now - 1))) break;
                c.put8(buffer + shift_index, c.u8(buffer + shift_index + 1));
                ++shift_index;
            }
            c.put8(G + control::changed, 1);
            refresh(0x1071bae8, 0x1071baf8);
            clear_input();
            return 0;
        }
    }
    // A typed character.
    if (w(c.u32(G + control::caret)) <= -1) return 0;
    const u32 limit = c.u32(G + control::max_length);
    regs.hold({{29, limit}});
    regs.apply();
    if (w(invoke(c, 0x1077dbd0, 0x1071bb2c, {buffer})) >= w(limit)) return 0;
    const u32 ch = c.u8(char_ptr);
    if (ch < 0x20) return 0;
    const std::int8_t s = std::int8_t(ch);
    const u32 flags = c.u32(flags_base - 0x5790);
    if (s == 0x22 || s == 0x2a || s == 0x2f || (s == 0x3a && !(flags & control::f_path)) || s == 0x3c || s == 0x3e ||
        (s == 0x3f && !(flags & control::f_wildcard)) || (s == 0x5c && !(flags & control::f_path)) || s == 0x7c ||
        s == 0x7b || s == 0x7d || s == 0x5b || s == 0x5d || s == 0x7e)
        return 0;
    regs.apply();
    const u32 length = invoke(c, 0x1077dbd0, 0x1071bbec, {buffer});
    regs.hold({{29, length}});
    const u32 text = frame.at(0x60);
    regs.apply();
    invoke(c, 0x1077b720, 0x1071bc08, {text, pool + 0x44, buffer, sign8(c.u8(char_ptr)),
                                       rect.bottom /* r7: the pointer test 0x1071b5f0 set it; the strlen calls since leave r7 alone (the trace compares it) */});
    regs.apply();
    const u32 width = invoke(c, 0x1076d1e0, 0x1071bc1c, {sign16(c.i16(G + control::font)), text});
    const u32 span = c.u32(G + control::right) - c.u32(G + control::left);
    if (w(width) <= w(span - 10)) {
        if (w(c.u32(G + control::caret)) < w(length)) {
            // Make room: shift the tail one byte right.
            for (u32 at = length - 1; w(at) >= w(c.u32(G + control::caret)); --at) c.put8(buffer + at + 1, c.u8(buffer + at));
        }
        const u32 typed = c.u8(char_ptr);
        c.put8(buffer + c.u32(G + control::caret), typed);
        c.put8(buffer + 1 + length, 0);
        c.put32(G + control::caret, c.u32(G + control::caret) + 1);
        c.put8(G + control::changed, 1);
        c.set(7, typed);  // r7 holds the typed character (0x1071bc6c) up to the redraw
        refresh(0x1071bcbc, 0x1071bccc);
    }
    clear_input();
    return 0;
}

// 0x1071bd00: one area's scroll bar. `area_arg` is the area (a halfword). The pointer on an arrow or the track moves
// the position (control + 0x1c of the bar), clamped to [0, steps], and the thumb is recomputed and redrawn.
u32 scroll_bar(Cpu& c, u32 table, u32 area_arg) {
    const Frame frame(c, 0x1071bd00, 0xc0, {.first_gpr = 24});
    CallRegisters regs(c);
    const u32 success_ptr = c.toc(toc::success), message = c.toc(toc::message), pool = c.toc(toc::pool);
    const u32 offset = sign16(half(area_arg)) * 0xbf0;                  
    const u32 area_plus_4 = table + offset + 4;
    const u32 S = area_plus_4 + area::scroll - 4; // the scroll bar (area + 0xb80)
    regs.hold({{24, message}, {25, pool}, {26, table}, {27, area_arg}, {28, area_plus_4}, {29, offset}, {30, S}, {31, success_ptr}});
    regs.apply();
    if (S == 0) {
        report(c, frame, {0x1071bd50, 0x40, 0x38, 0x717});
        return 0;
    }
    const u32 copy = frame.at(0x60);
    invoke(c, 0x10739ef0, 0x1071bda4, {copy, S, 0x34});
    c.put8(S + bar::hot, 0);
    const auto word = [&](u32 field) { return c.u32(S + field); };
    // `v` clamped below at 0 the way the original does it (neg, andc, srawi, and).
    const auto at_least_zero = [](u32 v) { return v & u32(std::int32_t((0u - v) & ~v) >> 31); };
    const auto step = [&] { return u32(c.u8(area_at(table, sign16(half(area_arg))) + area::step)); };
    const auto mouse_in = [&](u32 left, u32 top, u32 right, u32 bottom) {
        return b(c.u8(success_ptr)) && b(pointer_inside(c, left, top, right, bottom));
    };
    if (mouse_in(word(bar::left), word(bar::top), word(bar::right), word(bar::track_top) - 1)) {
        c.put32(S + bar::position, at_least_zero(word(bar::position) - 1));  // arrow up
        c.put8(S + bar::hot, 1);
    } else if (mouse_in(word(bar::left), word(bar::track_bottom) + 1, word(bar::right), word(bar::bottom))) {
        const u32 v = word(bar::position) + 1;  // arrow down
        c.put32(S + bar::position, w(v) < w(word(bar::steps)) ? v : word(bar::steps));
        c.put8(S + bar::hot, 2);
    } else if (mouse_in(word(bar::left), word(bar::track_top), word(bar::right), word(bar::thumb_top) - 1)) {
        c.put32(S + bar::position, at_least_zero(word(bar::position) - step()));  // page up
    } else if (mouse_in(word(bar::left), word(bar::thumb_bottom) + 1, word(bar::right), word(bar::track_bottom))) {
        const u32 v = word(bar::position) + step();  // page down
        c.put32(S + bar::position, w(v) < w(word(bar::steps)) ? v : word(bar::steps));
    } else if (b(c.u8(success_ptr)) && b(pointer_inside(c, word(bar::left), word(bar::thumb_top), word(bar::right), word(bar::thumb_bottom)))) {
        // The thumb: start a scroll-bar drag.
        c.put8(table + tbl::mode, 2);
        c.put32(table + tbl::press_y, c.u32(c.toc(-0x5c14)));
        c.put16(table + tbl::pressed_scroll_area, area_arg);
    }
    const u32 position = word(bar::position);
    const u32 previous = c.u32(frame.at(0x7c));
    const auto redraw = [&] {
        if (word(bar::image) != 0) {
            invoke(c, 0x1076b140, 0x1071bfbc, {word(bar::left), word(bar::top), word(bar::image)});
            invoke(c, 0x1076b7e0, 0x1071bfc4, {word(bar::image)});
            c.put32(S + bar::image, 0);
        }
        invoke(c, 0x10715110, 0x1071bfd4, {area_plus_4});
        invoke(c, 0x107692f0, 0x1071bfe8, {word(bar::left), word(bar::top), word(bar::right), word(bar::bottom)});
    };
    if (position != previous) {
        const u32 span = word(bar::track_bottom) - word(bar::track_top) + 1 - word(bar::thumb);
        c.put32(S + bar::thumb_top, word(bar::track_top) + u32(divide_signed<4>(position * span, word(bar::steps))));
        c.put32(S + bar::thumb_bottom, word(bar::thumb_top) + word(bar::thumb) - 1);
        redraw();
        invoke(c, 0x10715340, 0x1071bffc, {area_plus_4, word(bar::position) - previous});
        return 1;
    }
    if (c.u8(S + bar::hot) != c.u8(frame.at(0x90))) {
        if (word(bar::image) != 0) {
            invoke(c, 0x1076b140, 0x1071c02c, {word(bar::left), word(bar::top), word(bar::image)});
            invoke(c, 0x1076b7e0, 0x1071c034, {word(bar::image)});
            c.put32(S + bar::image, 0);
        }
        invoke(c, 0x10715110, 0x1071c044, {area_plus_4});
        invoke(c, 0x107692f0, 0x1071c058, {word(bar::left), word(bar::top), word(bar::right), word(bar::bottom)});
    }
    return 0;
}
}
