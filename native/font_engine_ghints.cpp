// The font engine, block `ghints` (GHINTS.C, 0x1075ac40..0x1075b970): the global hint data the autogridder starts from (0x6c bytes at the
// font's sfnt + 0x38): per font, ten triples (the lower value, the upper value and their difference, one halfword each) and two arrays of
// twelve halfwords, which prepare_hinting (block t2k_core) either reads from the font's T2KG table (0x1075ad60) or measures from the font's own
// glyphs (0x1075af60: the heights of the letters the pool names, medians of the tops or bottoms of their outlines, tidied until the values that
// are nearly equal are made equal, then the first 'o' (or the glyphs of a block of the font) run through the autogridder's analysis 0x10754f70).
//
// Contract. Every routine that has a frame builds it (guest::Frame of the original's size and saved registers) and holds the original's
// non-volatile registers at each call (the callees save them: the stack leaver 0x10767bb0 is called from here at the original's stack pointer,
// so its prologue words are the original's). The volatile registers r3-r10 are left as the original leaves them on every path (callers read r9
// and r10 through the pass-throughs): the leaves and the straight-line code of 0x1075af60 write them where the listing's instructions do; the
// callees' own leftovers land in them through the shadow `v` where a native callee is called with Leftovers. The arrays the original keeps on its
// frame (the sample arrays of 0x1075ae30 and 0x1075af60) stay in the guest frame: where the original reads an entry it never wrote (no sample
// at all), it reads the stack's old bytes, and so does this code.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
constexpr std::int32_t extsh(std::uint32_t v) { return std::int16_t(v); }
constexpr std::uint32_t reg(std::int32_t v) { return std::uint32_t(v); }
// The pool of GHINTS.C (TOC -0x3c48): the characters of the height lists, the assertion texts (+0, +0x63, +0x6c, +0x8b) and the file name at +0xd.
constexpr std::uint32_t text_no_glyph = 0x63, text_missing_arrays = 0x6c, text_too_many = 0x8b, file_name = 0xd;

// What the generated stream readers store in their caller's frame before they run (see font_engine_glyph.cpp).
void reader_prologue(Cpu& c) {
    const auto sp = c.sp();
    c.put32(sp - 4, c.r(31));
    c.put32(sp - 8, c.r(30));
    c.put32(sp - 0xc, c.r(29));
    c.put32(sp + 8, c.lr());
    c.put32(sp - 0x50, sp);
}
using Volatile = std::array<std::uint32_t, 11>;
void store(Cpu& c, const Volatile& v) { for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]); }
void load(Cpu& c, Volatile& v) { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }

// The extreme of the y halfwords over the contours of more than one point: 0x1075ac40 keeps the highest (starting from -0x8000), 0x1075acd0 the
// lowest (from 0x7fff). A contour whose start is not below its end (one point) is skipped.
std::int32_t extreme_y(Cpu& c, std::uint32_t glyph, bool highest) {
    Volatile v{};
    load(c, v);
    std::uint32_t r4 = v[4], r5 = v[5], r6 = 0, r7 = v[7], r8 = 0, r9 = v[9];
    const std::uint32_t r10 = c.u32(glyph + glyph_record::ys);
    std::uint32_t r11 = highest ? 0xffff8000u : 0x7fff;
    while (std::int32_t(r8) < extsh(c.u16(glyph + glyph_record::contours))) {
        r5 = c.u32(glyph + glyph_record::contour_starts);
        r4 = c.u32(glyph + glyph_record::contour_ends);
        r7 = reg(extsh(c.u16(r5 + r6)));
        r4 = reg(extsh(c.u16(r4 + r6)));
        if (std::int32_t(r7) < std::int32_t(r4)) {
            const auto count = r4 + 1 - r7;
            r5 = (r7 << 1) & 0xfffffffeu;
            r9 = r7;
            r4 = r10 + r5;
            for (std::uint32_t n = count; n != 0; --n) {  // mtspr CTR: bdnz
                r7 = reg(extsh(c.u16(r4)));
                const auto kept = extsh(r11);
                if (highest ? std::int32_t(r7) > kept : std::int32_t(r7) < kept) r11 = r7;
                r5 += 2;
                r4 += 2;
                r9 += 1;
            }
        }
        r6 += 2;
        r8 += 1;
    }
    v[3] = reg(extsh(r11)); v[4] = r4; v[5] = r5; v[6] = r6; v[7] = r7; v[8] = r8; v[9] = r9; v[10] = r10;
    store(c, v);
    return extsh(r11);
}
}

std::int32_t glyph_highest(Cpu& c, std::uint32_t glyph) { return extreme_y(c, glyph, true); }
std::int32_t glyph_lowest(Cpu& c, std::uint32_t glyph) { return extreme_y(c, glyph, false); }

// 0x1075ad60 (data, stream): the global hint data from a T2KG table: a halfword that must be zero (the assertion at line 0x4e otherwise), ten
// pairs (lower, upper) with the sum stored between them, then twelve pairs of halfwords at +0x3c and +0x54.
void read_hint_data(Cpu& c, std::uint32_t data, std::uint32_t stream) {
    guest::Frame frame(c, 0x1075ad60, 0x50, {.first_gpr = 27});
    std::uint32_t r27 = data, r28 = stream, r29 = c.r(29), r30 = c.r(30), r31 = c.toc(toc::ghints_pool);
    const std::uint32_t pool = r31;
    c.set({{27, r27}, {28, r28}});
    const auto hold = [&] { c.set({{27, r27}, {28, r28}, {29, r29}, {30, r30}, {31, r31}}); };
    const auto read = [&](std::uint32_t back) {
        c.set(3, r28);
        hold();
        c.link(back);
        reader_prologue(c);
        const auto value = reg(read_signed_16(c, r28));
        c.set(3, value);
        return value;
    };
    c.set(3, r28);
    hold();
    c.link(0x1075ad84);
    reader_prologue(c);
    c.set(3, reg(read_signed_16(c, r28)));
    if (extsh(c.r(3)) != 0) {
        c.set({{3, pool}, {4, pool + file_name}, {5, 0x4e}});
        hold();
        c.call(assertion_report, 0x1075ad9c);
    }
    r29 = 0;
    r31 = 0;
    for (;;) {
        const auto lower = read(0x1075adb0);
        r30 = r27 + r31;
        c.put16(r30, lower);
        const auto upper = read(0x1075adc0);
        c.put16(r30 + 4, upper);
        r29 += 1;
        r31 += 6;
        c.put16(r30 + 2, c.u16(r30) + c.u16(r30 + 4));
        c.set(3, reg(extsh(c.u16(r30))));
        if (!(std::int32_t(r29) < 10)) break;
    }
    r29 = 0;
    r31 = 0;
    for (;;) {
        const auto first = read(0x1075adf8);
        r30 = r27 + r31;
        c.put16(r30 + 0x3c, first);
        const auto second = read(0x1075ae08);
        r29 += 1;
        c.put16(r30 + 0x54, second);
        r31 += 2;
        if (!(std::int32_t(r29) < 12)) break;
    }
}

// 0x1075ae30 (font, characters, top): the median of the tops (top != 0: the highest y) or the bottoms (the lowest y) of the glyphs of the
// characters in the zero-terminated list (at most 32), over the glyphs that have an outline. A composite takes the first component's glyph.
// A list with no outline gives 0.
std::int32_t median_height(Cpu& c, std::uint32_t font, std::uint32_t characters, std::uint32_t top) {
    guest::Frame frame(c, 0x1075ae30, 0xa0, {.first_gpr = 26});
    std::uint32_t r26 = c.r(26), r27 = font, r28 = characters, r29 = top, r30 = 0, r31 = 0;
    const auto hold = [&] { c.set({{26, r26}, {27, r27}, {28, r28}, {29, r29}, {30, r30}, {31, r31}}); };
    const auto samples = frame.at(0x3c);
    const auto load_one = [&](std::uint32_t index, std::uint32_t back) {
        c.set({{3, r27}, {4, index}, {5, 0}, {6, frame.at(0x3a)}, {7, frame.at(0x38)}});
        hold();
        c.link(back);
        const auto glyph = load_glyph(c, r27, index, 0, frame.at(0x3a), frame.at(0x38));
        c.set(3, glyph);
        return glyph;
    };
    const auto discard = [&](std::uint32_t glyph, std::uint32_t back) {
        c.set(3, glyph);
        hold();
        c.link(back);
        delete_glyph(c, glyph);
    };
    for (;;) {
        if (!(extsh(r31) < 0x20)) break;
        const auto code = c.u8(r28 + reg(extsh(r31)));
        c.set(4, code);
        if (code == 0) break;
        c.set(3, r27);
        hold();
        c.link(0x1075ae60);
        const auto glyph_number = character_glyph(c, r27, code);
        c.set(3, glyph_number);
        r26 = load_one(glyph_number & 0xffff, 0x1075ae78);
        if (extsh(c.u16(r26 + glyph_record::contours)) < 0) {
            const auto components = c.u32(r26 + glyph_record::components);
            const auto first = reg(extsh(c.u16(components + 2))) & 0xffff;
            const auto old = r26;
            r26 = first;
            c.set(4, components);
            discard(old, 0x1075ae98);
            r26 = load_one(first, 0x1075aeb0);
        }
        if (extsh(c.u16(r26 + glyph_record::contours)) != 0 && extsh(c.u16(r26 + glyph_record::outline_points)) > 0) {
            c.set(3, r26);
            hold();
            std::int32_t value;
            if (extsh(r29) != 0) { c.link(0x1075aedc); value = glyph_highest(c, r26); }
            else { c.link(0x1075aee8); value = glyph_lowest(c, r26); }
            c.set(4, samples);
            c.put16(samples + 2 * reg(extsh(r30)), reg(value));
            r30 += 1;
        }
        discard(r26, 0x1075af04);
        r31 += 1;
    }
    if (extsh(r30) == 0) {
        c.set(3, 0);
        return 0;
    }
    Volatile v{};
    load(c, v);
    v[3] = samples; v[4] = reg(extsh(r30));
    store(c, v);
    hold();
    c.link(0x1075af3c);
    Leftovers left{v[4], v[5], v[6], v[7], v[8]};
    sort_shorts(c, samples, extsh(r30), left);
    c.set({{4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}});
    const auto result = extsh(c.u16(samples + (reg(extsh(r30)) & ~1u)));
    c.set(3, reg(result));
    return result;
}

// 0x1075af60 (font, hinter, data, mode): the global hint data measured from the font. Twenty medians of the tops and bottoms of the letters the
// pool names fill ten triples (lower, upper, difference); differences that should not be negative (or positive) are clamped and values of nearly
// equal levels made equal around the median of seven of them; twelve pairs of halfwords at +0x3c and +0x54 are zero except the first pair, the
// median extent of the autogridder's analysis (0x10754f70, called through the dispatcher) of the glyph 'o', or, when the mode is not zero or that
// glyph has no outline, of the glyphs 0..(limit / 64 stepping by limit / 32, five at a time) of the font (at most 32 samples; the median of the
// sorted values, both made the lower one when they are one apart). Ends with the cmap released.
void analyse_hint_data(Cpu& c, std::uint32_t font, std::uint32_t hinter, std::uint32_t data, std::uint32_t mode) {
    guest::Frame frame(c, 0x1075af60, 0x140, {.first_gpr = 21});
    std::array<std::uint32_t, 32> g{};
    for (unsigned n = 21; n <= 31; ++n) g[n] = c.r(n);
    const std::uint32_t pool = c.toc(toc::ghints_pool);
    g[22] = font; g[23] = hinter; g[24] = data; g[26] = mode; g[30] = pool;
    const auto hold = [&] { for (unsigned n = 21; n <= 31; ++n) c.set(n, g[n]); };
    const auto halfword = [&](std::uint32_t at) { return extsh(c.u16(data + at)); };
    const auto sp = [&](std::uint32_t offset) { return frame.at(std::int32_t(offset)); };
    const auto report = [&](std::uint32_t text, std::uint32_t line, std::uint32_t back) {
        c.set({{3, pool + text}, {4, pool + file_name}, {5, line}});
        hold();
        c.call(assertion_report, back);
    };
    // The twenty medians: (characters at pool + offset, top, destination, address after the bl).
    struct Median { std::uint32_t characters, top, destination, back; };
    static constexpr Median medians[] = {
        {0x16, 1, 0x0, 0x1075af90}, {0x1c, 1, 0x2, 0x1075afa4}, {0x1e, 1, 0x6, 0x1075afb8}, {0x24, 1, 0x8, 0x1075afcc}, {0x29, 1, 0xc, 0x1075afe0},
        {0x2b, 1, 0xe, 0x1075aff4}, {0x2f, 1, 0x12, 0x1075b008}, {0x31, 1, 0x14, 0x1075b01c}, {0x35, 0, 0x18, 0x1075b030}, {0x3d, 0, 0x1a, 0x1075b044},
        {0x43, 0, 0x1e, 0x1075b058}, {0x45, 0, 0x20, 0x1075b06c}, {0x4a, 0, 0x24, 0x1075b080}, {0x4f, 0, 0x26, 0x1075b094}, {0x56, 0, 0x2a, 0x1075b0a8},
        {0x59, 0, 0x2c, 0x1075b0bc}, {0x5b, 1, 0x30, 0x1075b0d0}, {0x5e, 1, 0x32, 0x1075b0e4}, {0x5b, 0, 0x36, 0x1075b0f8}, {0x5e, 0, 0x38, 0x1075b10c}};
    for (const auto& m : medians) {
        c.set({{3, font}, {4, pool + m.characters}, {5, m.top}});
        hold();
        c.link(m.back);
        const auto value = median_height(c, font, pool + m.characters, m.top);
        c.put16(data + m.destination, reg(value));
    }
    // The differences: upper less lower, a negative one (of the first five) or a positive one (of the rest) made zero.
    for (const std::uint32_t at : {0u, 6u, 0xcu, 0x12u, 0x18u, 0x1eu, 0x24u, 0x2au, 0x30u, 0x36u})
        c.put16(data + at + 4, reg(halfword(at + 2) - halfword(at)));
    for (const std::uint32_t at : {4u, 0xau, 0x10u, 0x16u, 0x34u}) if (halfword(at) < 0) c.put16(data + at, 0);
    for (const std::uint32_t at : {0x1cu, 0x22u, 0x28u, 0x2eu, 0x3au}) if (halfword(at) > 0) c.put16(data + at, 0);
    // The seven values whose median is the common level.
    const auto samples = sp(0x4c);
    c.put16(samples, reg(halfword(0xa)));
    c.put16(samples + 2, reg(halfword(0x16)));
    c.put16(samples + 4, reg(halfword(0x10)));
    c.put16(samples + 6, reg(-halfword(0x1c)));
    c.put16(samples + 8, reg(-halfword(0x22)));
    c.put16(samples + 10, reg(-halfword(0x28)));
    c.put16(samples + 12, reg(-halfword(0x2e)));
    Volatile v{};
    c.set({{3, samples}, {4, 7}});
    hold();
    c.link(0x1075b2d8);
    Leftovers left{7, c.r(5), c.r(6), c.r(7), c.r(8)};
    sort_shorts(c, samples, 7, left);
    load(c, v);
    v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
    const auto median = reg(extsh(c.u16(sp(0x52))));  // r0
    v[3] = 4;
    v[9] = 0;
    // Four pairs (the upper level's index, the lower's): when the median is within two of both the level and its mirror the pair is moved to it,
    // else the same is tried with half the sum of the pair.
    static constexpr unsigned upper[] = {1, 3, 2, 8}, lower[] = {4, 5, 6, 9};
    for (unsigned i = 0; i < 4; ++i) {
        v[3] = upper[i]; v[4] = lower[i];
        v[4] = reg(extsh(v[4])); v[3] = reg(extsh(v[3]));
        v[4] = v[4] * 6;
        v[8] = 0;
        v[3] = v[3] * 6;
        v[7] = v[4] + 4;
        v[5] = reg(extsh(c.u16(data + v[7])));
        v[6] = v[3] + 4;
        v[4] = reg(extsh(c.u16(data + v[6])));
        v[3] = v[4] - v[5];
        v[3] += 1;
        v[3] = reg(std::int32_t(v[3]) >> 1);
        v[10] = reg(extsh(v[3]));
        v[3] = v[10];
        v[3] = median;  // cr0 is the equal of `cmpwi r8,0` just made: the first try is always with the median
        v[8] = reg(extsh(v[3]));
        std::uint32_t r11 = 0, r12 = 0;
        const auto near = [&] {  // from `subf r3,r4,r8`
            v[3] = v[8] - v[4];
            r11 = reg(extsh(v[3]));
            if (std::int32_t(r11) < 0) { v[3] = reg(-std::int32_t(r11)); r11 = reg(extsh(v[3])); }
            v[3] = v[8] + v[5];
            r12 = reg(extsh(v[3]));
            if (std::int32_t(r12) < 0) { v[3] = reg(-std::int32_t(r12)); r12 = reg(extsh(v[3])); }
            v[3] = reg(extsh(r11));
            if (std::int32_t(v[3]) >= 3) return false;
            v[3] = reg(extsh(r12));
            return !(std::int32_t(v[3]) >= 3);
        };
        const auto move = [&] {
            c.put16(data + v[6], v[8]);
            v[3] = reg(-std::int32_t(v[8]));
            c.put16(data + v[7], v[3]);
        };
        if (near()) move();
        else {
            v[3] = v[10];
            v[8] = reg(extsh(v[3]));
            if (near()) move();
        }
        v[9] += 1;
    }
    store(c, v);
    // The median against the first width and the last difference.
    {
        std::uint32_t r3 = reg(extsh(c.u16(data + 4)));
        r3 = median - r3;
        r3 = reg(extsh(r3));
        if (std::int32_t(r3) < 0) { r3 = reg(-std::int32_t(r3)); r3 = reg(extsh(r3)); }
        r3 = reg(extsh(r3));
        if (std::int32_t(r3) < 3) c.put16(data + 4, median);
        r3 = reg(extsh(c.u16(data + 0x2e)));
        r3 = median + r3;
        r3 = reg(extsh(r3));
        if (std::int32_t(r3) < 0) { r3 = reg(-std::int32_t(r3)); r3 = reg(extsh(r3)); }
        r3 = reg(extsh(r3));
        if (std::int32_t(r3) < 3) c.put16(data + 0x2e, reg(-std::int32_t(median)));
    }
    // Each triple's upper value is the lower plus the (adjusted) difference; the last read, the lower of the last triple, stays in r3.
    for (const std::uint32_t at : {0u, 6u, 0xcu, 0x12u, 0x18u, 0x1eu, 0x24u, 0x2au, 0x30u, 0x36u}) c.put16(data + at + 2, reg(halfword(at) + halfword(at + 4)));
    c.set(3, reg(halfword(0x36)));
    for (std::uint32_t at = 0x3c; at < 0x6c; at += 2) c.put16(data + at, 0);
    g[27] = 1;
    // One glyph through the autogridder's analysis (0x10754f70: the hinter, the glyph description at sp + 0xdc, 0, 2 and four result pointers at sp + 0x48,
    // 0x40, 0x44, 0x3c: two arrays of values and their counts), both arrays sorted. The callers differ in the assertion lines and the return addresses.
    struct Backs { std::uint32_t analysis, arrays, first_sort, second_sort; std::uint32_t line_analysis, line_arrays; };
    const auto analyse = [&](std::uint32_t glyph, const Backs& b) {
        c.put16(sp(0xdc), c.r(3));  // r3: the contour count (lha 0x36)
        const auto points = reg(extsh(c.u16(glyph + glyph_record::outline_points)));
        c.put16(sp(0xde), points);
        c.set({{3, g[23]}, {4, sp(0xdc)}, {5, 0}, {6, 2}, {7, sp(0x48)}, {8, sp(0x40)}, {9, sp(0x44)}, {10, sp(0x3c)}});
        for (const auto [from, to] : {std::pair{0x3cu, 0xe0u}, {0x40u, 0xe4u}, {0x44u, 0xe8u}, {0x48u, 0xecu}, {0x4cu, 0xf0u}}) c.put32(sp(to), c.u32(glyph + from));
        c.put32(sp(0xf4), 0);
        c.put32(sp(0xf8), 0);
        hold();
        c.call(0x10754f70, b.analysis);
        if (c.r(3) != 0) report(text_no_glyph, b.line_analysis, b.analysis + 0x18);
        c.set(3, 0);
        if (c.u32(sp(0x48)) != 0 && c.u32(sp(0x44)) != 0) c.set(3, 1);
        if (c.r(3) == 0) report(text_missing_arrays, b.line_arrays, b.arrays);
        for (const auto [array, count, back] : {std::tuple{0x48u, 0x40u, b.first_sort}, {0x44u, 0x3cu, b.second_sort}}) {
            c.set({{3, c.u32(sp(array))}, {4, c.u32(sp(count))}});
            hold();
            c.link(back);
            Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)};
            sort_shorts(c, c.u32(sp(array)), std::int32_t(c.u32(sp(count))), left);
            c.set({{4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}});
        }
    };
    // The median element (index count / 2, a halfword at byte offset count & ~1) of a sorted array, read by sra, shift and mask as the original does.
    const auto middle = [&](std::uint32_t array, std::uint32_t count) {
        const auto offset = (reg(std::int32_t(count) >> 1) << 1) & 0xfffffffeu;
        return reg(extsh(c.u16(array + offset)));
    };
    const auto release_both = [&](std::uint32_t back_first, std::uint32_t back_second) {
        const auto mem = [&] { return c.u32(font + sfnt::mem); };
        c.set({{3, mem()}, {4, c.u32(sp(0x48))}});
        hold();
        c.link(back_first);
        c.call(dispatched::release_entry, back_first);
        c.set({{4, c.u32(sp(0x44))}});
        c.put32(sp(0x48), 0);
        c.set(3, mem());
        hold();
        c.link(back_second);
        c.call(dispatched::release_entry, back_second);
        c.put32(sp(0x44), 0);
    };
    if (g[26] == 0) {
        c.set({{3, font}, {4, 0x6f}});
        hold();
        c.link(0x1075b5a8);
        c.set(3, character_glyph(c, font, 0x6f));
        c.set({{4, c.r(3) & 0xffff}, {3, font}, {5, 0}, {6, sp(0x3a)}, {7, sp(0x38)}});
        hold();
        c.link(0x1075b5c0);
        g[25] = load_glyph(c, font, c.r(4), 0, sp(0x3a), sp(0x38));
        c.set(3, reg(extsh(c.u16(g[25] + glyph_record::contours))));
        if (extsh(c.r(3)) > 0) {
            analyse(g[25], {0x1075b634, 0x1075b684, 0x1075b690, 0x1075b69c, 0x143, 0x144});
            c.put16(data + 0x3c, middle(c.u32(sp(0x48)), c.u32(sp(0x40))));
            c.put16(data + 0x54, middle(c.u32(sp(0x44)), c.u32(sp(0x3c))));
            c.set(3, c.u32(sp(0x44)));
            release_both(0x1075b6d8, 0x1075b6ec);
            g[27] = 0;
        }
        c.set(3, g[25]);
        hold();
        c.link(0x1075b700);
        delete_glyph(c, g[25]);
    }
    if (g[26] != 0 || g[27] != 0) {
        c.set(3, font);
        hold();
        c.link(0x1075b718);
        g[31] = glyph_limit(c, font);
        c.set(3, g[31]);
        g[26] = 0;
        g[28] = 0;
        g[27] = reg(std::int32_t(g[31]) >> 6) + ((std::int32_t(g[31]) < 0 && (g[31] & 0x3f) != 0) ? 1u : 0u);
        while (std::int32_t(g[27]) < std::int32_t(g[31]) && std::int32_t(g[26]) < 0x20) {
            g[21] = 0;
            g[29] = g[28];
            for (;;) {
                c.set({{3, font}, {4, g[27] + g[21]}, {5, 0}, {6, sp(0x3a)}, {7, sp(0x38)}});
                hold();
                c.link(0x1075b750);
                g[25] = load_glyph(c, font, g[27] + g[21], 0, sp(0x3a), sp(0x38));
                c.set(3, reg(extsh(c.u16(g[25] + glyph_record::contours))));
                if (extsh(c.r(3)) > 0) {
                    analyse(g[25], {0x1075b7c4, 0x1075b814, 0x1075b820, 0x1075b82c, 0x164, 0x165});
                    const auto first = middle(c.u32(sp(0x48)), c.u32(sp(0x40)));
                    const auto second = middle(c.u32(sp(0x44)), c.u32(sp(0x3c)));
                    c.set({{3, sp(0x5c)}, {4, c.u32(sp(0x44))}, {5, sp(0x9c)}, {6, first}});
                    c.put16(sp(0x9c) + g[29], first);
                    c.put16(sp(0x5c) + g[29], second);
                    if (!(std::int32_t(g[26]) < 0x20)) report(text_too_many, 0x16a, 0x1075b87c);
                    g[28] += 2;
                    g[26] += 1;
                    g[29] += 2;
                    release_both(0x1075b894, 0x1075b8a8);
                    g[21] = 0x3e7;
                }
                c.set(3, g[25]);
                hold();
                c.link(0x1075b8bc);
                delete_glyph(c, g[25]);
                const bool again = std::int32_t(g[21]) < 4;
                g[21] += 1;
                if (!again) break;
            }
            g[27] += reg(std::int32_t(g[31]) >> 5) + ((std::int32_t(g[31]) < 0 && (g[31] & 0x1f) != 0) ? 1u : 0u);
        }
        for (const auto array : {sp(0x9c), sp(0x5c)}) {
            c.set({{4, g[26]}, {3, array}});
            hold();
            c.link(array == sp(0x9c) ? 0x1075b8f0 : 0x1075b8fc);
            Leftovers left{c.r(4), c.r(5), c.r(6), c.r(7), c.r(8)};
            sort_shorts(c, array, std::int32_t(g[26]), left);
            c.set({{4, left.r4}, {5, left.r5}, {6, left.r6}, {7, left.r7}, {8, left.r8}});
        }
        const auto offset = g[26] & 0xfffffffeu;
        const auto high = reg(extsh(c.u16(sp(0x9c) + offset))), low = reg(extsh(c.u16(sp(0x5c) + offset)));
        c.set({{3, offset}, {5, sp(0x9c)}, {4, sp(0x5c)}});
        c.put16(data + 0x3c, high);
        c.put16(data + 0x54, low);
        // The larger of the two (r0) and the smaller (r4): within one of each other, both are the smaller.
        std::uint32_t big = high, small = low;
        if (std::int32_t(high) < std::int32_t(low)) { small = high; big = low; }
        c.set({{4, small}, {3, reg(extsh(small))}});
        if (std::int32_t(extsh(big) - extsh(small)) <= 1) {
            c.put16(data + 0x3c, small);
            c.put16(data + 0x54, small);
        }
    }
    c.set(3, font);
    hold();
    c.link(0x1075b95c);
    delete_cmap(c, font);
}
}
