#pragma once
// graph_carbon.cpp, blocks of the text layer (graph_carbon_text*.cpp): the bitmap fonts' record views, the TOC slots the text routines
// read, the register and call helpers they share. Offsets come from the listing and the decompilation (.local/rewrites/graph_carbon).
#include "graph_carbon_detail.hpp"

namespace cm::aot::graph_carbon {
// Table of contents slots of the text layer (each holds the address of a variable, read with Cpu::toc).
namespace text_toc {
inline constexpr std::int32_t font_slots = -0x3bd8,       // the array of 8 outline font slots (0x10bf71b0; FONT.CPP's records)
    font_table = -0x3bdc,                                 // the table of 7 bitmap fonts (0x1404 bytes each, font n at (n - 1) * 0x1404)
    callback = -0x3be4,                                   // the descriptor of the probe callback 0x1076ca20
    draw_jumps = -0x3be8, width_jumps = -0x3bec, height_jumps = -0x3bf0,  // the three tables of eight arms (read through the TOC)
    selection = -0x740c,                                  // FONT.CPP's font selection (-1: the bitmap fonts)
    window_width = -0x746c, window_height = -0x7470,      // the display's width and height words
    message_flag = -0x7bac, message_buffer = -0x7ba8;     // the assertion's success flag (a byte) and its message buffer
}
// The surface variables of the small data.
namespace text_small {
inline constexpr std::int32_t initialised = -0x3550,   // byte
    surface = -0x3564,                                 // u32: the 15-bit surface's base
    row_bytes = -0x3566, stride = -0x3568;             // i16: the row's bytes and its pixels
}
// A bitmap font (0x1404 bytes): the height of its glyphs, then 256 glyph records of 0x14 bytes from character 0x20 up (the file has
// words 1-4 of each; the loader fills the pixel pointer). The pixels are 4 bits each, two to a byte, rows of `width` pixels.
namespace bitmap_font {
inline constexpr std::uint32_t size = 0x1404, height = 0, first_glyph = 4, glyph_size = 0x14, space = 0x20;
inline constexpr std::uint32_t glyph(std::uint32_t font, std::uint32_t character) { return font + first_glyph + character * glyph_size; }
// Offsets inside a glyph record.
namespace glyph_field {
inline constexpr std::uint32_t width = 0, row_bytes = 4, lead = 8, trail = 0xc, pixels = 0x10;
}
}
// The pool's offsets of the assertion text (the file name 0, the format 0x11 and the title 0x1e) and the report's TOC words.
namespace text_pool { inline constexpr std::uint32_t format = 0x11, title = 0x1e; }

// The argument registers r3-r10 as the original has them at a point of a routine: the routine keeps them by name where it reaches a
// call or its end (the callees read them and the checker compares them in full).
struct Scratch {
    std::uint32_t r3, r4, r5, r6, r7, r8, r9, r10;
    static Scratch take(const Cpu& c) { return {c.r(3), c.r(4), c.r(5), c.r(6), c.r(7), c.r(8), c.r(9), c.r(10)}; }
    void put(Cpu& c) const { c.set({{3, r3}, {4, r4}, {5, r5}, {6, r6}, {7, r7}, {8, r8}, {9, r9}, {10, r10}}); }
};

// `extsh`.
constexpr std::uint32_t extsh(std::uint32_t value) { return std::uint32_t(std::int32_t(std::int16_t(value & 0xffff))); }
// The high word of the signed product (`mulhw`).
constexpr std::uint32_t mulhw(std::uint32_t a, std::uint32_t b) { return std::uint32_t((std::int64_t(std::int32_t(a)) * std::int64_t(std::int32_t(b))) >> 32); }
// `srawi`: the shifted value and the carry `addze` adds (a negative value with a one bit shifted out).
struct ArithmeticShift { std::uint32_t value, carry; };
constexpr ArithmeticShift srawi(std::uint32_t value, unsigned shift) {
    const auto negative = (value >> 31) != 0;
    return {std::uint32_t(std::int32_t(value) >> shift), negative && (value & ((1u << shift) - 1)) != 0 ? 1u : 0u};
}
// The sign bit as `rlwinm r,r,1,31,31` takes it.
constexpr std::uint32_t sign_bit(std::uint32_t value) { return value >> 31; }

// One call of a text routine to another routine of the module that is C++ (typed): the link register as the original's `bl` leaves it, and
// the checker's call trace told (the original's call is a dispatch it records).
inline void direct_call(Cpu& c, std::uint32_t target, std::uint32_t back) {
    c.link(back);
    if (graph_carbon_direct_call_probe) graph_carbon_direct_call_probe(c.s, target);
}

// The routines the text layer is one of the callers of: FONT.CPP's drawer and string width, the line drawer, the assertion's callees.
inline constexpr std::uint32_t font_draw_entry = 0x1074f800, font_width_entry = 0x107500f0, line_entry = 0x1076a140;
}
