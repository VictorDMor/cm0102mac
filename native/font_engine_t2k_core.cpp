// The font engine, block `t2k_core` (T2K.C, 0x1075e960..0x10761070): the scaler object FONT.CPP keeps per font (the T2K object,
// t2k_object:: in font_engine_detail.hpp), its creation, the transformation (size and 2x2 matrix, and the metrics they give), the
// release of what a rendering leaves, the renderer of one glyph (which recurses over a composite's components) and the destruction. The
// five API entries are 0x1075e960 (new), 0x1075eef0 (transformation), 0x1075f980 (purge), 0x10760e30 (render a character) and 0x10760f50
// (delete); the rest are their workers.
//
// What the game asks (FONT.CPP): the renderer with cmd = 2 (scan convert, no hinting) for the characters 0x20..0xff, the transformation with
// the hinter off. The hinter (AUTOGRID.C, GHINTS.C, the interpreter) is therefore dormant: its calls are written as the original makes them
// (through the dispatcher, the callees being generated) and are reached by cases only.
//
// Errors (brief.md, "How errors unwind"): the five API entries run setjmp on the memory object's jump buffer and catch the transfer of an error
// site (the allocator's, `raise_error`) at the setjmp's return: the code goes through the caller's pointer and the object's blocks are freed.
// The setjmp stores the condition register, so cr0 is what the original's last compare left before it.
//
// Registers. Every entry here has callers that read r9 and r10 after it returns (tools/register_contract.py --after; the constructor's r3
// too); r4-r8 are rewritten by the reporter or the next call before anything reads them. So r9 and r10 are written where the original writes
// them and pass through every callee (the callees that write them, 0x10766dc0 and 0x1075c0f0, leave them as their originals do: both are compared
// on r9/r10 in their own blocks). The renderer keeps the original's non-volatile registers r13-r31 in `r` and sets all of them before every call that can
// reach a stack leaver (the allocator's release, the scan converter's), because the leaver check compares what that leaver's prologue saves.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
namespace ob = t2k_object;
constexpr std::uint32_t sext8(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int8_t(v))); }
constexpr std::uint32_t sext16(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
// mulhw: the high word of the signed 64-bit product.
constexpr std::int32_t mulhw(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32); }
// sraw: an arithmetic shift by the low six bits of the count (32 and over fill with the sign).
constexpr std::uint32_t sraw(std::uint32_t value, std::uint32_t count) {
    count &= 0x3f;
    return count >= 32 ? std::uint32_t(std::int32_t(value) >> 31) : std::uint32_t(std::int32_t(value) >> count);
}
constexpr std::uint32_t file_name = 0x10;  // the file name in T2K.C's pool
// Callees that still run generated (the dispatcher runs them): the hinter's (AUTOGRID.C, GHINTS.C) and the scan converter's (T2KSC.C).
constexpr std::uint32_t hinter_new = 0x107555d0, hinter_size = 0x10755930, hinter_delete = 0x10755840, hinter_setup = 0x107551c0, hinter_glyph = 0x107552b0,
                        hinter_copy = 0x1075ad60, hinter_make = 0x1075af60, scan_new = 0x107620d0, scan_run = 0x107623c0, scan_release = 0x10762e30;
constexpr std::uint32_t tag_t2kg = 0x54324b47;
constexpr std::uint32_t error_null_memory = 10000, error_null_matrix = 0x2711, error_bad_size = 0x2712, error_bad_quality = 0x2713, error_no_glyph = 0x2715,
                        error_too_many_points = 0x2716, error_bad_object = 0x2717, error_bad_stamp = 0x2723;

// The assertion of T2K.C's pool at `text`, line `line`, called at `back`.
void assertion(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back) {
    dispatched::assertion(c, pool, text, file_name, line, back);
}

// The identity test of the matrix words: 1 when they are the unit matrix.
std::uint32_t is_identity(std::uint32_t xx, std::uint32_t xy, std::uint32_t yx, std::uint32_t yy) {
    return xx == 0x10000 && xy == 0 && yx == 0 && yy == 0x10000 ? 1 : 0;
}
}

// 0x1075e960 NewT2K(mem, sfnt, error): the scaler object over an sfnt. With the setjmp the memory object's errors come back as the code in
// *error (0, or an error site's; 10000 for a null memory object) and the result is null then. The object is tsi_AllocMem(0x5f8) and only the
// fields below are set: the stamps, the memory object and the sfnt, the render result words, the table of -1 words from +0x14c, and the
// sfnt's platform and encoding ids set to "any" (0xffff).
std::uint32_t new_t2k(Cpu& c, std::uint32_t mem, std::uint32_t font, std::uint32_t error_out) {
    Frame frame(c, 0x1075e960, 0x50, {.first_gpr = 29});
    const auto pool = c.toc(toc::t2k_pool);
    c.set({{29, mem}, {30, font}, {31, error_out}});
    if (error_out == 0) assertion(c, pool, 0, 0x64, 0x1075e99c);
    if (mem == 0) {
        c.put32(error_out, error_null_memory);
        c.set(3, 0);
        return 0;
    }
    const auto fail = [&] {
        c.set(3, mem);
        c.link(0x1075eaec);
        free_blocks(c, mem);
        c.set(3, 0);
        return std::uint32_t(0);
    };
    try {
        c.set(3, mem + tsimem::jump_buffer);
        c.compare_logical(0, mem, 0);  // `cmplwi r29,0`: setjmp stores the condition register into the jump buffer
        c.call(dispatched::setjmp_entry, 0x1075e9b8);
        const auto code = c.r(3);
        c.put32(error_out, code);
        if (code != 0) return fail();
        c.set({{3, mem}, {4, ob::size}});
        c.link(0x1075e9d0);
        const auto t = allocate_block(c, mem, ob::size);
        c.put32(t + ob::mem, mem);
        c.put32(t + ob::stamp, 0x5a1234a5);
        c.put32(t + ob::sfnt, font);
        c.put32(t + ob::end_stamp, 0xa5fedc5a);
        c.set({{9, 0}, {10, 0}});
        c.put32(t + ob::glyph, 0);
        c.put16(t + ob::glyph_number, 0);
        c.put32(t + ob::hint_handle, 0);
        c.put32(t + ob::bitmap, 0);
        c.put32(t + ob::bitmap_block, 0);
        c.put32(t + ob::buffer_20, 0);
        c.put32(t + ob::buffer_24, 0);
        c.put32(t + ob::hint_size_y, 0xffffffff);
        c.put32(t + ob::hint_size_x, 0xffffffff);
        for (std::uint32_t n = 0; n < 0x95; ++n) c.put32(t + ob::minus_ones + 4 * n, 0xffffffff);  // 144 words in groups of eight, then five
        c.put16(c.u32(t + ob::sfnt) + sfnt::platform_id, 0xffff);
        c.put16(c.u32(t + ob::sfnt) + sfnt::encoding_id, 0xffff);
        c.put32(t + ob::glyph_cache, 0);
        c.set(3, t);
        return t;
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != 0x1075e9b8 || c.sp() != frame.base()) throw;
        c.put32(error_out, c.r(3));
        return fail();
    }
}

// 0x1075eb10: the hinter for the object's size (dormant for the game: the transformation's hint flag is 0). The first time it makes the hinter
// (a handle at +0x148, from the font's glyph count and units per em), the font's hint data (+0x38 of the sfnt: 0x6c bytes made from the
// font's T2KG table, or by the hinter's own routine) and gives them to the hinter; when the size differs from the last one it sets it
// (and the interpreter's, when the font has hint tables).
void prepare_hinting(Cpu& c, std::uint32_t t) {
    Frame frame(c, 0x1075eb10, 0xb0, {.first_gpr = 30});
    std::uint32_t r30 = c.r(30);
    const auto hold = [&] { c.set({{30, r30}, {31, t}}); };
    c.set(31, t);
    const auto mem = c.u32(t + ob::mem);
    const auto raise = [&](std::uint32_t code, std::uint32_t back) {
        c.set({{3, mem}, {4, code}});
        hold();
        c.link(back);
        raise_error(c, mem, code);
    };
    if (c.u32(t + ob::hint_handle) == 0) {
        const auto font = c.u32(t + ob::sfnt);
        c.set(3, font);
        hold();
        c.link(0x1075eb3c);
        const auto points = sext16(max_outline_points(c, font));
        c.set({{3, mem}, {4, points}, {5, sext16(c.u16(t + ob::units_per_em))}, {6, t + ob::hint_handle}});
        hold();
        c.call(hinter_new, 0x1075eb54);
        if (c.r(3) != 0) raise(c.r(3), 0x1075eb64);
        c.set(3, c.u32(t + ob::sfnt));
        hold();
        c.link(0x1075eb6c);
        const auto limit = glyph_limit(c, c.u32(t + ob::sfnt));
        c.put32(t + ob::hint_mode, std::int32_t(limit) >= 1000 ? 2 : 1);
        const auto font_now = c.u32(t + ob::sfnt);
        if (c.u32(font_now + 0x38) == 0) {
            r30 = 0;
            if (c.u32(font_now + sfnt::glyph_callback) == 0) {
                c.set({{3, font_now}, {4, tag_t2kg}});
                hold();
                c.link(0x1075ebb4);
                r30 = new_table_stream(c, font_now, tag_t2kg);
            }
            if (r30 != 0) {
                c.set({{3, frame.at(0x38)}, {4, r30}});
                hold();
                c.call(hinter_copy, 0x1075ebcc);
                c.set({{3, r30}, {4, 0}});
                hold();
                c.link(0x1075ebd8);
                delete_input_stream(c, r30, 0);
            } else {
                c.set({{3, c.u32(t + ob::sfnt)}, {4, c.u32(t + ob::hint_handle)}, {5, frame.at(0x38)}, {6, c.u32(t + ob::hint_mode) == 2 ? 1u : 0u}});
                hold();
                c.call(hinter_make, 0x1075ebfc);
            }
            c.set({{3, c.u32(t + ob::mem)}, {4, 0x6c}});
            hold();
            c.link(0x1075ec08);
            const auto block = allocate_block(c, c.u32(t + ob::mem), 0x6c);
            c.put32(c.u32(t + ob::sfnt) + 0x38, block);
            // The buffer on the stack is filled by 0x1075ad60 (every byte: ten triples and two arrays of twelve halfwords) when the font has a T2KG
            // table and no glyph callback (sfnt+0x3c = 0); the hinter's own routine 0x1075af60 is taken otherwise (a font without T2KG, and also a font
            // whose sfnt+0x3c is set, i.e. the bold style) and leaves bytes unwritten: the original then copies the stack's leftovers into the block,
            // which a native frame cannot hold the same. Dormant for the game: FONT.CPP passes hint = 0 (0x10750510) and cmd = 2 (0x10750738); the checks
            // give such a font a block of its own instead.
            c.set({{3, c.u32(c.u32(t + ob::sfnt) + 0x38)}, {4, frame.at(0x38)}, {5, 0x6c}});
            hold();
            c.call(byte_move, 0x1075ec24);
        }
        c.set({{3, c.u32(t + ob::hint_handle)}, {4, c.u32(c.u32(t + ob::sfnt) + 0x38)}, {5, c.u32(t + ob::hint_mode)}});
        hold();
        c.call(hinter_setup, 0x1075ec38);
        if (c.r(3) != 0) raise(c.r(3), 0x1075ec48);
    }
    if (c.u32(t + ob::hint_size_x) == c.u32(t + ob::size_x) && c.u32(t + ob::hint_size_y) == c.u32(t + ob::size_y)) return;
    c.set({{3, c.u32(t + ob::hint_handle)}, {4, c.u32(t + ob::size_x)}, {5, c.u32(t + ob::size_y)}, {6, t + ob::hint_byte}});
    hold();
    c.call(hinter_size, 0x1075ec78);
    if (c.r(3) != 0) raise(c.r(3), 0x1075ec88);
    if (c.u32(c.u32(t + ob::sfnt) + sfnt::hint_font) != 0) {
        c.put8(t + ob::hint_byte, 0);
        c.set({{3, c.u32(c.u32(t + ob::sfnt) + sfnt::hint_font)}, {4, c.u32(t + ob::size_x)}, {5, c.u32(t + ob::size_y)}});
        hold();
        c.link(0x1075ecb4);
        set_hint_size(c, c.u32(c.u32(t + ob::sfnt) + sfnt::hint_font), c.u32(t + ob::size_x), c.u32(t + ob::size_y));
    }
    c.put32(t + ob::hint_size_x, c.u32(t + ob::size_x));
    c.put32(t + ob::hint_size_y, c.u32(t + ob::size_y));
}

// 0x1075ece0: `count` halfword coordinates scaled by a ratio record (scale_ratio's: +0 multiplier, +2 shift, +4 divisor, +8 rounding, +0xc a
// 16.16 factor, +0x10 the kind). Kind 0 is (v * multiplier + rounding) >> shift, kind 1 the same divided by the divisor (sign by magnitude: the
// rounding is added to the negated product of a negative one), kind 2 a 16.16 multiply (0x10767d40). Another kind does nothing. A kind-0 loop
// of more than eight values is unrolled by eight in the original: it leaves r9 = 0x20 per pass and r10 = the seventh value of the last pass.
void scale_coordinates(Cpu& c, std::uint32_t source, std::uint32_t destination, std::int32_t count, std::uint32_t ratio) {
    Frame frame(c, 0x1075ece0, 0x50, {.first_gpr = 27});
    const auto kind = c.i16(ratio + 0x10);
    const auto multiplier = sext16(c.u16(ratio));
    const auto rounding = c.u32(ratio + 8);
    if (kind == 1) {
        const auto divisor = c.u32(ratio + 4);
        for (std::int32_t n = 0; n < count; ++n) {
            auto product = sext16(c.u16(source + 2 * std::uint32_t(n))) * multiplier;
            std::uint32_t quotient;
            if (std::int32_t(product) < 0) quotient = negate(std::uint32_t(divide_signed<4>(negate(product) + rounding, divisor)));
            else quotient = std::uint32_t(divide_signed<4>(product + rounding, divisor));
            c.put32(destination + 4 * std::uint32_t(n), quotient);
        }
    } else if (kind == 0) {
        const auto shift = sext16(c.u16(ratio + 2));
        std::uint32_t seventh = 0;
        for (std::int32_t n = 0; n < count; ++n) {
            const auto value = sraw(sext16(c.u16(source + 2 * std::uint32_t(n))) * multiplier + rounding, shift);
            c.put32(destination + 4 * std::uint32_t(n), value);
            if (n % 8 == 6 && n < 8 * ((count - 1) >> 3)) seventh = value;
        }
        if (count > 8) c.set({{9, 0x20 * std::uint32_t((count - 1) >> 3)}, {10, seventh}});
    } else if (kind == 2) {
        const auto factor = c.u32(ratio + 0xc);
        Leftovers left{};
        for (std::int32_t n = 0; n < count; ++n)
            c.put32(destination + 4 * std::uint32_t(n), std::uint32_t(fix_multiply(wrap(factor), std::int32_t(sext16(c.u16(source + 2 * std::uint32_t(n)))), left)));
    }
}

// 0x1075eef0 T2K_NewTransformation(t2k, hint, size_x, size_y, matrix, ?, error): the size (pixels per em, x and y) and the 2x2 matrix (four 16.16
// words, normalised here to unit columns), and what follows from them: the scales to 16.16 and per unit of the em, the ratio records of
// scale_ratio, the glyph cache's bitmap tables, the resolution, and the font's horizontal and vertical metrics (ascender, descender, line gap,
// maximum advance, caret, underline: the first four as an x and a y word) rotated through the matrix. A null matrix or a size that is not positive
// raises (0x2711, 0x2712). With the hint flag it prepares the hinter (dormant for the game).
void set_transformation(Cpu& c, std::uint32_t t, std::uint32_t hint, std::uint32_t size_x_in, std::uint32_t size_y_in, std::uint32_t matrix,
                        std::uint32_t extra_word, std::uint32_t error_out) {
    Frame frame(c, 0x1075eef0, 0xa0, {.first_gpr = 20});
    std::array<std::uint32_t, 32> r{};
    for (unsigned n = 20; n <= 31; ++n) r[n] = c.r(n);
    const auto hold = [&] { for (unsigned n = 20; n <= 31; ++n) c.set(n, r[n]); };
    r[20] = error_out;
    r[24] = c.toc(toc::t2k_pool);
    r[31] = t;
    r[25] = hint;
    r[21] = size_x_in;
    r[23] = size_y_in;
    r[26] = matrix;
    r[27] = extra_word;
    const auto pool = r[24];
    if (error_out == 0) {
        hold();
        assertion(c, pool, 0, 0x165, 0x1075ef34);
    }
    const auto mem = c.u32(t + ob::mem);
    const auto fail = [&] {
        c.set(3, c.u32(t + ob::mem));
        hold();
        c.link(0x1075f854);
        free_blocks(c, c.u32(t + ob::mem));
    };
    const auto raise = [&](std::uint32_t code, std::uint32_t back) {
        c.set({{3, c.u32(t + ob::mem)}, {4, code}});
        hold();
        c.link(back);
        raise_error(c, c.u32(t + ob::mem), code);
    };
    const auto multiply = [&](std::uint32_t a, std::uint32_t b) {
        Leftovers left{};
        return std::uint32_t(fix_multiply(wrap(a), wrap(b), left));
    };
    const auto divide = [&](std::uint32_t a, std::uint32_t b) {
        Leftovers left{};
        return std::uint32_t(fix_divide(wrap(a), wrap(b), left));
    };
    try {
        c.set(3, mem + tsimem::jump_buffer);
        hold();
        if (error_out != 0) c.compare(0, std::int32_t(error_out), 0);  // `or. r20,r9,r9` (after the assertion cr0 is what the reporter left)
        c.call(dispatched::setjmp_entry, 0x1075ef40);
        c.put32(error_out, c.r(3));
        if (c.r(3) != 0) return fail();
        if (matrix == 0) raise(error_null_matrix, 0x1075ef60);
        if (!(std::int32_t(size_x_in) > 0 && std::int32_t(size_y_in) > 0)) raise(error_bad_size, 0x1075ef7c);
        c.set(3, c.u32(t + ob::sfnt));
        hold();
        c.link(0x1075ef84);
        r[28] = units_per_em(c, c.u32(t + ob::sfnt));
        c.put16(t + ob::units_per_em, r[28]);
        Leftovers left{};
        r[22] = std::uint32_t(distance(wrap(c.u32(matrix)), wrap(c.u32(matrix + 8)), left));
        r[21] = std::uint32_t(distance(wrap(c.u32(matrix + 4)), wrap(c.u32(matrix + 0xc)), left));
        // The sizes in 16.16 pixels over 72, multiplied by the columns' lengths.
        const auto per_72 = [&](std::uint32_t size) {
            const auto high = mulhw(0x38e38e39, wrap(rlwinm(size, 16, 0, 15))) >> 4;
            return std::uint32_t(high + (std::uint32_t(high) >> 31));
        };
        r[30] = multiply(r[22], per_72(size_x_in));
        r[29] = multiply(r[21], per_72(size_y_in));
        c.put32(t + ob::scale_x, r[30]);
        c.put32(t + ob::scale_y, r[29]);
        r[20] = rlwinm(r[28], 16, 0, 15);
        r[23] = r[28] & 0xffff;
        c.put32(t + ob::unit_scale_x, divide(c.u32(t + ob::scale_x), r[20]));
        c.put32(t + ob::unit_scale_y, divide(c.u32(t + ob::scale_y), r[20]));
        r[30] = std::uint32_t(std::int32_t(r[30] + 0x8000) >> 16);  // the sizes rounded to pixels
        r[29] = std::uint32_t(std::int32_t(r[29] + 0x8000) >> 16);
        if (std::int32_t(r[30]) > 0 && std::int32_t(r[29]) > 0) {
            c.put32(matrix, divide(c.u32(matrix), r[22]));
            c.put32(matrix + 8, divide(c.u32(matrix + 8), r[22]));
            c.put32(matrix + 0xc, divide(c.u32(matrix + 0xc), r[21]));
            c.put32(matrix + 4, divide(c.u32(matrix + 4), r[21]));
        } else {
            c.put32(matrix, 0);
            c.put32(matrix + 8, 0);
            c.put32(matrix + 0xc, 0);
            c.put32(matrix + 4, 0);
        }
        c.put32(t + ob::matrix_xx, c.u32(matrix));
        c.put32(t + ob::matrix_xy, c.u32(matrix + 4));
        c.put32(t + ob::matrix_yx, c.u32(matrix + 8));
        c.put32(t + ob::matrix_yy, c.u32(matrix + 0xc));
        const auto identity = is_identity(c.u32(t + ob::matrix_xx), c.u32(t + ob::matrix_xy), c.u32(t + ob::matrix_yx), c.u32(t + ob::matrix_yy));
        c.put32(t + ob::identity, identity);
        c.put32(t + ob::size_x, r[30]);
        c.put32(t + ob::size_y, r[29]);
        if (hint != 0) {
            c.set(3, t);
            hold();
            c.link(0x1075f13c);
            prepare_hinting(c, t);
        }
        c.set({{3, r[30]}, {4, r[23]}, {5, t + ob::ratio_x}});
        hold();
        c.link(0x1075f14c);
        scale_ratio(c, r[30], r[23], t + ob::ratio_x);
        c.set({{3, r[29]}, {4, r[23]}, {5, t + ob::ratio_y}});
        hold();
        c.link(0x1075f15c);
        scale_ratio(c, r[29], r[23], t + ob::ratio_y);
        if (!(t != 0 && c.u32(t + ob::sfnt) != 0)) assertion(c, pool, 0x16, 0x18d, 0x1075f190);
        // The glyph cache of the object: which glyphs of the font have been rendered (a bit each), made at the first transformation.
        if (c.u32(t + ob::glyph_cache) == 0) {
            c.set(3, c.u32(t + ob::sfnt));
            hold();
            c.link(0x1075f1a4);
            r[23] = glyph_limit(c, c.u32(t + ob::sfnt));
            r[22] = c.u32(t + ob::mem);
            c.set({{3, r[22]}, {4, 0x18}});
            hold();
            c.link(0x1075f1b8);
            r[21] = allocate_block(c, r[22], 0x18);
            c.put32(r[21], r[22]);
            c.put32(r[21] + 8, 0xffffffff);
            c.put32(r[21] + 4, 0xffffffff);
            if (std::int32_t(r[23]) > 0x800) r[23] = 0x800;
            c.put32(r[21] + 0xc, rlwinm(std::uint32_t(std::int32_t(r[23] + 7) >> 3), 1, 0, 30));
            c.set({{3, r[22]}, {4, c.u32(r[21] + 0xc)}});
            hold();
            c.link(0x1075f1f4);
            const auto bits = allocate_block(c, r[22], c.u32(r[21] + 0xc));
            c.put32(r[21] + 0x10, bits);
            c.put32(r[21] + 0x14, bits + (std::uint32_t(std::int32_t(c.u32(r[21] + 0xc)) >> 1)));
            c.put32(t + ob::glyph_cache, r[21]);
        }
        const auto cache = c.u32(t + ob::glyph_cache);
        if (!(c.u32(cache + 4) == r[30] && c.u32(cache + 8) == r[30])) {
            const auto count = c.u32(cache + 0xc), buffer = c.u32(cache + 0x10);
            c.set(9, buffer);
            c.put32(cache + 4, r[30]);
            c.put32(cache + 8, r[30]);
            if (std::int32_t(count) > 0) {
                c.set(10, count - 8);
                for (std::uint32_t n = 0; n < count; ++n) c.put8(buffer + n, 0);
            }
        }
        // The resolution: the smaller size in 26.6 over twice the units per em.
        const auto smaller = std::int32_t(r[29]) < std::int32_t(r[30]) ? r[29] : r[30];
        c.put32(t + ob::resolution, std::uint32_t(divide_signed<4>(rlwinm(smaller, 6, 0, 25), 2 * (r[28] & 0xffff))));
        c.put32(t + ob::horizontal_metrics_present, 0);
        c.put32(t + ob::vertical_metrics_present, 0);
        c.put32(t + 0x13c, r[27]);
        c.put32(t + 0x13c, 0);
        r[30] = 0;
        if (c.u32(t + ob::horizontal_metrics_present) == 0 && c.u32(t + ob::vertical_metrics_present) == 0) {
            hold();
            c.link(0x1075f32c);
            caret_slopes(c, c.u32(t + ob::sfnt), frame.at(0x50), frame.at(0x38));
            r[30] = 1;
        }
        // The metric of a halfword scaled by the unit scale (16.16 per unit), then split over the matrix (x, y): the y-axis metrics (ascender,
        // descender, line gap, underline) take x from the matrix's xy column and y from yy, the x-axis ones (the advance) x from xx and y from yx.
        const auto rotate_vertical = [&](std::uint32_t v) {  // the y-axis metrics: (x, y) = (xy*v, yy*v) unless identity
            std::uint32_t x = 0, y = v;
            if (c.u32(t + ob::identity) == 0) {
                x = multiply(c.u32(t + ob::matrix_xy), v);
                y = multiply(c.u32(t + ob::matrix_yy), v);
            }
            return std::pair{x, y};
        };
        const auto rotate_horizontal = [&](std::uint32_t v) {  // the x-axis metrics: (x, y) = (xx*v, yx*v) unless identity
            std::uint32_t x = v, y = 0;
            if (c.u32(t + ob::identity) == 0) {
                x = multiply(c.u32(t + ob::matrix_xx), v);
                y = multiply(c.u32(t + ob::matrix_yx), v);
            }
            return std::pair{x, y};
        };
        if (c.u32(frame.at(0x50)) != 0) {
            // Horizontal metrics: ascender, descender, line gap (y-axis sizes at the vertical unit scale), maximum advance (x-axis size at the
            // horizontal), the caret vector, the underline position and thickness.
            c.put32(t + 0x30, sext16(c.u16(frame.at(0x54))));
            c.put32(t + 0x30, rlwinm(c.u32(t + 0x30), 16, 0, 15));
            c.put32(t + 0x2c, 0);
            c.put32(t + 0x38, sext16(c.u16(frame.at(0x56))));
            c.put32(t + 0x38, rlwinm(c.u32(t + 0x38), 16, 0, 15));
            c.put32(t + 0x34, 0);
            c.put32(t + 0x40, sext16(c.u16(frame.at(0x58))));
            c.put32(t + 0x40, rlwinm(c.u32(t + 0x40), 16, 0, 15));
            c.put32(t + 0x3c, 0);
            c.put32(t + 0x44, c.u16(frame.at(0x5a)));
            c.put32(t + 0x44, rlwinm(c.u32(t + 0x44), 16, 0, 15));
            c.put32(t + 0x48, 0);
            c.put32(t + 0x4c, c.u32(frame.at(0x5c)));
            c.put32(t + 0x50, c.u32(frame.at(0x60)));
            c.put32(t + ob::horizontal_metrics_present, 1);
            if (c.u32(t + ob::identity) == 0) {
                r[22] = c.u32(t + 0x50);
                r[20] = c.u32(t + 0x4c);
                const auto first = multiply(c.u32(t + ob::matrix_xy), r[22]) + multiply(c.u32(t + ob::matrix_xx), r[20]);
                c.put32(t + 0x4c, first);
                const auto second = multiply(c.u32(t + ob::matrix_yy), r[22]) + multiply(c.u32(t + ob::matrix_yx), r[20]);
                c.put32(t + 0x50, second);
            }
            // y-axis metrics at the vertical unit scale: ascender (0x2c, 0x30), descender (0x34, 0x38), line gap (0x3c, 0x40).
            for (const auto& [from, to] : std::array<std::array<std::uint32_t, 2>, 3>{{{0x54, 0x2c}, {0x56, 0x34}, {0x58, 0x3c}}}) {
                const auto scaled = multiply(rlwinm(sext16(c.u16(frame.at(from))), 16, 0, 15), c.u32(t + ob::unit_scale_y));
                const auto [x, y] = rotate_vertical(scaled);
                c.put32(t + to, x);
                c.put32(t + to + 4, y);
            }
            {
                const auto scaled = multiply(rlwinm(c.u16(frame.at(0x5a)), 16, 0, 15), c.u32(t + ob::unit_scale_x));
                const auto [x, y] = rotate_horizontal(scaled);
                c.put32(t + 0x44, x);
                c.put32(t + 0x48, y);
            }
            for (const auto& [from, to] : std::array<std::array<std::uint32_t, 2>, 2>{{{0x64, 0x54}, {0x66, 0x5c}}}) {
                const auto scaled = multiply(rlwinm(sext16(c.u16(frame.at(from))), 16, 0, 15), c.u32(t + ob::unit_scale_y));
                const auto [x, y] = rotate_vertical(scaled);
                c.put32(t + to, x);
                c.put32(t + to + 4, y);
            }
        }
        if (c.u32(frame.at(0x38)) != 0) {
            c.put32(t + 0x68, sext16(c.u16(frame.at(0x3c))));
            c.put32(t + 0x68, rlwinm(c.u32(t + 0x68), 16, 0, 15));
            c.put32(t + 0x6c, 0);
            c.put32(t + 0x70, sext16(c.u16(frame.at(0x3e))));
            c.put32(t + 0x70, rlwinm(c.u32(t + 0x70), 16, 0, 15));
            c.put32(t + 0x74, 0);
            c.put32(t + 0x78, sext16(c.u16(frame.at(0x40))));
            c.put32(t + 0x78, rlwinm(c.u32(t + 0x78), 16, 0, 15));
            c.put32(t + 0x7c, 0);
            c.put32(t + 0x84, c.u16(frame.at(0x42)));
            c.put32(t + 0x84, rlwinm(c.u32(t + 0x84), 16, 0, 15));
            c.put32(t + 0x80, 0);
            c.put32(t + 0x88, c.u32(frame.at(0x44)));
            c.put32(t + 0x8c, c.u32(frame.at(0x48)));
            c.put32(t + ob::vertical_metrics_present, 1);
            if (c.u32(t + ob::identity) == 0) {
                r[22] = c.u32(t + 0x8c);
                r[20] = c.u32(t + 0x88);
                const auto first = multiply(c.u32(t + ob::matrix_xy), r[22]) + multiply(c.u32(t + ob::matrix_xx), r[20]);
                c.put32(t + 0x88, first);
                const auto second = multiply(c.u32(t + ob::matrix_yy), r[22]) + multiply(c.u32(t + ob::matrix_yx), r[20]);
                c.put32(t + 0x8c, second);
            }
            // The vertical metrics: three x-axis sizes at the horizontal unit scale, then the last (a y-axis size) at the vertical.
            for (const auto& [from, to] : std::array<std::array<std::uint32_t, 2>, 3>{{{0x3c, 0x68}, {0x3e, 0x70}, {0x40, 0x78}}}) {
                const auto scaled = multiply(rlwinm(sext16(c.u16(frame.at(from))), 16, 0, 15), c.u32(t + ob::unit_scale_x));
                const auto [x, y] = rotate_horizontal(scaled);
                c.put32(t + to, x);
                c.put32(t + to + 4, y);
            }
            {
                const auto scaled = multiply(rlwinm(c.u16(frame.at(0x42)), 16, 0, 15), c.u32(t + ob::unit_scale_y));
                const auto [x, y] = rotate_vertical(scaled);
                c.put32(t + 0x80, x);
                c.put32(t + 0x84, y);
            }
        }
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != 0x1075ef40 || c.sp() != frame.base()) throw;
        c.put32(error_out, c.r(3));
        fail();
    }
}

// 0x1075f870: what a rendering leaves in the object, by level. Level 0 and up: the glyph (deleted). Above 0: the bitmap (back to the
// allocator's scratch slot 5 when it is that buffer, else released), and the block the object owns at +0xe0. From 100: slot 5's buffer itself
// when it has grown past 0x100 bytes. Above 1: the hinter's handle. The level is 1 before each render, 0 after one that keeps nothing (the
// renderer's cmd bit 4 clear), 2 for the delete.
void release_glyph(Cpu& c, std::uint32_t t, std::int32_t level) {
    Frame frame(c, 0x1075f870, 0x40, {.first_gpr = 30});
    c.set({{30, std::uint32_t(level)}, {31, t}});
    const auto glyph = c.u32(t + ob::glyph);
    c.set(3, glyph);
    c.link(0x1075f894);
    delete_glyph(c, glyph);
    c.put32(t + ob::glyph, 0);
    if (!(level > 0)) return;
    const auto slot_five = [&](std::uint32_t mem) { return mem + 4 * 5; };
    auto mem = c.u32(t + ob::mem);
    const auto bitmap = c.u32(t + ob::bitmap);
    c.set({{3, mem}, {4, bitmap}});
    if (bitmap == c.u32(slot_five(mem + tsimem::scratch_pointer))) {
        c.put32(slot_five(mem + tsimem::scratch_free), 1);
    } else {
        c.link(0x1075f8c8);
        release_block(c, mem, bitmap);
    }
    c.put32(t + ob::bitmap, 0);
    if (level > 99) {
        mem = c.u32(t + ob::mem);
        if (c.u32(slot_five(mem + tsimem::scratch_size)) > 0x100) {
            const auto buffer = c.u32(slot_five(mem + tsimem::scratch_pointer));
            c.set({{3, mem}, {4, buffer}});
            c.link(0x1075f8f0);
            release_block(c, mem, buffer);
            c.put32(slot_five(c.u32(t + ob::mem) + tsimem::scratch_pointer), 0);
            c.put32(slot_five(c.u32(t + ob::mem) + tsimem::scratch_size), 0);
            c.put32(slot_five(c.u32(t + ob::mem) + tsimem::scratch_free), 1);
        }
    }
    const auto block = c.u32(t + ob::bitmap_block);
    if (block != 0) {
        c.set({{3, c.u32(t + ob::mem)}, {4, block}});
        c.link(0x1075f924);
        release_block(c, c.u32(t + ob::mem), block);
        c.put32(t + ob::bitmap_block, 0);
    }
    if (level > 1) {
        c.set(3, c.u32(t + ob::hint_handle));
        c.call(hinter_delete, 0x1075f93c);
        const auto code = c.r(3);
        c.put32(t + ob::hint_handle, 0);
        c.put32(t + ob::hint_size_y, 0xffffffff);
        c.put32(t + ob::hint_size_x, 0xffffffff);
        if (code != 0) {
            c.set({{3, c.u32(t + ob::mem)}, {4, code}});
            c.link(0x1075f960);
            raise_error(c, c.u32(t + ob::mem), code);
        }
    }
}

// 0x1075f980 (the purge): release_glyph under the setjmp, `error` required.
void purge(Cpu& c, std::uint32_t t, std::uint32_t level, std::uint32_t error_out) {
    Frame frame(c, 0x1075f980, 0x50, {.first_gpr = 29});
    const auto pool = c.toc(toc::t2k_pool);
    c.set({{29, t}, {30, level}, {31, error_out}});
    if (error_out == 0) assertion(c, pool, 0, 0x21c, 0x1075f9bc);
    const auto mem = c.u32(t + ob::mem);
    try {
        c.set(3, mem + tsimem::jump_buffer);
        if (error_out != 0) c.compare(0, std::int32_t(error_out), 0);  // `or. r31,r5,r5`
        c.call(dispatched::setjmp_entry, 0x1075f9c8);
        c.put32(error_out, c.r(3));
        if (c.r(3) == 0) {
            c.set({{3, t}, {4, level}});
            c.link(0x1075f9e0);
            release_glyph(c, t, std::int32_t(level));
            return;
        }
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != 0x1075f9c8 || c.sp() != frame.base()) throw;
        c.put32(error_out, c.r(3));
    }
    c.set(3, c.u32(t + ob::mem));
    c.link(0x1075f9ec);
    free_blocks(c, c.u32(t + ob::mem));
}

// 0x1075fa10 RenderGlyph(t2k, index, depth, shift_x, shift_y, quality, cmd): one glyph, or (recursively) one component of a composite.
// cmd bits: 1 grid-fit the metrics, 2 scan convert, 4 keep the glyph and bitmap (no release at the end), 8 `index` is a glyph number (else a
// character code through the cmap), 0x10 offset the points by the two shifts, 0x20 passed on to the scan converter, 0x80 hint, 0x100 the
// colour table (the caller's), 0x200 the gray mode. The result is in the object: +0x94.. the advance, +0xd0..+0xdc the bitmap and its
// geometry, +0xa4/+0xa8 the bearings; +0xcc the glyph number (not for a component).
//
// The registers r13-r31 are kept in `r`, named by the original's use (they change role: r15 is the index, then the point count; r14 the
// glyph, then in a composite the size until the components are done), because the allocator's release and the scan converter's leaver save them.
void render_glyph(Cpu& c, std::uint32_t t2k_in, std::uint32_t index_in, std::uint32_t depth_in, std::uint32_t shift_x_in, std::uint32_t shift_y_in,
                  std::uint32_t quality_in, std::uint32_t command_in) {
    Frame frame(c, 0x1075fa10, 0x120, {.first_gpr = 13});
    std::array<std::uint32_t, 32> r{};
    for (unsigned n = 13; n <= 31; ++n) r[n] = c.r(n);
    const auto hold = [&] { for (unsigned n = 13; n <= 31; ++n) c.set(n, r[n]); };
    // r13 the command's low half, r14 the glyph, r15 the index, r28 the object, r29 the depth, r30 and r31 the shifts.
    r[28] = t2k_in;
    r[15] = index_in;
    r[29] = depth_in;
    r[30] = shift_x_in;
    r[31] = shift_y_in;
    const auto t = [&] { return r[28]; };
    const auto pool = [&] { return c.toc(toc::t2k_pool); };
    const auto mem = [&] { return c.u32(t() + ob::mem); };
    const auto quality_at = frame.caller() + 0x2f;  // the original stores its sixth argument byte in the caller's parameter area
    c.put8(quality_at, quality_in);
    const auto raise = [&](std::uint32_t code, std::uint32_t back) {
        c.set({{3, mem()}, {4, code}});
        hold();
        c.link(back);
        raise_error(c, mem(), code);
    };
    const auto multiply = [&](std::uint32_t a, std::uint32_t b) {
        Leftovers left{};
        return std::uint32_t(fix_multiply(wrap(a), wrap(b), left));
    };
    const auto divide_fixed = [&](std::uint32_t a, std::uint32_t b) {
        Leftovers left{};
        return std::uint32_t(fix_divide(wrap(a), wrap(b), left));
    };
    const auto scale = [&](std::uint32_t source, std::uint32_t destination, std::uint32_t count, std::uint32_t record, std::uint32_t back) {
        c.set({{3, source}, {4, destination}, {5, count}, {6, record}});
        hold();
        c.link(back);
        scale_coordinates(c, source, destination, std::int32_t(count), record);
    };
    // The command, adjusted: hinting needs a hinted font and a size the font allows (else the grid-fit bit goes on instead).
    std::uint32_t r9 = command_in;
    std::uint32_t hinted = command_in & 0x80;  // 0xac(r1)
    std::uint32_t digit = 0;                   // 0xb4(r1)
    if (hinted != 0) {
        const auto head = c.u32(c.u32(t() + ob::sfnt) + sfnt::head);
        if (head == 0 || c.i16(head + head::glyph_data_format) == 0x7d0 || c.i16(head + head::glyph_data_format) == 0x7d1) {
            hinted = 0;
            r9 = (r9 | 1) & 0xffff;
        } else {
            const auto smallest = std::int32_t(c.i16(head + head::lowest_rec_ppem));
            r9 = rlwinm(r9, 0, 16, 30);
            if (c.i32(t() + ob::size_x) < smallest || c.i32(t() + ob::size_y) < smallest) {
                hinted = 0;
                r9 = rlwinm(r9, 0, 25, 23) & 0xffff;
            }
        }
    }
    std::uint32_t grid = 0;  // 0xc8(r1)
    if (hinted == 0 && (r9 & 0xffff & 0x281) != 0) grid = 1;
    r[13] = r9 & 0xffff;
    const std::uint32_t quality_mode = (r9 >> 9) & 1 ? 3 : 0;  // 0xa8(r1)
    c.set(9, r9);
    if (c.u8(quality_at) > 5) raise(error_bad_quality, 0x1075fb10);
    const std::uint32_t units = c.u16(t() + ob::units_per_em);  // 0xb0(r1)
    c.set({{3, t()}, {4, 1}});
    hold();
    c.link(0x1075fb24);
    release_glyph(c, t(), 1);
    const auto font = [&] { return c.u32(t() + ob::sfnt); };
    const auto advance_at = frame.at(0x4a), vertical_at = frame.at(0x48);
    if (r[13] & 8) {
        c.set({{3, font()}, {4, r[15]}, {5, sext8(hinted)}, {6, advance_at}, {7, vertical_at}});
        hold();
        c.link(0x1075fb48);
        r[14] = load_glyph(c, font(), r[15], sext8(hinted), advance_at, vertical_at);
        c.set({{3, font()}, {4, r[15] & 0xffff}});
        hold();
        c.link(0x1075fb5c);
        digit = is_digit_glyph(c, font(), r[15] & 0xffff);
        if (r[29] == 0) c.put16(t() + ob::glyph_number, r[15]);
    } else {
        c.set({{3, font()}, {4, r[15] & 0xffff}});
        hold();
        c.link(0x1075fb7c);
        const auto number = character_glyph(c, font(), r[15] & 0xffff);
        c.set({{3, font()}, {4, number & 0xffff}, {5, sext8(hinted)}, {6, advance_at}, {7, vertical_at}});
        hold();
        c.link(0x1075fb98);
        r[14] = load_glyph(c, font(), number & 0xffff, sext8(hinted), advance_at, vertical_at);
        if (r[29] == 0) c.put16(t() + ob::glyph_number, c.u16(r[14] + 0x76));
        if (std::int32_t(r[15]) >= 0x30 && std::int32_t(r[15]) <= 0x39) digit = 1;
    }
    if (r[14] == 0) raise(error_no_glyph, 0x1075fbd8);
    c.put32(t() + ob::glyph, r[14]);
    // The matrix words a composite's components change, and the sizes: saved here, put back after each component.
    std::array<std::uint32_t, 4> saved_matrix{};
    std::uint32_t saved_instructions = 0, saved_length = 0;
    // r23, r24 the arrays of the points as 26.6 words (x, y), r25 their count with the four phantom points, r15 the points.
    const auto glyph_field = [&](std::uint32_t offset) { return r[14] + offset; };
    if (c.i16(glyph_field(0x36)) < 0) {
        // ---- A composite glyph: its components (flags, glyph, two offsets, an optional scale) rendered into one ----
        c.put32(frame.at(0x4c), 0);
        r[20] = 0;
        r[22] = (((r[13] & 0x291) & 0xff) | 0xc) & 0xffff;
        saved_instructions = c.u32(glyph_field(0x64));
        r[17] = c.u32(glyph_field(0x58));
        saved_length = c.u32(glyph_field(0x68));
        c.put32(glyph_field(0x58), 0);
        c.put32(glyph_field(0x64), 0);
        c.put32(glyph_field(0x68), 0);
        saved_matrix = {c.u32(t() + ob::matrix_xx), c.u32(t() + ob::matrix_xy), c.u32(t() + ob::matrix_yx), c.u32(t() + ob::matrix_yy)};
        r[14] = c.u32(t() + ob::size_x);
        r[19] = c.u32(t() + ob::size_y);
        if (c.u32(t() + ob::hint_mode) == 2 || quality_mode == 3) r[22] = (r[22] & 0xfffffdfe) & 0xffff;  // li r0,-0x202: clears bit 0 (grid) and 0x200
        r[27] = 0;
        for (;;) {
            const auto next_half = [&] {
                const auto value = sext16(c.u16(r[17] + r[27]));
                r[27] += 2;
                return value;
            };
            const auto flags_word = next_half();
            const auto component = next_half() & 0xffff;
            r[21] = flags_word & 0xffff;
            r[20] |= r[21];
            if (r[21] & 1) {
                r[16] = next_half();
                r[15] = next_half();
            } else {
                r[16] = next_half();
                if (r[21] & 2) {
                    const auto low = r[16] & 0xff;
                    r[16] = std::uint32_t(std::int32_t(r[16]) >> 8);
                    r[15] = sext8(low);
                } else {
                    r[15] = r[16] & 0xff;
                    r[16] = std::uint32_t(std::int32_t(r[16]) >> 8) & 0xff;
                }
            }
            r[18] = 0;
            if (r[21] & 2) {
                // The offsets in units of the font, scaled to the size (26.6) and rounded.
                const auto scale_x = rlwinm(c.u32(t() + ob::size_x), 6, 0, 25), scale_y = rlwinm(c.u32(t() + ob::size_y), 6, 0, 25);
                const auto upm = c.u16(t() + ob::units_per_em);
                const auto half = std::uint32_t(std::int32_t(upm) >> 1);
                const auto x = r[16] * scale_x + half, y = r[15] * scale_y + (rlwinm(upm, 31, 17, 31));
                r[16] = std::uint32_t(divide_signed<4>(x, upm));
                r[15] = std::uint32_t(divide_signed<4>(y, upm));
            }
            if (r[21] & 8) {
                r[23] = rlwinm(next_half(), 2, 0, 29);
                r[26] = 0;
                r[25] = 0;
                r[18] = 1;
                r[24] = r[23];
            } else if (r[21] & 0x40) {
                const auto first = next_half();
                const auto second = c.u16(r[17] + r[27]);
                r[26] = 0;
                r[25] = 0;
                r[23] = rlwinm(first, 2, 0, 29);
                r[24] = rlwinm(sext16(second), 2, 0, 29);
                r[18] = 1;
                r[27] += 2;
            } else if (r[21] & 0x80) {
                const auto a = next_half(), b = next_half(), cc = next_half(), d = sext16(c.u16(r[17] + r[27]));
                r[23] = rlwinm(a, 2, 0, 29);
                r[26] = rlwinm(b, 2, 0, 29);
                r[25] = rlwinm(cc, 2, 0, 29);
                r[24] = rlwinm(d, 2, 0, 29);
                r[18] = 1;
                r[27] += 2;
            }
            if (r[18] != 0) {
                c.put32(t() + ob::matrix_xx, r[23]);
                c.put32(t() + ob::matrix_xy, r[26]);
                c.put32(t() + ob::matrix_yx, r[25]);
                c.put32(t() + ob::matrix_yy, r[24]);
                c.put32(t() + ob::identity, is_identity(c.u32(t() + ob::matrix_xx), c.u32(t() + ob::matrix_xy), c.u32(t() + ob::matrix_yx), c.u32(t() + ob::matrix_yy)));
                c.put32(t() + ob::size_x, c.u32(t() + ob::size_x));
                c.put32(t() + ob::size_y, c.u32(t() + ob::size_y));
            }
            // The component, one level down (its command: bits 1 (not with the gray mode or the font's hint mode 2: -0x202 clears 1 and 0x200), 0x10
            // and 0x80 of this one, and 4 and 8: keep, by glyph number). The depth it gets is the one this routine was given: r29 counts the
            // components after the call (so a composite with components is never at depth 0 for the origin normalisation below).
            const auto old_depth = r[29];
            c.set({{3, t()}, {4, component}, {5, old_depth}, {6, r[30]}, {7, r[31]}, {8, 0}, {9, r[22]}});
            r[29] += 1;
            hold();
            c.link(0x1075fe44);
            render_glyph(c, t(), component, old_depth, r[30], r[31], 0, r[22]);
            if (r[18] != 0) {
                c.put32(t() + ob::matrix_xx, saved_matrix[0]);
                c.put32(t() + ob::matrix_xy, saved_matrix[1]);
                c.put32(t() + ob::matrix_yx, saved_matrix[2]);
                c.put32(t() + ob::matrix_yy, saved_matrix[3]);
                c.put32(t() + ob::identity, is_identity(saved_matrix[0], saved_matrix[1], saved_matrix[2], saved_matrix[3]));
                c.put32(t() + ob::size_x, r[14]);
                c.put32(t() + ob::size_y, r[19]);
            }
            const auto piece = c.u32(t() + ob::glyph);
            c.set({{3, frame.at(0x4c)}, {4, piece}, {5, r[21]}, {6, r[16]}, {7, r[15]}});
            hold();
            c.link(0x1075fed0);
            append_glyph(c, frame.at(0x4c), piece, r[21], r[16], r[15]);
            const auto accumulated = c.u32(frame.at(0x4c)), piece_now = c.u32(t() + ob::glyph);
            if (accumulated != piece_now) {
                c.set(3, piece_now);
                hold();
                c.link(0x1075fee4);
                delete_glyph(c, piece_now);
            }
            c.put32(t() + ob::glyph, 0);
            if (!(r[21] & 0x20)) break;
        }
        r[14] = c.u32(frame.at(0x4c));
        c.put32(t() + ob::glyph, r[14]);
        {
            // The components' words and the instructions go back to the allocator (slot 3 when it is the kept block), the glyph keeps the
            // instructions it had.
            const auto owner = mem();
            const auto block = c.u32(r[14] + 0x64);
            if (block == c.u32(owner + tsimem::scratch_pointer + 4 * 3)) {
                c.put32(owner + tsimem::scratch_free + 4 * 3, 1);
            } else {
                c.set({{3, owner}, {4, block}});
                hold();
                c.link(0x1075ff20);
                release_block(c, owner, block);
            }
            c.put32(r[14] + 0x64, 0);
            c.put32(r[14] + 0x68, 0);
            c.put32(r[14] + 0x64, saved_instructions);
            c.put32(r[14] + 0x68, saved_length);
            c.set({{3, mem()}, {4, r[17]}});
            hold();
            c.link(0x1075ff48);
            release_block(c, mem(), r[17]);
        }
        r[15] = sext16(c.u16(r[14] + 0x38));
        r[23] = c.u32(r[14] + 0x50);
        r[24] = c.u32(r[14] + 0x54);
        r[25] = r[15] + 4;
        if (!(r[20] & 0x200)) {
            // Without USE_MY_METRICS the glyph's own advance sets its second phantom point: x[n] = 0, x[n+1] = x[n] + advance, and the 26.6
            // original of x[n+1] is that scaled to the size (rounded to the pixel grid with the grid bits), original x[n] = 0.
            c.put16(c.u32(r[14] + 0x44) + 2 * r[15], 0);
            const auto g = c.u32(t() + ob::glyph);
            const auto xs = c.u32(r[14] + 0x44);
            const auto first = sext16(c.u16(xs + 2 * sext16(c.u16(g + 0x38))));
            c.put16(xs + 2 * sext16(c.u16(r[14] + 0x38)) + 2, c.u16(advance_at) + first);
            const auto n = sext16(c.u16(r[14] + 0x38));
            const auto upm = c.u16(t() + ob::units_per_em);
            const auto x = sext16(c.u16(c.u32(r[14] + 0x44) + 2 * n + 2));
            const auto product = x * rlwinm(c.u32(t() + ob::size_x), 6, 0, 25) + rlwinm(upm, 31, 17, 31);
            c.put32(c.u32(r[14] + 0x50) + 4 * n + 4, std::uint32_t(divide_signed<4>(product, upm)));
            if (r[13] & 0x81) {
                const auto at = c.u32(r[14] + 0x50) + 4 * sext16(c.u16(r[14] + 0x38)) + 4;
                c.put32(at, c.u32(at) + 0x20);
                const auto again = c.u32(r[14] + 0x50) + 4 * sext16(c.u16(r[14] + 0x38)) + 4;
                c.put32(again, c.u32(again) & 0xffffffc0);
            }
            c.put32(c.u32(r[14] + 0x50) + 4 * sext16(c.u16(r[14] + 0x38)), 0);
        }
        const bool scaled = (r[13] & 0x80) || quality_mode == 3 || (c.u32(t() + ob::hint_mode) == 2 && grid != 0);
        if (scaled) {
            // The points from the font's units: 26.6 words scaled to halfwords by units per em / size (16.16).
            r[16] = divide_fixed(units, rlwinm(c.u32(t() + ob::size_x), 6, 0, 25));
            r[17] = divide_fixed(units, rlwinm(c.u32(t() + ob::size_y), 6, 0, 25));
            r[15] = sext16(c.u16(r[14] + 0x38));
            r[22] = 0;
            r[19] = r[23];
            r[18] = r[24];
            r[21] = 0;
            r[25] = r[15] + 4;
            while (std::int32_t(r[22]) < std::int32_t(r[25])) {
                r[26] = c.u32(r[18]);
                r[20] = multiply(c.u32(r[19]), r[16]);
                const auto y = multiply(r[26], r[17]);
                r[18] += 4;
                r[19] += 4;
                r[22] += 1;
                c.put16(c.u32(r[14] + 0x44) + r[21], r[20]);
                c.put16(c.u32(r[14] + 0x48) + r[21], y);
                r[21] += 2;
            }
            if (hinted != 0) {
                const auto hint_font = c.u32(font() + sfnt::hint_font);
                c.set({{3, hint_font}, {4, r[14]}});
                hold();
                c.link(0x1076010c);
                load_glyph_program(c, hint_font, r[14]);
            }
            if (grid != 0) {
                c.set(3, t());
                hold();
                c.link(0x10760120);
                prepare_hinting(c, t());
                c.put16(frame.at(0x78), c.u16(r[14] + 0x36));
                if (std::int32_t(r[15]) > 0x7d00) raise(error_too_many_points, 0x1076013c);
                c.put16(frame.at(0x7a), r[15]);
                c.put32(frame.at(0x7c), c.u32(r[14] + 0x3c));
                c.put32(frame.at(0x80), c.u32(r[14] + 0x40));
                c.put32(frame.at(0x84), c.u32(r[14] + 0x44));
                c.put32(frame.at(0x88), c.u32(r[14] + 0x48));
                c.put32(frame.at(0x8c), c.u32(r[14] + 0x4c));
                c.put32(frame.at(0x90), r[23]);
                c.put32(frame.at(0x94), r[24]);
                const auto nonzero = c.u8(quality_at) != 0 ? 1u : 0u;
                c.set({{3, c.u32(t() + ob::hint_handle)}, {4, frame.at(0x78)}, {5, quality_mode}, {6, sext16(digit)}, {7, sext16(c.u16(r[14] + 0x34))},
                       {8, nonzero}, {9, 4}});
                hold();
                c.call(hinter_glyph, 0x107601a4);
                if (c.r(3) != 0) raise(c.r(3), 0x107601b4);
            }
        }
    } else {
        // ---- A simple glyph ----
        r[15] = sext16(c.u16(r[14] + 0x38));
        r[23] = c.u32(r[14] + 0x50);
        r[24] = c.u32(r[14] + 0x54);
        r[25] = r[15] + 4;
        const auto x_points = [&] { return c.u32(r[14] + 0x44); };
        const auto y_points = [&] { return c.u32(r[14] + 0x48); };
        if ((hinted != 0 || grid != 0) && !(r[13] & 0x10) && std::int32_t(r[15]) > 0) {
            if (hinted != 0) {
                scale(x_points(), r[23], r[25], t() + ob::ratio_x, 0x10760210);
                scale(y_points(), r[24], r[25], t() + ob::ratio_y, 0x10760224);
                const auto hint_font = c.u32(font() + sfnt::hint_font);
                c.set({{3, hint_font}, {4, r[14]}});
                hold();
                c.link(0x10760234);
                load_glyph_program(c, hint_font, r[14]);
            }
            if (grid != 0) {
                c.set(3, t());
                hold();
                c.link(0x10760248);
                prepare_hinting(c, t());
                c.put16(frame.at(0x50), c.u16(r[14] + 0x36));
                if (std::int32_t(r[15]) > 0x7d00) raise(error_too_many_points, 0x10760264);
                c.put16(frame.at(0x52), r[15]);
                r[16] = r[25];
                c.put32(frame.at(0x54), c.u32(r[14] + 0x3c));
                c.put32(frame.at(0x58), c.u32(r[14] + 0x40));
                c.put32(frame.at(0x5c), x_points());
                c.put32(frame.at(0x60), y_points());
                c.put32(frame.at(0x64), c.u32(r[14] + 0x4c));
                c.put32(frame.at(0x68), r[23]);
                c.put32(frame.at(0x6c), r[24]);
                scale(x_points(), r[23], r[25], t() + ob::ratio_x, 0x107602ac);
                if (quality_mode != 3) scale(y_points(), r[24], r[25], t() + ob::ratio_y, 0x107602cc);
                const auto nonzero = c.u8(quality_at) != 0 ? 1u : 0u;
                c.set({{3, c.u32(t() + ob::hint_handle)}, {4, frame.at(0x50)}, {5, quality_mode}, {6, sext16(digit)}, {7, sext16(c.u16(r[14] + 0x34))},
                       {8, nonzero}, {9, 4}});
                hold();
                c.call(hinter_glyph, 0x10760300);
                r[17] = c.r(3);
                if (r[17] == 0xffffff9c) {
                    r[17] = 0;
                    scale(y_points(), r[24], r[16], t() + ob::ratio_y, 0x10760324);
                }
                if (r[17] != 0) raise(r[17], 0x10760338);
            }
        } else {
            scale(x_points(), r[23], r[25], t() + ob::ratio_x, 0x10760350);
            scale(y_points(), r[24], r[25], t() + ob::ratio_y, 0x10760364);
            if (r[13] & 0x10) {
                // Offset by the shifts (signed bytes): x added, y subtracted.
                if (sext8(r[30]) != 0)
                    for (std::int32_t n = 0; n < std::int32_t(r[15]); ++n) c.put32(r[23] + 4 * std::uint32_t(n), c.u32(r[23] + 4 * std::uint32_t(n)) + sext8(r[30]));
                if (sext8(r[31]) != 0)
                    for (std::int32_t n = 0; n < std::int32_t(r[15]); ++n) c.put32(r[24] + 4 * std::uint32_t(n), c.u32(r[24] + 4 * std::uint32_t(n)) - sext8(r[31]));
            }
        }
    }
    // ---- Common: the advances, the phantom points, the transformation, the metrics, the scan conversion ----
    c.put32(t() + ob::advance_16_x, c.u16(advance_at));
    c.put32(t() + ob::advance_16_x, rlwinm(c.u32(t() + ob::advance_16_x), 16, 0, 15));
    c.put32(t() + ob::advance_16_y, 0);
    c.put32(t() + 0xb8, 0);
    c.put32(t() + 0xbc, c.u16(vertical_at));
    c.put32(t() + 0xbc, rlwinm(c.u32(t() + 0xbc), 16, 0, 15));
    if (r[29] == 0) {
        const auto origin = 4 * r[15];
        const auto flags = r[13] & 0x140;
        std::uint32_t pivot = c.u32(r[23] + origin);
        if (pivot != 0 && std::int32_t(r[25]) > 0)
            for (std::int32_t n = 0; n < std::int32_t(r[25]); ++n) c.put32(r[23] + 4 * std::uint32_t(n), c.u32(r[23] + 4 * std::uint32_t(n)) - pivot);
        pivot = c.u32(r[24] + origin);
        if (pivot != 0 && std::int32_t(r[25]) > 0)
            for (std::int32_t n = 0; n < std::int32_t(r[25]); ++n) c.put32(r[24] + 4 * std::uint32_t(n), c.u32(r[24] + 4 * std::uint32_t(n)) - pivot);
        if (!(r[13] & 0x10)) {
            // The first two phantom points on the pixel grid; with the grid-fit bits the outline shifts by half the rounding.
            const auto at = r[23] + origin;
            const auto left = c.u32(at), right = c.u32(at + 4);
            const auto rounded_left = (left + 0x20) & 0xffffffc0;
            const auto rounded_right = rounded_left + ((right - left + 0x20) & 0xffffffc0);
            c.put32(at, rounded_left);
            c.put32(at + 4, rounded_right);
            if (flags != 0 && std::int32_t(r[15]) > 0) {
                const auto shift = std::uint32_t(std::int32_t((rounded_left - left) + rounded_right - right) >> 1);
                if (shift != 0)
                    for (std::int32_t n = 0; n < std::int32_t(r[15]); ++n) c.put32(r[23] + 4 * std::uint32_t(n), c.u32(r[23] + 4 * std::uint32_t(n)) + shift);
            }
        }
    }
    c.put32(t() + ob::advance_16_x, multiply(c.u32(t() + ob::advance_16_x), c.u32(t() + ob::unit_scale_x)));
    c.put32(t() + 0xbc, multiply(c.u32(t() + 0xbc), c.u32(t() + ob::unit_scale_y)));
    r[18] = r[23] + 4 * r[15];
    r[19] = r[24] + 4 * r[15];
    for (const auto offset : {0u, 4u, 8u, 0xcu}) {
        c.put32(r[18] + offset, rlwinm(c.u32(r[18] + offset), 10, 0, 21));
        c.put32(r[19] + offset, rlwinm(c.u32(r[19] + offset), 10, 0, 21));
    }
    if (c.u32(t() + ob::identity) == 0) {
        r[27] = c.u32(t() + ob::matrix_xy);
        r[26] = c.u32(t() + ob::matrix_xx);
        r[29] = c.u32(t() + ob::matrix_yx);
        r[30] = c.u32(t() + ob::matrix_yy);
        if (r[27] == 0 && r[29] == 0) {
            r[16] = r[23];
            r[15] = r[24];
            for (r[17] = 0; std::int32_t(r[17]) < std::int32_t(r[25]); ++r[17], r[15] += 4, r[16] += 4) {
                r[20] = c.u32(r[15]);
                c.put32(r[16], multiply(r[26], c.u32(r[16])));
                c.put32(r[15], multiply(r[30], r[20]));
            }
        } else {
            r[16] = r[23];
            r[15] = r[24];
            for (r[20] = 0; std::int32_t(r[20]) < std::int32_t(r[25]); ++r[20], r[15] += 4, r[16] += 4) {
                r[22] = c.u32(r[15]);
                r[21] = c.u32(r[16]);
                r[17] = multiply(r[27], r[22]);
                c.put32(r[16], multiply(r[26], r[21]) + r[17]);
                r[17] = multiply(r[30], r[22]);
                c.put32(r[15], multiply(r[29], r[21]) + r[17]);
            }
        }
        r[15] = c.u32(t() + ob::advance_16_x);
        c.put32(t() + ob::advance_16_x, multiply(r[26], r[15]));
        c.put32(t() + ob::advance_16_y, multiply(r[29], r[15]));
        r[15] = c.u32(t() + 0xbc);
        c.put32(t() + 0xb8, multiply(r[27], r[15]));
        c.put32(t() + 0xbc, multiply(r[30], r[15]));
    }
    c.put32(t() + ob::advance_x, c.u32(r[18] + 4) - c.u32(r[18]));
    c.put32(t() + ob::advance_y, c.u32(r[19] + 4) - c.u32(r[19]));
    c.put32(t() + 0xb0, negate(c.u32(r[18] + 0xc) - c.u32(r[18] + 8)));
    c.put32(t() + 0xb4, negate(c.u32(r[19] + 0xc) - c.u32(r[19] + 8)));
    for (const auto offset : {0u, 4u, 8u, 0xcu}) {
        c.put32(r[18] + offset, std::uint32_t(std::int32_t(c.u32(r[18] + offset)) >> 10));
        c.put32(r[19] + offset, std::uint32_t(std::int32_t(c.u32(r[19] + offset)) >> 10));
    }
    for (const auto field : {ob::bitmap_block, ob::bitmap, ob::bitmap_stride, ob::bitmap_width, ob::bitmap_height, ob::top, ob::left, 0xc0u, 0xc4u}) c.put32(t() + field, 0);
    c.put8(r[14] + 0x74, 1);
    if (c.u32(t() + ob::identity) != 0) {
        // A glyph the hinter's size limit excludes from the cache of scan-converted bitmaps.
        std::uint32_t limit = 0xff;
        if (hinted != 0) {
            const auto from_font = c.u32(c.u32(font() + sfnt::hint_font) + 0x7c) & 0xff;
            if (from_font != 0xff) limit = from_font;
        }
        if (c.i32(t() + ob::size_x) > std::int32_t(limit)) c.put8(r[14] + 0x74, 0);
    }
    if (r[13] & 2) {
        if (c.i16(r[14] + 0x38) > 0) {
            r[15] = 0;
            std::uint32_t grayscale = 0;
            if (sext8(c.u8(t() + ob::hint_byte)) != 0 && (r[13] & 1) && c.i32(t() + ob::hint_size_x) <= 0x18) grayscale = 1;
            r[16] = grayscale;
            if (c.i32(r[14] + 0xc) > 0) {
                hold();
                assertion(c, pool(), 0x33, 0x412, 0x10760b88);
            } else {
                std::uint32_t keep = c.u8(r[14] + 0x74), flag = keep;
                r[17] = c.u16(t() + ob::glyph_number);
                const auto cached = [&](std::uint32_t at_offset) {
                    // Bit `glyph number` of the object's table of glyphs already rendered, 1 for numbers past the table.
                    if (r[17] >= 0x800) return true;
                    const auto table = c.u32(c.u32(t() + ob::glyph_cache) + at_offset);
                    return (c.u8(table + ((r[17] >> 3) & 0x1fff)) & (1u << (r[17] & 7))) != 0;
                };
                if (c.u8(quality_at) == 0 && c.u32(t() + ob::identity) != 0 && sext8(keep) != 0) {
                    if (cached(0x10)) flag = 0;
                    if (cached(0x10)) keep = 0;
                }
                c.put32(frame.at(0x38), sext8(c.u16(r[14] + 0x34)));
                c.put32(frame.at(0x3c), flag);
                c.put32(frame.at(0x40), keep);
                c.put32(frame.at(0x44), c.u32(t() + ob::resolution));
                c.set({{3, mem()}, {4, sext16(c.u16(r[14] + 0x36))}, {5, c.u32(r[14] + 0x3c)}, {6, c.u32(r[14] + 0x40)}, {7, r[23]}, {8, r[24]},
                       {9, c.u32(r[14] + 0x4c)}, {10, c.u8(quality_at)}});
                hold();
                c.call(scan_new, 0x10760c78);
                r[15] = c.r(3);
                c.set({{3, r[15]}, {4, r[16]}, {5, r[13] & 0x20}});
                hold();
                c.call(scan_run, 0x10760c8c);
                const auto bitmap_record = r[15];
                c.put32(t() + ob::bitmap_width, c.u32(bitmap_record + 4) - c.u32(bitmap_record));
                c.put32(t() + ob::bitmap_height, c.u32(bitmap_record + 0xc) - c.u32(bitmap_record + 8));
                c.put32(t() + ob::top, c.u32(bitmap_record + 0x10));
                c.put32(t() + ob::left, c.u32(bitmap_record + 0x14));
                c.put32(t() + 0xc0, c.u32(bitmap_record + 0x10));
                c.put32(t() + 0xc4, c.u32(bitmap_record + 0x14));
                c.put32(t() + ob::bitmap_stride, c.u32(bitmap_record + 0x18));
                if (c.u8(quality_at) == 0 && c.u32(t() + ob::identity) != 0) {
                    const auto mark = [&](std::uint32_t flag_offset, std::uint32_t table_offset) {
                        if (c.u32(bitmap_record + flag_offset) == 0 && r[17] < 0x800) {
                            const auto table = c.u32(c.u32(t() + ob::glyph_cache) + table_offset);
                            const auto at = table + (r[17] >> 3);
                            c.put8(at, c.u8(at) | (1u << (r[17] & 7)));
                        }
                    };
                    mark(0x2204, 0x10);
                    mark(0x2208, 0x14);
                }
            }
            c.put32(t() + 0xc4, c.u32(t() + 0xc4) - c.u32(r[18] + 8));
            c.put32(t() + 0xc0, c.u32(t() + 0xc0) - c.u32(r[19] + 8));
            if (c.i32(r[14] + 0xc) > 0) {
                hold();
                assertion(c, pool(), 0x33, 0x43c, 0x10760da0);
            } else {
                const auto bitmap_record = r[15];
                c.put32(t() + ob::bitmap, c.u32(bitmap_record + 0x1c));
                c.put32(bitmap_record + 0x1c, 0);
                c.put32(t() + ob::bitmap_block, 0);
            }
            if (c.u32(r[18]) != 0) c.put32(t() + ob::left, c.u32(t() + ob::left) - c.u32(r[18]));
            if (c.u32(r[19]) != 0) c.put32(t() + ob::top, c.u32(t() + ob::top) - c.u32(r[19]));
            c.put32(t() + ob::left, c.u32(t() + ob::left) & 0xffffffc0);
            c.put32(t() + ob::top, c.u32(t() + ob::top) & 0xffffffc0);
            c.set(3, r[15]);
            hold();
            c.call(scan_release, 0x10760e08);
        }
    }
    if (!(r[13] & 4)) {
        c.set({{3, t()}, {4, 0}});
        hold();
        c.link(0x10760e1c);
        release_glyph(c, t(), 0);
    }
}

// 0x10760e30 (the character renderer, the game's): RenderGlyph under the setjmp with `error` required. A bad error stamp raises 0x2723
// (the object was not made by NewT2K, or a longjmp left it half done), cmd bit 0x40 with 1 or 0x80 asserts, and cmd bit 0x100 turns the
// result into the colour table (the caller of that is block t2kextra). The sizes of the arguments: (t2k, character, shift_x, shift_y, quality, cmd, error).
void render_character(Cpu& c, std::uint32_t t, std::uint32_t index, std::uint32_t shift_x, std::uint32_t shift_y, std::uint32_t quality, std::uint32_t cmd,
                      std::uint32_t error_out) {
    Frame frame(c, 0x10760e30, 0x60, {.first_gpr = 24});
    const auto pool = c.toc(toc::t2k_pool);
    const auto hold = [&] { c.set({{24, index}, {25, shift_x}, {26, shift_y}, {27, quality}, {28, cmd}, {29, error_out}, {30, pool}, {31, t}}); };
    hold();
    if (error_out == 0) assertion(c, pool, 0, 0x513, 0x10760e74);
    const auto mem = c.u32(t + ob::mem);
    try {
        c.set(3, mem + tsimem::jump_buffer);
        if (error_out != 0) c.compare(0, std::int32_t(error_out), 0);  // `or. r29,r9,r9`
        c.call(dispatched::setjmp_entry, 0x10760e80);
        c.put32(error_out, c.r(3));
        if (c.r(3) == 0) {
            if (c.u32(c.u32(t + ob::mem) + tsimem::error_stamp) != tsimem::error_stamp_initial) {
                c.set({{3, c.u32(t + ob::mem)}, {4, error_bad_stamp}});
                c.link(0x10760ea8);
                raise_error(c, c.u32(t + ob::mem), error_bad_stamp);
            }
            const auto command = cmd & 0xffff;
            c.set(29, command);
            if ((command & 0x81) != 0 && (command & 0x40) != 0) assertion(c, pool, 0x68, 0x51d, 0x10760edc);
            c.put32(t + ob::error, 0);
            c.set({{3, t}, {4, index}, {5, 0}, {6, shift_x}, {7, shift_y}, {8, quality}, {9, cmd}});
            c.link(0x10760f04);
            render_glyph(c, t, index, 0, shift_x, shift_y, quality, cmd);
            c.put32(t + ob::rendered, 1);
            c.put32(t + 0xac, 0);
            if (command & 0x100) {
                c.set(3, t);
                c.link(0x10760f24);
                render_color_table(c, t);
            }
            return;
        }
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != 0x10760e80 || c.sp() != frame.base()) throw;
        c.put32(error_out, c.r(3));
    }
    c.set(3, c.u32(t + ob::mem));
    c.link(0x10760f30);
    free_blocks(c, c.u32(t + ob::mem));
}

// 0x10760f50 DeleteT2K(t2k, error): under the setjmp, the stamps checked (0x2717), what a rendering holds released (level 2), the hint data the
// sfnt keeps (+0x38), the object's two buffers, the glyph cache's bitmap tables and record, the allocator's scratch buffers and the object itself.
void delete_t2k(Cpu& c, std::uint32_t t, std::uint32_t error_out) {
    Frame frame(c, 0x10760f50, 0x40, {.first_gpr = 30});
    const auto pool = c.toc(toc::t2k_pool);
    std::uint32_t r31 = error_out;
    const auto hold = [&] { c.set({{30, t}, {31, r31}}); };
    hold();
    if (error_out == 0) assertion(c, pool, 0, 0x554, 0x10760f84);
    const auto mem = c.u32(t + ob::mem);
    try {
        c.set(3, mem + tsimem::jump_buffer);
        if (error_out != 0) c.compare(0, std::int32_t(error_out), 0);  // `or. r31,r4,r4`
        c.call(dispatched::setjmp_entry, 0x10760f90);
        c.put32(error_out, c.r(3));
        if (c.r(3) == 0) {
            const auto release = [&](std::uint32_t owner, std::uint32_t block, std::uint32_t back) {
                c.set({{3, owner}, {4, block}});
                hold();
                c.link(back);
                release_block(c, owner, block);
            };
            if (c.u32(t + ob::stamp) != 0x5a1234a5 || c.u32(t + ob::end_stamp) != 0xa5fedc5a) {
                c.set({{3, c.u32(t + ob::mem)}, {4, error_bad_object}});
                hold();
                c.link(0x10760fc8);
                raise_error(c, c.u32(t + ob::mem), error_bad_object);
            }
            c.set({{3, t}, {4, 2}});
            hold();
            c.link(0x10760fd4);
            release_glyph(c, t, 2);
            release(c.u32(t + ob::mem), c.u32(c.u32(t + ob::sfnt) + 0x38), 0x10760fe4);
            c.put32(c.u32(t + ob::sfnt) + 0x38, 0);
            release(c.u32(t + ob::mem), c.u32(t + ob::buffer_20), 0x10760ffc);
            release(c.u32(t + ob::mem), c.u32(t + ob::buffer_24), 0x10761008);
            r31 = c.u32(t + ob::glyph_cache);
            if (r31 != 0) {
                release(c.u32(r31), c.u32(r31 + 0x10), 0x10761020);
                release(c.u32(r31), r31, 0x1076102c);
            }
            c.set(3, c.u32(t + ob::mem));
            hold();
            c.link(0x10761034);
            scratch_release_all(c, c.u32(t + ob::mem));
            release(c.u32(t + ob::mem), t, 0x10761040);
            return;
        }
    } catch (const GuestContextTransfer& transfer) {
        if (transfer.state != &c.s || c.s.pc != 0x10760f90 || c.sp() != frame.base()) throw;
        c.put32(error_out, c.r(3));
        r31 = error_out;
    }
    c.set(3, c.u32(t + ob::mem));
    hold();
    c.link(0x1076104c);
    free_blocks(c, c.u32(t + ob::mem));
}
}
