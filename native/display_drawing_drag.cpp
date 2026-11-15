// DISPLAY.CPP block "drawing" (2/2): the mouse drags of the table's modes 2 and 3 (0x1071add0
// scroll_bar_drag, 0x1071afc0 area_drag). See display_drawing.hpp.
#include "display_drawing.hpp"
#include "adaptive_gui.hpp"
#include "aot_runtime.hpp"
#include <array>
#include <cstdint>

namespace cm::aot::display {
namespace {
using u32 = std::uint32_t;
namespace f = drawing::field;
namespace g = drawing::guio;

namespace callee {
constexpr u32 scroll_refresh = 0x10715110, scroll_moved = 0x10715340, flush = 0x107692f0, restore_bits = 0x1076b140,
    make_image = 0x1076b020, free_image = 0x1076b7e0, draw_plain = 0x1076a580, draw_frame = 0x1076ad50,
    block_move = 0x1077fc28;
}
// The mouse state the unit keeps behind TOC words: pointers to the current x and y, and to the
// "button is down" bytes (one for controls that track it themselves, one shared).
namespace pointer {
constexpr std::int32_t y_word = -0x5c14, x_word = -0x5c10, button_gui = -0x5c0c, button_shared = -0x7bac, colour = -0x7a74;
}
// A scroll bar record inside an area row (row + 0xb7c): the bounds, the position and the saved image.
namespace bar {
constexpr u32 offset = 0xb80, left = 0x0, top = 0x4, right = 0x8, bottom = 0xc, min = 0x10, max = 0x14, span = 0x18,
    thumb = 0x1c, thumb_size = 0x20, position = 0x24, end = 0x28, saved_bits = 0x2c;
}
}

u32 scroll_bar_drag(Cpu& c, u32 table) {
    const Frame frame(c, 0x1071add0, 0x80, {.first_gpr = 26});
    const u32 pool = c.toc(toc::pool);
    const u32 success = c.toc(toc::success), message = c.toc(toc::message);
    CallRegisters regs(c);
    regs.hold({{29, table}, {26, success}, {27, message}, {28, pool}});
    const auto call = [&](u32 entry, u32 back, std::initializer_list<u32> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const bool pressed = c.i8(success) != 0;  // the shared button/success byte (TOC -0x7bac)
    const u32 y = c.u32(c.toc(pointer::y_word));  // the shared button byte is TOC -0x7bac
    regs.hold({{30, y}});
    std::int32_t move = std::int32_t(y - c.u32(table + f::last_y));
    if (!pressed || move == 0) {
        if (!pressed) c.put8(table, 0);
        return 0xffffffffu;
    }
    const u32 record = area_at(table, c.i16(table + f::drag_area)) + bar::offset;
    regs.hold({{31, record}});
    if (record == 0) {  // unreachable from a real state: a null row is a wrapped address
        regs.apply();
        report(c, frame, {0x1071ae40, 0x40, 0x38, 0x4da});
        return 0;
    }
    // Keep the bar inside [min, max].
    const std::int32_t position = c.i32(record + bar::position), minimum = c.i32(record + bar::min);
    if (std::int32_t(u32(position) + u32(move)) < minimum) move = std::int32_t(u32(minimum) - u32(position));
    const std::int32_t end = c.i32(record + bar::end), maximum = c.i32(record + bar::max);
    if (std::int32_t(u32(end) + u32(move)) > maximum) move = std::int32_t(u32(maximum) - u32(end));
    if (move != 0) {
        c.put32(record + bar::position, c.u32(record + bar::position) + u32(move));
        c.put32(record + bar::end, c.u32(record + bar::end) + u32(move));
        if (const u32 saved = c.u32(record + bar::saved_bits); saved != 0) {
            call(callee::restore_bits, 0x1071aeec, {c.u32(record + bar::left), c.u32(record + bar::top), saved});
            call(callee::free_image, 0x1071aef4, {c.u32(record + bar::saved_bits)});
            c.put32(record + bar::saved_bits, 0);
        }
        call(callee::scroll_refresh, 0x1071af14, {area_at(table, c.i16(table + f::drag_area)) + 4});
        call(callee::flush, 0x1071af28,
             {c.u32(record + bar::left), c.u32(record + bar::top), c.u32(record + bar::right), c.u32(record + bar::bottom)});
        const std::int32_t low = c.i32(record + bar::min), old_thumb = c.i32(record + bar::thumb);
        const std::int32_t scaled = std::int32_t(c.u32(record + bar::span) * (c.u32(record + bar::position) - u32(low)));
        const std::int32_t length = std::int32_t(((c.u32(record + bar::max) - u32(low)) + 1) - c.u32(record + bar::thumb_size));
        // divw as the generated code does it (a zero divisor stops the run there).
        const u32 thumb = u32(divide_signed<4>(u32(scaled), u32(length)));
        c.put32(record + bar::thumb, thumb);
        const std::int32_t new_thumb = c.i32(record + bar::thumb);
        if (new_thumb != old_thumb)
            call(callee::scroll_moved, 0x1071af84,
                 {area_at(table, c.i16(table + f::drag_area)) + 4, u32(new_thumb) - u32(old_thumb)});
    }
    c.put32(table + f::last_y, y);
    return 0xffffffffu;
}

u32 area_drag(Cpu& c, u32 table) {
    const Frame frame(c, 0x1071afc0, 0x60, {.first_gpr = 25});
    CallRegisters regs(c);
    const u32 colour = c.toc(pointer::colour);
    regs.hold({{26, table}, {31, colour}});
    const auto call = [&](u32 entry, u32 back, std::initializer_list<u32> arguments) {
        regs.apply();
        return invoke(c, entry, back, arguments);
    };
    const u32 x = c.u32(c.toc(pointer::x_word)), y = c.u32(c.toc(pointer::y_word));
    regs.hold({{29, x}, {28, y}});
    std::int32_t dx = std::int32_t(x - c.u32(table + f::last_x)), dy = std::int32_t(y - c.u32(table + f::last_y));
    regs.hold({{27, u32(dx)}, {30, u32(dy)}});
    const auto current_guio = [&]() { return guio_at(table, c.i16(table + f::drag_guio)); };
    const bool down = (c.u32(current_guio() + g::flags) & 0x100) != 0 ? c.i8(c.toc(pointer::button_gui)) != 0
                                                                      : c.i8(c.toc(pointer::button_shared)) != 0;
    const auto rectangle = [&]() {
        return std::array<u32, 4>{c.u32(table + f::left), c.u32(table + f::top), c.u32(table + f::right),
                                  c.u32(table + f::bottom)};
    };
    // Frees the saved image under the dragged rectangle and flushes the rectangle (the same four calls
    // as the original at 0x1071b0dc and 0x1071b204).
    const auto drop_saved_bits = [&](u32 saved, u32 left, u32 top, u32 back_restore, u32 back_flush, u32 back_free) {
        call(callee::restore_bits, back_restore, {left, top, saved});
        const auto box = rectangle();
        call(callee::flush, back_flush, {box[0], box[1], box[2], box[3]});
        call(callee::free_image, back_free, {c.u32(table + f::saved_bits)});
        c.put32(table + f::saved_bits, 0);
    };

    if (down) {
        if (dy == 0 && dx == 0 && c.u32(table + f::saved_bits) != 0) return 0xffffffffu;
        // Keep the rectangle on the screen. 0x1071b074/0x1071b0a0 compare with the screen's last column
        // and row, 0x1071b07c/0x1071b0a8 subtract from them.
        const std::int32_t left = c.i32(table + f::left);
        if (std::int32_t(u32(left) + u32(dx)) < 0) dx = std::int32_t(0u - u32(left));
        const std::int32_t right = c.i32(table + f::right);
        if (std::int32_t(u32(right) + u32(dx)) > screen_right(c)) dx = std::int32_t(u32(screen_right(c)) - u32(right));
        const std::int32_t top = c.i32(table + f::top);
        if (std::int32_t(u32(top) + u32(dy)) < 0) dy = std::int32_t(0u - u32(top));
        const std::int32_t bottom = c.i32(table + f::bottom);
        if (std::int32_t(u32(bottom) + u32(dy)) > screen_bottom(c)) dy = std::int32_t(u32(screen_bottom(c)) - u32(bottom));
        regs.hold({{27, u32(dx)}, {30, u32(dy)}});
        if (dx != 0 || dy != 0 || c.u32(table + f::saved_bits) == 0) {
            if (const u32 saved = c.u32(table + f::saved_bits); saved != 0)
                drop_saved_bits(saved, u32(left), u32(top), 0x1071b0e0, 0x1071b0f8, 0x1071b104);
            c.put32(table + f::left, c.u32(table + f::left) + u32(dx));
            c.put32(table + f::top, c.u32(table + f::top) + u32(dy));
            c.put32(table + f::right, c.u32(table + f::right) + u32(dx));
            c.put32(table + f::bottom, c.u32(table + f::bottom) + u32(dy));
            auto box = rectangle();
            c.put32(table + f::saved_bits, call(callee::make_image, 0x1071b15c, {box[0], box[1], box[2], box[3], 0}));
            const u32 flags2 = c.u32(current_guio() + g::flags2);
            box = rectangle();
            const u32 tint = c.u16(colour);
            call((flags2 & 0x10000) != 0 ? callee::draw_frame : callee::draw_plain,
                 (flags2 & 0x10000) != 0 ? 0x1071b19c : 0x1071b1bc, {box[0], box[1], box[2], box[3], 1, tint});
            box = rectangle();
            call(callee::flush, 0x1071b1d4, {box[0], box[1], box[2], box[3]});
        }
        c.put32(table + f::last_x, x);
        c.put32(table + f::last_y, y);
        return 0xffffffffu;
    }

    // The button is up: release the dragged rectangle and choose the controls and list entries under the mouse.
    if (const u32 saved = c.u32(table + f::saved_bits); saved != 0)
        drop_saved_bits(saved, c.u32(table + f::left), c.u32(table + f::top), 0x1071b208, 0x1071b220, 0x1071b22c);
    const u32 high = table + 0x130000;
    regs.hold({{31, high}});
    c.put16(current_guio() + g::hit_item, 0xffff);

    // The control at (x, y), if any, is taken as the dragged control's hit item (once), together with the rectangle.
    const auto under_mouse = [&](u32 control) {
        const u32 flags = c.u32(control + g::flags);
        return (flags & 2) != 0 && (flags & 0x200) == 0 && std::int32_t(x) >= c.i32(control + g::left) &&
               std::int32_t(x) <= c.i32(control + g::right) && std::int32_t(y) >= c.i32(control + g::top) &&
               std::int32_t(y) <= c.i32(control + g::bottom);
    };
    const auto take = [&](u32 item, u32 back) {
        const u32 slot = current_guio() + g::hit_item;
        if (c.i16(slot) != -1) return;
        c.put16(slot, item);
        const auto box = rectangle();
        c.put16(frame.at(0x3a), box[0]);
        c.put16(frame.at(0x38), box[1]);
        c.put16(frame.at(0x3e), box[2]);
        c.put16(frame.at(0x3c), box[3]);
        call(callee::block_move, back, {frame.at(0x38), current_guio() + g::saved_rect, 8});
        c.set(2, c.u32(frame.at(0x14)));  // the glue leaves the caller's TOC in its frame
    };

    u32 i = 0;
    regs.hold({{27, 0}, {30, 0}});
    for (; std::int16_t(i) < c.i16(table + f::guio_order_count); ++i) {
        regs.hold({{27, i}, {30, 2 * i}});
        const std::int32_t item = c.i16(table + f::guio_order + 2 * i);
        if (under_mouse(guio_at(table, item))) take(u32(item), 0x1071b320);
    }
    regs.hold({{27, i}, {30, 2 * i}});

    i = 0;
    regs.hold({{25, high}, {27, 0}, {30, 0}});
    for (; std::int16_t(i) < c.i16(table + f::area_order_count); ++i) {
        const u32 order_at = 2 * i + f::area_order;
        regs.hold({{27, i}, {30, 2 * i}, {31, order_at}});
        const u32 row = area_at(table, c.i16(table + order_at));
        if (c.i8(row + 0xbf2) == 0 || c.i16(row + 0x212) != -1) continue;
        c.put16(row + 0xb7c, c.u16(row + 0xb7a));
        // Walk the area's list from its first entry while the entries stay within the area's run of keys.
        for (;;) {
            const u32 object = area_at(table, c.i16(table + order_at)) + 4;
            const std::int32_t cursor = c.i16(object + 0xb78);
            std::int32_t item = -1;
            if (cursor < c.i16(object + 0xb74)) {
                c.put16(object + 0xb78, u32(cursor + 1));
                item = c.i16(object + 0x212 + u32(cursor) * 2);
            }
            if (item == -1) break;
            const u32 listed = area_at(table, c.i16(table + order_at));
            const std::int32_t first = c.i16(listed + 4 + 0x212 + u32(std::int32_t(c.i16(listed + 4 + 0xb76))) * 2);
            const std::int32_t key = c.i32(guio_at(table, item) + g::order_key);
            const std::int32_t run = std::int32_t(u32(key) - u32(c.i32(guio_at(table, first) + g::order_key)));
            if (run > std::int32_t(u32(c.u8(listed + 0xbb5)) - 1)) break;
            if (under_mouse(guio_at(table, item))) take(u32(item), 0x1071b434);
        }
    }
    c.put8(table, 0);
    return u32(std::int32_t(c.i16(table + f::drag_guio)));
}
}
