#pragma once
// graph_carbon.cpp (tools/graph_carbon_audit.py): the game's drawing layer over the Mac, rewritten as typed C++. Each function is one
// routine of the original; the adapter (graph_carbon_adapter.cpp, graph_carbon_routines*.inc) reads the guest ABI's arguments, calls it
// and leaves the results the original leaves. The routines are native inside and keep the guest ABI at their edge: guest memory is the
// only truth, and the registers they leave (r3 and r4-r10) are those of the original's last instruction or callee.
#include "guest_abi.hpp"

namespace cm::aot::graph_carbon {
using guest::Cpu;

// The argument registers r4-r10 as a routine leaves them: the hook starts from the registers it was entered with and the routine
// overwrites the ones the original writes (callers' next calls store them into the stack image, and the checker compares them).
struct Leftovers { std::uint32_t r4, r5, r6, r7, r8, r9, r10; };
// A rectangle as the routines take it in registers: left, top, right, bottom (signed 32-bit words).
struct Corners { std::int32_t left, top, right, bottom; };

// Block `leaves` (graph_carbon_leaves.cpp): the leaf routines of the module.
// 0x10769b50: the display mode flag (a byte; nonzero while the game runs full screen: the initialization saves it, GAME_CONFIG.CPP and
// MAIN.CPP ask for it).
std::uint32_t full_screen(Cpu&);
// 0x1076e910: the index of the cursor 0x1076e920 last set (a byte kept in the TOC's small data).
std::uint32_t current_cursor(Cpu&);
// 0x107698d0: the Mac Rect {top, left, bottom, right} at `rect` that holds the two corner points (x1, y1) and (x2, y2) in order; the
// larger coordinates are left in r5 and r6.
void rectangle_from_corners(Cpu&, std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2, std::uint32_t rect, Leftovers&);
// 0x10769920: the area of the intersection of two rectangles, 0 when it is empty (a negative width or height counts as none; the
// product wraps at 32 bits). The intersection's edges are left in r7-r10, its height in r4 and its width in r5.
std::uint32_t intersection_area(Corners a, Corners b, Leftovers&);
// 0x1076bb20: the 15-bit pixel of an 8-bit red, green and blue (the top five bits of each).
std::uint32_t pack_pixel(std::uint32_t red, std::uint32_t green, std::uint32_t blue);
// 0x1076bb40: a 15-bit pixel with each component scaled by `percent` (the low byte, a percentage) and clamped at 255; the divide by 100
// is the compiler's multiply by 0x51eb851f.
std::uint32_t scale_pixel(std::uint32_t pixel, std::uint32_t percent, Leftovers&);
// 0x1076ca20: the callback the text drawer 0x1074f800 calls through its ninth argument (descriptor 0x108bdaac): it counts its calls and,
// when the count reaches the limit the caller 0x1076ca50 stored, keeps the low halfword of `value` (sign-extended). Returns 0.
std::uint32_t probe_text_position(Cpu&, std::uint32_t value, Leftovers&);
}
