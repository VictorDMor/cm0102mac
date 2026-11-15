// DISPLAY.CPP block "input": mouse and keyboard handling over the display table's controls.
//   0x1071a280 pointer_press   the pointer pass over the controls and areas (hover, press, text fields, scroll bars)
//   0x1071aa10 pointer_nested  the same walk over the areas that have a parent (hover chains, tooltips)
//   0x1071b520 text_field_key  one text field's keyboard editing
//   0x1071bd00 scroll_bar      one area's scroll bar: arrows, track and thumb
//   0x1071c190 pointer_inside  whether the pointer lies in a rectangle
#pragma once
#include "guest_abi.hpp"

namespace cm::aot::display {
// 0x1071c190 (frameless): the pointer (the words behind TOC -0x5c10 and -0x5c14) in [left, right] x [top, bottom],
// signed compares, the horizontal one first. r3 = 1 or 0.
inline std::uint32_t pointer_inside(const guest::Cpu& c, std::uint32_t left, std::uint32_t top, std::uint32_t right,
                                    std::uint32_t bottom) {
    const auto x = c.i32(c.toc(-0x5c10));
    if (x < std::int32_t(left) || x > std::int32_t(right)) return 0;
    const auto y = c.i32(c.toc(-0x5c14));
    return y >= std::int32_t(top) && y <= std::int32_t(bottom);
}
// The routines below take and return what the original's r3/r4 hold.
std::uint32_t pointer_press(guest::Cpu&, std::uint32_t table);                        // 0x1071a280
std::uint32_t pointer_nested(guest::Cpu&, std::uint32_t table);                       // 0x1071aa10
std::uint32_t text_field_key(guest::Cpu&, std::uint32_t table, std::uint32_t r4);     // 0x1071b520 (r4: control, halfword)
std::uint32_t scroll_bar(guest::Cpu&, std::uint32_t table, std::uint32_t r4);         // 0x1071bd00 (r4: area, halfword)
}
