// FONT.CPP, block `selector` (0x10750320..0x10750580, one routine, 152 instructions): the style selector. A font record keeps an sfnt (the
// font file's tables) and a scaler (the engine's T2K object) for one style and size; this routine makes them match what the caller asks
// for: bits 0 and 1 of the style (an emboldening of strength 1.25 or 1.5) re-create the sfnt, handing it the two functions of the style
// block (the outline's and the advances', TOC -0x5b74 and -0x5b78) and the strength, and then the scaler over it; bits 2 and 3 (which of
// the pool's three skew angles) and the size set the scaler's 2x2 transformation, whose skew term is `constant * sin(angle) * size`
// converted to an integer (the module's only floating point). (font_cpp_detail.hpp's toc names bold_style and italic_style are the
// pilot's guess for those two words: the first is the outline's function, the second the advances'.)
//
// It is a stack-contract leaver (stmw r24, a 0xa0 frame): the game's printf reads the bytes its frame leaves, so the frame is the
// original's (size, saved r24-r31, link register, and every local the original stores). Each call is made with the registers the
// original holds there (r24 = the outline function's TOC word until the size word replaces it, r25 the style, r26 the size, r27 the
// callbacks record or zero, r28 the font record, r29 the pool's constants, r30 the advances function's TOC word, r31 the pool), because a
// callee's prologue saves them. Registers: r3 and r4-r10 are left as the original leaves them (they are compared in full); only the
// registers the original writes are written here.
//
// Floating point: the constants are read from guest memory (the module's pool, TOC -0x3e10: three angles, the skew factor and the
// 2^52 + 2^31 conversion constant), the product keeps the original's order ((skew * sin) first, then (size - magic) * that), through the
// guest FPU's methods so FPSCR is left as the original leaves it; the conversion is fctiwz (round toward zero).
//
// The engine's routines are called through the dispatcher (c.call at the original's return address), not as typed C++: they are C++ in
// production either way, but the checker runs the original side's engine calls generated (its hooks decline for entries it does not check)
// and the engine's own checks leave their results' r3 (and r4-r8) unspecified, so a typed call would show the engine's leftovers, not this
// routine's. Through the dispatcher both sides run the same callee and the registers compare in full.
#include "font_cpp_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::font_cpp {
namespace {
constexpr std::uint32_t assertion_report = 0x107755c0, sine = 0x10780558;
// Entries of the engine the selector calls, through the dispatcher (see the comment at the top).
constexpr std::uint32_t new_t2k_entry = 0x1075e960, set_transformation_entry = 0x1075eef0, delete_t2k_entry = 0x10760f50,
                        new_sfnt_entry = 0x10766ab0, delete_sfnt_entry = 0x10767190;
// The pool's assertion texts: the first (+0xd) for a null pointer, the second (+0x5a) for an engine error; its file name at +0x12.
constexpr std::uint32_t null_text = 0xd, error_text = 0x5a, file_name = 0x12;
// The constants at TOC -0x3e10: the skew angles (bit 2, bit 3, neither), the skew factor and the integer conversion's magic number.
constexpr std::uint32_t angle_if_bit2 = 0x0, angle_if_bit3 = 0x8, angle_otherwise = 0x10, skew_factor = 0x18, conversion_magic = 0x20;
// Locals of the original's frame (offsets from the new stack pointer).
constexpr std::int32_t error_word = 0x38, matrix = 0x3c /* four words: xx, xy, yx, yy */, callbacks = 0x4c /* three words */, converted = 0x64,
                       magic_hi = 0x6c, magic_lo = 0x70, glue_toc_save = 0x14;
// The callbacks record the new sfnt is given: the outline function's and the advances function's TOC words and the strength (16.16).
constexpr std::uint32_t strength_bit0 = 0x14000, strength_bit1 = 0x18000;
}

void select_style(Cpu& c, std::uint32_t slot, std::uint32_t style, std::uint32_t size) {
    Frame frame(c, 0x10750320, 0xa0, {.first_gpr = 24});
    const auto local = [&](std::int32_t offset) { return frame.at(offset); };
    // The prologue's loads, in the original's order; r27 starts as 0 (no callbacks record).
    c.set({{29, c.toc(toc::skew_constants)}, {25, style}, {30, c.toc(toc::bold_style)}, {26, size}, {24, c.toc(toc::italic_style)}, {27, 0},
           {31, c.toc(toc::pool)}, {28, c.u32(slot)}});
    const auto pool = c.r(31), font = c.r(28);
    const auto report = [&](std::uint32_t text, std::uint32_t line, std::uint32_t back) {
        c.set({{3, pool + text}, {4, pool + file_name}, {5, line}});
        c.call(assertion_report, back);
    };
    // The error word of an engine call: the callee stores the code there; any code but 0 is reported.
    const auto engine_error = [&](std::uint32_t line, std::uint32_t back) {
        if (c.u32(local(error_word)) != 0) report(error_text, line, back);
    };
    if (font == 0) report(null_text, 0x416, 0x10750368);

    c.set(3, c.u32(font + font_record::scaler));
    bool recreate = true;
    if (c.r(3) != 0) {
        c.set(4, style & 3);
        recreate = (style & 3) != (c.u16(font + font_record::style) & 3);
    }
    if (recreate) {
        // The old scaler and sfnt are deleted (r3 still holds the scaler here).
        if (c.r(3) != 0) {
            c.set(4, local(error_word));
            c.call(delete_t2k_entry, 0x10750398);
        }
        c.set(3, c.u32(font + font_record::sfnt));
        if (c.r(3) != 0) {
            c.set(4, local(error_word));
            c.call(delete_sfnt_entry, 0x107503ac);
        }
        // Bit 0, else bit 1, hands the sfnt a record {outline function, advances function, strength}; neither: no record.
        c.set(3, style & 0xffff);
        if ((style & 1) != 0) {
            c.set(3, 0x10000);
            c.put32(local(callbacks), c.r(24));
            c.set(27, local(callbacks));
            c.put32(local(callbacks + 4), c.r(30));
            c.put32(local(callbacks + 8), strength_bit0);
        } else if ((c.r(3) & 2) != 0) {
            c.set(3, 0x20000);
            c.put32(local(callbacks), c.r(24));
            c.set(27, local(callbacks));
            c.put32(local(callbacks + 4), c.r(30));
            c.put32(local(callbacks + 8), strength_bit1);
        }
        // The sfnt over the font's memory object and input stream: format 2, index 0.
        c.set({{3, c.u32(font + font_record::sfnt_memory)}, {7, c.r(27)}, {6, c.u32(font + font_record::stream)}, {8, local(error_word)},
               {4, 2}, {5, 0}});
        c.call(new_sfnt_entry, 0x10750410);
        c.put32(font + font_record::sfnt, c.r(3));
        engine_error(0x439, 0x10750430);
        // The scaler over it.
        c.set({{4, c.u32(font + font_record::sfnt)}, {5, local(error_word)}});
        c.set(3, c.u32(c.r(4) + 0x5c));
        c.call(new_t2k_entry, 0x10750440);
        c.put32(font + font_record::scaler, c.r(3));
        engine_error(0x43f, 0x10750460);
        // The sfnt's platform and encoding ids: 3 and 1 (Microsoft, Unicode); the size byte is reset.
        c.set({{3, c.u32(font + font_record::scaler)}, {5, 3}, {4, 1}});
        c.set(3, c.u32(c.r(3) + scaler::sfnt));
        c.put16(c.r(3) + sfnt_ids::platform, c.r(5));
        c.set(3, c.u32(font + font_record::scaler));
        c.set(3, c.u32(c.r(3) + scaler::sfnt));
        c.put16(c.r(3) + sfnt_ids::encoding, c.r(4));
        c.put8(font + font_record::size, 0);
    }

    // The skew and size: skipped when bits 2 and 3 of the style and the size are what the record has.
    c.set({{3, style & 0xc}, {4, style & 0xffff}});
    bool settled = (style & 0xc) == (c.u16(font + font_record::style) & 0xc);
    if (settled) {
        c.set(3, size & 0xff);
        settled = c.r(3) == c.u8(font + font_record::size);
    }
    if (!settled) {
        const auto constants = c.r(29);
        // The angle: bit 2 first, then bit 3, else the third constant.
        const auto angle = (c.r(4) & 4) != 0 ? angle_if_bit2 : (c.r(4) & 8) != 0 ? angle_if_bit3 : angle_otherwise;
        c.set_f(1, c.f64(constants + angle));
        c.set(24, std::rotl(size, 16) & 0x00ff0000u);  // the size in the 16.16 matrix's integer half
        c.put32(local(matrix), c.r(24));
        c.call(sine, 0x107504e0);
        c.set(2, c.u32(local(glue_toc_save)));
        // size as a double (the integer-conversion trick: 0x43300000 : size, less 2^52), skew * sin(angle) * size, truncated.
        c.set(3, 0x43300000);
        c.put32(local(magic_lo), size & 0xff);
        c.put32(local(magic_hi), c.r(3));
        c.set({{7, local(matrix)}, {9, local(error_word)}});
        guest::Fpu fpu(c);
        c.set_f(0, c.f64(constants + skew_factor));
        c.set_f(2, c.f64(constants + conversion_magic));
        c.set_f(1, fpu.fmul(c.f(0), c.f(1)));
        c.set_f(0, c.f64(local(magic_hi)));
        c.set({{4, 0}, {5, 0x60}, {6, 0x60}, {8, 1}});
        c.put32(local(matrix + 8), 0);
        c.put32(local(matrix + 12), c.r(24));
        c.set_f(0, fpu.fsub(c.f(0), c.f(2)));
        c.set_f(0, fpu.fmul(c.f(0), c.f(1)));
        c.put64(local(converted), fpu.fctiwz(0, c.f(0)));
        c.put32(local(matrix + 4), c.u32(local(converted + 4)));
        c.set(3, c.u32(font + font_record::scaler));
        c.call(set_transformation_entry, 0x10750548);
        engine_error(0x45d, 0x10750564);
    }
    c.put16(font + font_record::style, style);
    c.put8(font + font_record::size, size);
}
}
