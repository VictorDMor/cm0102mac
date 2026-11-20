// DISPLAY.CPP block "drawing" (1/2): 0x10718f60 paint_first, 0x10718fc0 draw_screen, 0x10719350
// screen_step, 0x10725770 update_blinking. The mouse drags (0x1071add0, 0x1071afc0) are in
// display_drawing_drag.cpp. See display_drawing.hpp.
//
// Calls through `invoke` reach other modules' routines (generated or C++); a routine of this block calls
// its siblings directly after `link`. The registers r4-r10 are left alone wherever the original leaves them:
// callers (0x10725770's formatter, 0x10719350's callers) read the volatile registers their callee left.
#include "competition_editor_screen.hpp"
#include "display_drawing.hpp"
#include "adaptive_gui.hpp"
#include <cstdint>

namespace cm::aot::display {
namespace {
using u32 = std::uint32_t;
namespace f = drawing::field;
namespace g = drawing::guio;

// The routines of other modules this block calls.
namespace callee {
constexpr u32 title = 0x1073edb0, copy = 0x1077dbf0, compare = 0x107393f0, data_path = 0x10746280, tick = 0x10746a30,
    area_draw = 0x10714110, area_erase = 0x10714630, guio_draw = 0x10725ec0, guio_text = 0x107268c0,
    flush = 0x107692f0, clear_rect = 0x10769460, fill = 0x10769980, make_image = 0x1076b020,
    restore_bits = 0x1076b140, load_image = 0x1076b2f0, free_image = 0x1076b7e0, end_input = 0x1076e920,
    format = 0x1077b720, text_matches = 0x1077dc90, area_input = 0x1071a280, guio_input = 0x1071aa10;
}
// The data the unit keeps at fixed offsets from its table of contents (r2).
namespace global {
constexpr std::int32_t blink_slow_time = -0x35b0, blink_slow_set = -0x35b4, blink_fast_time = -0x35b8,
    blink_fast_set = -0x35b9, released_bits = -0x35ba;
}
u32 global_at(const Cpu& c, std::int32_t offset) { return c.r(2) + u32(offset); }

}

void paint_first(Cpu& c, u32 table) {
    const Frame frame(c, 0x10718f60, 0x40, {});
    if (c.i8(table + f::painted_once) == 0) {
        c.put8(table + f::painted_once, 1);
        // 0x10718f8c, 0x10718fa0: the screen's last column and row.
        invoke(c, callee::fill, 0x10718f98, {0, 0, u32(screen_right(c)), u32(screen_bottom(c))});
        invoke(c, callee::flush, 0x10718fac, {0, 0, u32(screen_right(c)), u32(screen_bottom(c))});
    }
}

u32 draw_screen(Cpu& c, u32 table, u32 left_raw, u32 top_raw, u32 right_raw, u32 bottom_raw) {
    const Frame frame(c, 0x10718fc0, 0x60, {.first_gpr = 23});
    const u32 game = c.toc(toc::game), pool = c.toc(toc::pool);
    CallRegisters regs(c);
    regs.hold({{30, game}, {24, table}, {31, pool}, {25, left_raw}, {26, top_raw}, {27, right_raw}, {28, bottom_raw}});
    const auto call = [&](u32 entry, u32 back, std::initializer_list<u32> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const u32 title = call(callee::title, 0x10718ff4, {game});
    regs.hold({{29, title}});
    if (c.i8(table + f::skip_draw) != 0) return 1;

    const bool named_game = c.i8(game + 0x25819c) != 0;
    c.put8(c.toc(-0x74b0), 0);
    if (named_game && c.i8(title) != 0) call(callee::copy, 0x10719040, {title, pool + 0x1f});

    if (call(callee::compare, 0x10719050, {table + f::background_name, title}) != 0) {
        if (c.i8(title) == 0) {
            // 0x10719074, 0x10719078: the screen's last column and row.
            const u32 image = call(callee::make_image, 0x10719080,
                                   {0, 0, u32(screen_right(c)), u32(screen_bottom(c)), c.u32(table + f::background)});
            c.put32(table + f::background, image);
            c.put8(table + f::background_name, 0);
        } else {
            call(callee::copy, 0x107190a4, {table + f::background_name, title});
            u32 image = call(callee::load_image, 0x107190b4, {table + f::background_name, c.u32(table + f::background)});
            regs.hold({{23, image}});
            if (image == 0) {
                const u32 other = call(callee::data_path, 0x107190c8, {pool + 0x21, pool + 0x29});
                image = call(callee::load_image, 0x107190d4, {other, c.u32(table + f::background)});
                regs.hold({{23, image}});
            }
            const u32 old = c.u32(table + f::background);
            if (image != 0 && image != old) {
                if (old != 0) call(callee::free_image, 0x107190fc, {old});
                c.put32(table + f::background, image);
            }
        }
    }
    if (const u32 image = c.u32(table + f::background); image != 0)
        call(callee::restore_bits, 0x10719120, {0, 0, image});

    // 0x10719120: the adaptive interface paints its background (it reads r24, the table) before the loops.
    regs.apply();
    draw_adaptive_background(c.s);
    competition_editor_backdrop(c.s, table);
    regs.hold({{29, table + 0x130000}});

    // The original walks each order list with r23 as the index and r31 as its byte offset.
    const auto each = [&](u32 count_at, const auto& body) {
        u32 i = 0;
        regs.hold({{23, 0}, {31, 0}});
        for (; std::int16_t(i) < c.i16(table + count_at); ++i) {
            regs.hold({{23, i}, {31, 2 * i}});
            body(i);
        }
        regs.hold({{23, i}, {31, 2 * i}});
    };
    const auto area_in_order = [&](u32 i) { return area_at(table, c.i16(table + f::area_order + 2 * i)); };
    const auto draw_listed_areas = [&](bool without_parent) {
        each(f::area_order_count, [&](u32 i) {
            const u32 row = area_in_order(i);
            if (c.i8(row + 0xbf2) != 0 && (c.i16(row + 0x212) == -1) == without_parent)
                call(callee::area_draw, without_parent ? 0x107191b8 : 0x10719264, {row + 4, 1});
        });
    };

    if (c.i8(table + f::drawn) == 0)
        each(f::area_order_count, [&](u32 i) { call(callee::area_erase, 0x10719158, {area_in_order(i) + 4}); });
    draw_listed_areas(true);
    each(f::guio_order_count, [&](u32 i) {
        call(callee::guio_draw, 0x10719204, {guio_at(table, c.i16(table + f::guio_order + 2 * i)), 1});
    });
    draw_listed_areas(false);

    // 0x1071929c, 0x107192a0, 0x107192c8, 0x107192cc, 0x10719318, 0x1071931c: the screen's last column and row.
    if (c.i8(table + f::painted_once) != 0 && c.i8(game + 0x2c61) != 0)
        call(callee::fill, 0x107192a8, {0, 0, u32(screen_right(c)), u32(screen_bottom(c))});
    if (c.i8(table + f::clear_flag) != 0) {
        c.put8(table + f::clear_flag, 0);
        call(callee::clear_rect, 0x107192d8, {0, 0, u32(screen_right(c)), u32(screen_bottom(c)), 0x14});
    }
    const u32 left = sign16(left_raw), top = sign16(top_raw), right = sign16(right_raw), bottom = sign16(bottom_raw);
    constexpr u32 whole = 0xffffffffu;
    if (left == whole || right == whole || top == whole || bottom == whole)
        call(callee::flush, 0x10719324, {0, 0, u32(screen_right(c)), u32(screen_bottom(c))});
    else
        call(callee::flush, 0x1071930c, {left, top, right, bottom});
    c.put8(table + f::drawn, 1);
    return 1;
}

u32 screen_step(Cpu& c, u32 table) {
    const Frame frame(c, 0x10719350, 0x80, {.first_gpr = 26});
    const u32 pool = c.toc(toc::pool);
    CallRegisters regs(c);
    const u32 enter_flag = c.toc(-0x5c00), cancel_flag = c.toc(-0x744c), key_flag = c.toc(-0x74b0);
    regs.hold({{27, table}, {28, enter_flag}, {29, cancel_flag}, {26, key_flag}, {30, c.toc(toc::message)}, {31, pool}});
    const auto call = [&](u32 entry, u32 back, std::initializer_list<u32> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    c.put8(global_at(c, global::released_bits), 0);

    // A key the table was waiting for (0x12a901) has been pressed: answer with its result.
    if (c.i16(table + f::key_result) != -1 && c.i8(table + f::key_expected) != 0 &&
        c.i8(key_flag) == c.i8(table + f::key_expected)) {
        call(callee::end_input, 0x107193b8, {0});
        c.put8(key_flag, 0);
        return u32(std::int32_t(c.i16(table + f::key_result)));
    }
    if (c.i8(cancel_flag) != 0) {
        if (c.i16(table + f::cancel_result) != -1) {
            call(callee::end_input, 0x107193f8, {0});
            c.put8(cancel_flag, 0);
            const u32 result = u32(std::int32_t(c.i16(table + f::cancel_result)));
            c.put16(table + f::cancel_result, 0xffff);
            return result;
        }
        c.put8(cancel_flag, 0);
    }
    if (c.i8(enter_flag) != 0) {
        if (c.i16(table + f::enter_result) != -1) {
            call(callee::end_input, 0x10719440, {0});
            c.put8(enter_flag, 0);
            const u32 result = u32(std::int32_t(c.i16(table + f::enter_result)));
            c.put16(table + f::enter_result, 0xffff);
            return result;
        }
        c.put8(enter_flag, 0);
    }

    regs.apply();
    c.set(3, table);
    c.link(0x1071946c);
    update_blinking(c, table);

    u32 result;
    switch (c.u8(table)) {  // the table's mode
    case 0: result = call(callee::area_input, 0x107194a0, {table}); break;
    case 1: result = call(callee::guio_input, 0x107194b0, {table}); break;
    case 2:
        regs.apply();
        c.set(3, table);
        c.link(0x107194c0);
        result = scroll_bar_drag(c, table);
        break;
    case 3:
        regs.apply();
        c.set(3, table);
        c.link(0x107194d0);
        result = area_drag(c, table);
        break;
    default:  // a mode above 3: the assertion at 0x107194ec (DISPLAY.CPP line 0x1fe), result 0
        regs.apply();
        report(c, frame, {0x107194ec, 0x40, 0x38, 0x1fe});
        return 0;
    }
    regs.hold({{26, result}});
    call(callee::end_input, 0x1071953c, {c.u8(global_at(c, global::released_bits))});
    return result;
}

void update_blinking(Cpu& c, u32 table) {
    const Frame frame(c, 0x10725770, 0x170, {.first_gpr = 20});
    const u32 main_object = c.toc(toc::game), pool = c.toc(toc::pool);
    CallRegisters regs(c);
    regs.hold({{30, main_object}, {23, table}, {31, pool}});
    const auto call = [&](u32 entry, u32 back, std::initializer_list<u32> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const u32 slow_time = global_at(c, global::blink_slow_time), slow_set = global_at(c, global::blink_slow_set),
              fast_time = global_at(c, global::blink_fast_time), fast_set = global_at(c, global::blink_fast_set);
    if (c.i8(slow_set) == 0) { c.put32(slow_time, 0); c.put8(slow_set, 1); }
    if (c.i8(fast_set) == 0) { c.put32(fast_time, 0); c.put8(fast_set, 1); }
    const u32 now = call(callee::tick, 0x107257c8, {});
    // 1000 ms and 100 ms windows (cmplw: unsigned, the sum wraps).
    const u32 slow_start = c.u32(slow_time);
    const bool slow = now < slow_start || !(now < slow_start + 1000);
    if (slow) c.put32(slow_time, now);
    const u32 fast_start = c.u32(fast_time);
    const bool fast = now < fast_start || !(now < fast_start + 100);
    if (fast) c.put32(fast_time, now);
    regs.hold({{26, slow}, {25, fast}});
    if (!slow && !fast) return;

    regs.hold({{22, table + 0x130000}});
    u32 i = 0, offset = 0;
    regs.hold({{27, 0}, {28, 0}});
    for (; std::int32_t(i) < c.i16(table + f::guio_count); ++i, offset += drawing::guio_size) {
        const u32 control = offset + drawing::guio_base + table;
        const u32 flags = c.u32(control + g::flags);
        regs.hold({{27, i}, {28, offset}, {24, control}, {29, flags}});
        if ((flags & 0x800) != 0 && slow) {
            // The text of a control ends at a 0 or a 1 byte; a 1 is turned into 0 and the text continues after it.
            const u32 text = control + 0x78;
            u32 length = 0;
            while (c.u8(text + length) != 1 && c.i8(text + length) != 0) ++length;
            if (c.u8(text + length) == 1) {
                c.put8(text + length, 0);
                ++length;
            }
            const u32 rest = text + length;
            regs.hold({{21, text}, {20, rest}});
            call(callee::format, 0x107258bc, {frame.at(0x38), pool + 0x13d, rest, 1, text});
            if (call(callee::text_matches, 0x107258c8, {text, rest}) != 0) {
                call(callee::guio_text, 0x107258e0, {control, frame.at(0x38), 0});
                const u32 left = sign16(c.u32(control + g::left)), top = sign16(c.u32(control + g::top)),
                          right = sign16(c.u32(control + g::right)), bottom = sign16(c.u32(control + g::bottom));
                regs.apply();
                c.set({{3, table}, {4, left}, {5, top}, {6, right}, {7, bottom}});
                c.link(0x10725908);
                draw_screen(c, table, left, top, right, bottom);
            }
        }
        if ((flags & 0x1000) != 0 && fast) {
            // Swap the control's two images.
            const u32 first = c.u16(control + 0x6a);
            c.put16(control + 0x6a, c.u16(control + 0x70));
            c.put16(control + 0x70, first);
            const u32 human_table = c.u32(main_object + u32(std::int32_t(c.i16(main_object + game::current_human))) * game::human_size + game::current_table);
            if (c.i8(human_table + f::drawn) != 0) {
                const u32 left = sign16(c.u32(control + g::left)), top = sign16(c.u32(control + g::top)),
                          right = sign16(c.u32(control + g::right)), bottom = sign16(c.u32(control + g::bottom));
                regs.apply();
                c.set({{3, table}, {4, left}, {5, top}, {6, right}, {7, bottom}});
                c.link(0x10725970);
                draw_screen(c, table, left, top, right, bottom);
            }
        }
    }
}
}
