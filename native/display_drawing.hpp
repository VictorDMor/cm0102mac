#pragma once
// DISPLAY.CPP block "drawing": painting the display table and the per-frame mouse step on it.
//   0x10718f60  paint_first          first full-screen repaint of a table (once per table)
//   0x10718fc0  draw_screen          paints the background, every area and control, then flushes a rectangle
//   0x10719350  screen_step          one step of the table's input loop: pending key results, then the
//                                    handler of the table's mode (0 and 1 are the input block's, 2 and 3 here)
//   0x1071add0  scroll_bar_drag      drags the scroll bar of the current area with the mouse
//   0x1071afc0  area_drag            drags the saved rectangle of the current control, and on release picks
//                                    the control and the area's list entries under the mouse
//   0x10725770  update_blinking      the two timers (1000 ms and 100 ms) that refresh text controls and
//                                    flip a control's two images
#include "display_detail.hpp"

namespace cm::aot::display {
// Offsets this block names (display table fields are 0x130000 plus the original's negative displacement;
// an area row is table + index * 0xbf0 and its AREA.CPP object starts at +4; a control is a GUIO object).
namespace drawing {
inline constexpr std::uint32_t area_row_size = 0xbf0, guio_base = 0xba864, guio_size = 0x17c;
namespace field {  // display table
inline constexpr std::uint32_t area_order = 0x129da8, guio_order = 0x129f9c,  // halfword arrays
    guio_count = 0x129da6, area_order_count = 0x12a8fc, guio_order_count = 0x12a8fe,
    painted_once = 0x12a900, key_expected = 0x12a901, key_result = 0x12a902, cancel_result = 0x12a904,
    enter_result = 0x12a906, last_x = 0x12a90c, last_y = 0x12a910, left = 0x12a914, top = 0x12a918,
    right = 0x12a91c, bottom = 0x12a920, saved_bits = 0x12a924, background = 0x12a928,
    drag_area = 0x12a932, drag_guio = 0x12a934, clear_flag = 0x12a938, skip_draw = 0x12a93e, drawn = 0x12a940,
    background_name = 0x12a946;
}
namespace guio {  // from a control's address (table + index * 0x17c + 0xba864)
inline constexpr std::uint32_t flags = 0xc, left = 0x10, top = 0x14, right = 0x18, bottom = 0x1c, order_key = 0x24,
    flags2 = 0x38, saved_rect = 0x60, hit_item = 0x76;
}
}
// `table` is the display table (r3). Callers of the routines that are not void read r3.
void paint_first(Cpu&, std::uint32_t table);
// r4..r7 are the rectangle's left, top, right and bottom as the caller holds them (halfwords, -1 = whole screen).
std::uint32_t draw_screen(Cpu&, std::uint32_t table, std::uint32_t left, std::uint32_t top, std::uint32_t right,
                          std::uint32_t bottom);
std::uint32_t screen_step(Cpu&, std::uint32_t table);
std::uint32_t scroll_bar_drag(Cpu&, std::uint32_t table);
std::uint32_t area_drag(Cpu&, std::uint32_t table);
void update_blinking(Cpu&, std::uint32_t table);
}
