// The font engine, block `autogrid_a` (AUTOGRID.C's first half, 0x10751350..0x10753560): the autogridder's height and stem analysis. Dormant for the
// game (FONT.CPP passes hint = 0 and cmd = 2, so the T2K scaler never makes a hinter): the routines are checked by cases only.
//
// The autogridder works on a glyph's points (halfword x and y, an on-curve byte, the first and last point of each contour, the next and the previous
// point of each, all behind the object `autogrid::` of font_engine_detail.hpp). 0x10751350 scales the font's height records and stem widths to the
// pixel size (once per size), 0x10751b10 maps each y to its band, 0x10751bd0 finds the height record nearest to a point, 0x10751eb0 normalises a
// vector to 2.14, 0x10751fd0 computes each point's direction to the next point and from the previous one, 0x107522b0 marks the x and y extrema
// (flag bits 4 and 8), 0x107524b0 the inflections (0x80), 0x10752990 classifies every point (corners, axis-aligned edges, near-extrema, the edge
// marks 1 and 2) and 0x10753110 links each edge point to its nearest partners.
//
// Contract. The callers read r3 (0x10751bd0) and r9/r10, which the routines pass through the chain up to the hint size and glyph entries
// (font_engine_volatile_read.inc: mask 0x60); r4-r8 are leftovers no caller reads (tools/register_contract.py --after, followed through the
// pass-through chain: 0x10755930 and 0x10754ff0 read r3 and nothing else after the call, 0x10754f70's callers r3, r9 and r10). So every routine
// leaves r9 and r10 as the original does: `r9` and `r10` below mirror them, written where the original's instructions write them (every other
// register is a plain local), stored into the registers before a call out and at the end. The three that call out (0x10751350, 0x10751eb0, 0x10751fd0)
// build the original's frame; the others are leaves of the original (the red zone only) and build none.
#include "font_engine_detail.hpp"

namespace cm::aot::font_engine {
namespace {
constexpr std::int32_t s32(std::uint32_t v) { return std::int32_t(v); }
constexpr std::uint32_t u32(std::int32_t v) { return std::uint32_t(v); }
// extsh: the low halfword, sign-extended (as a register).
constexpr std::int32_t extsh(std::uint32_t v) { return std::int16_t(v); }
// srawi: the arithmetic shift.
constexpr std::int32_t sra(std::int32_t v, unsigned n) { return v >> n; }
// mulhw: the high word of the signed 64-bit product.
constexpr std::int32_t mulhw(std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * std::int64_t(b)) >> 32); }
// divw as the generated code does it: a zero divisor faults.
std::int32_t divw(std::uint32_t a, std::uint32_t b) { return s32(std::uint32_t(divide_signed<4>(a, b))); }
// The sign trick the compiler uses for |v| (xor with the sign, subtract it): the most negative value stays.
constexpr std::int32_t abs32(std::int32_t v) { const auto s = std::uint32_t(v >> 31); return std::int32_t((std::uint32_t(v) ^ s) - s); }
// srawi then addze: the quotient by 2^n truncated towards zero.
constexpr std::int32_t quotient_trunc(std::int32_t v, unsigned n) { return (v >> n) + ((v < 0 && (v & ((1 << n) - 1)) != 0) ? 1 : 0); }
// The length estimate larger + smaller/2 of a vector from its components cut to halfwords: each made positive (the most negative halfword stays),
// the sum cut to a halfword again.
constexpr std::int32_t approximate_length(std::uint32_t x, std::uint32_t y) {
    auto a = extsh(x), b = extsh(y);
    if (a < 0) a = extsh(u32(-a));
    if (b < 0) b = extsh(u32(-b));
    return extsh(u32(a <= b ? b + (a >> 1) : a + (b >> 1)));
}
// A halfword table entry (lhax) at a signed index (the byte offset is rlwinm r,i,1,0,30).
std::int32_t half_at(Cpu& c, std::uint32_t table, std::int32_t index) { return c.i16(table + (u32(index) << 1)); }
constexpr std::uint32_t twice(std::int32_t index) { return u32(index) << 1; }

// The assertion `bl 0x107755c0` of AUTOGRID.C's pool: text, file name (+0xb) and line; r9 and r10 reach the reporter.
void report(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back, std::uint32_t r9, std::uint32_t r10) {
    c.set({{3, pool + text}, {4, pool + 0xb}, {5, line}, {9, r9}, {10, r10}});
    c.call(assertion_report, back);
}
}

// 0x10751350: the font's height records and stem widths for a size (the hint handle's size entry). Clears the tables, then for the ten height records
// keeps the font-unit height and shift and, when `scale` is nonzero, scales them to 26.6 (heights to the grid, shifts to quarter pixels); records the
// four band limits and the ratios between them; and does the same for the two groups of stem widths (never under 0x21), snapping each to the first of
// its group when within 0x20. `gray_out` receives a byte from the first x stem. Returns 0.
std::uint32_t scale_hint_heights(Cpu& c, std::uint32_t g, std::uint32_t scale, std::uint32_t gray_out) {
    using namespace autogrid;
    guest::Frame frame(c, 0x10751350, 0x70, {.first_gpr = 20});
    std::uint32_t r9 = c.r(9), r10 = c.r(10);
    const auto pool = c.toc(toc::autogrid_pool);
    const std::int32_t em = c.i16(g + autogrid::units_per_em);
    const std::uint32_t x_scale = c.u32(g + size_x) << 6, y_scale = c.u32(g + size_y) << 6;
    c.put8(g + ready, 1);
    // The tables are cleared a block of eight at a time, the words and the halfwords alternately.
    for (std::uint32_t pass = 0; pass < 12; ++pass) {
        for (std::uint32_t k = 0; k < 8; ++k) c.put32(g + scaled + 0x20 * pass + 4 * k, 0);
        for (std::uint32_t k = 0; k < 8; ++k) c.put16(g + heights + 0x10 * pass + 2 * k, 0);
    }
    const std::uint32_t half_em = u32(sra(em, 1));
    const std::uint32_t kind = c.u32(g + flavor);
    // The sixth height first: its rounding (to a pixel after +0x20 and the flavor's +9) is added to the zones of the first heights.
    const std::uint32_t height_3 = u32(c.i16(g + height_records + 3 * 6));
    c.put16(g + heights + 6 * 2, height_3);
    const std::uint32_t raw_6 = u32(divw(height_3 * y_scale + half_em, u32(em)));
    const std::uint32_t grid_6 = (raw_6 + (kind == 1 ? 9 : 0) + 0x20) & 0xffffffc0u;
    c.put32(g + scaled + 6 * 4, grid_6);
    const std::uint32_t grid_shift = grid_6 - raw_6;
    r10 = grid_shift;
    // The ten height records from the last: the height (kept in font units, scaled to the grid) and the shift (rounded to a quarter pixel and signed,
    // added to the grid height). The heights 0, 1, 2 and 8 move with the sixth height's rounding, the third gets +9 in the flavor 1; in that flavor
    // the second height joins the first when they are within 0x20 of a pixel.
    for (std::int32_t i = 9; i >= 0; --i) {
        const std::uint32_t record = g + height_records + 6 * u32(i);
        r9 = g + 6 * u32(i);
        std::int32_t height = c.i16(record);
        if (i == 2 && kind == 1) {
            const std::int32_t first = c.i16(g + height_records + 6), second = c.i16(g + height_records + 12);
            if (!(s32(u32(divw(u32(abs32(first - second)) * y_scale + half_em, u32(em)))) > 0x20)) height = first;
        }
        c.put16(g + heights + 4 * u32(i), u32(height));
        if (scale != 0) {
            std::uint32_t value = u32(divw(u32(height) * y_scale + half_em, u32(em)));
            if (i == 3 && kind == 1) value += 9;
            else if (i < 3 || i == 8) value += grid_shift;
            c.put32(g + scaled + 8 * u32(i), (value + 0x20) & 0xffffffc0u);
        }
        std::int32_t shift = c.i16(record + 4);
        c.put16(g + heights + 4 * u32(i) + 2, u32(shift));
        if (scale != 0) {
            std::int32_t sign = 1;
            r9 = 1;
            if (shift < 0) { shift = -shift; sign = -1; r9 = u32(-1); }
            const std::uint32_t magnitude = u32(divw(u32(shift) * y_scale + half_em, u32(em)));
            const std::uint32_t grid = c.u32(g + scaled + 8 * u32(i));
            c.put32(g + scaled + 8 * u32(i) + 4, ((magnitude + 0x10) & 0xffffffc0u) * u32(sign) + grid);
        }
    }
    // The four band limits (the heights 2, 6, 10 and 14): the unrounded scaled value, the grid value and their difference, which must be within a
    // pixel and a half (an assertion otherwise; the reporter returns and the routine goes on).
    struct Limit { std::uint32_t entry, at, record, high_back, low_back; };
    for (const auto& limit : {Limit{2, 0, 0, 0x10751590, 0x107515a8}, Limit{6, 2, 12, 0x107515e4, 0x107515fc}, Limit{10, 4, 24, 0x10751638, 0x10751650},
                              Limit{14, 6, 36, 0x1075168c, 0x107516a4}}) {
        const std::uint32_t height = u32(c.i16(g + heights + 2 * limit.entry));
        c.put16(g + band_limits + limit.at, height);
        const std::uint32_t unrounded = u32(divw(height * y_scale, u32(em)));
        c.put32(g + band_records + limit.record, unrounded);
        const std::uint32_t grid = c.u32(g + scaled + 4 * limit.entry);
        const std::uint32_t difference = grid - unrounded;
        c.put32(g + band_records + limit.record + 4, grid);
        if (s32(difference) >= 0x60) { report(c, pool, 0, 0x43, limit.high_back, r9, r10); r9 = c.r(9); r10 = c.r(10); }
        if (s32(difference) <= -0x60) { report(c, pool, 0x16, 0x44, limit.low_back, r9, r10); r9 = c.r(9); r10 = c.r(10); }
        c.put32(g + band_records + limit.record + 8, difference);
    }
    // The five bands: numerator (the size scale, or the distance between two grid limits), denominator (the em, or the distance between two limits in
    // font units) and offset; the ratios are their 16.16 quotients (a zero denominator faults).
    const auto limit_half = [&](std::uint32_t at) { return c.i16(g + band_limits + at); };
    c.put32(g + band_numerators, y_scale);
    c.put32(g + band_denominators, u32(em));
    c.put32(g + band_offsets, c.u32(g + band_records + 8));
    c.put32(g + band_numerators + 16, y_scale);
    c.put32(g + band_denominators + 16, u32(em));
    c.put32(g + band_offsets + 16, c.u32(g + band_records + 36 + 8));
    c.put32(g + band_numerators + 4, c.u32(g + band_records + 4) - c.u32(g + band_records + 12 + 4));
    c.put32(g + band_denominators + 4, u32(limit_half(0) - limit_half(2)));
    c.put32(g + band_numerators + 8, c.u32(g + band_records + 12 + 4) - c.u32(g + band_records + 24 + 4));
    c.put32(g + band_denominators + 8, u32(limit_half(2) - limit_half(4)));
    c.put32(g + band_numerators + 12, c.u32(g + band_records + 24 + 4) - c.u32(g + band_records + 36 + 4));
    c.put32(g + band_denominators + 12, u32(limit_half(4) - limit_half(6)));
    for (std::uint32_t k = 0; k < 5; ++k) {
        Leftovers left;
        c.put32(g + band_ratios + 4 * k, u32(fix_divide(c.i32(g + band_numerators + 4 * k), c.i32(g + band_denominators + 4 * k), left)));
    }
    // The offsets of the three middle bands: half of what the grid limits have over the scaled middle of two limits.
    for (std::uint32_t band = 1; band <= 3; ++band) {
        Leftovers left;
        const auto middle = fix_multiply(c.i32(g + band_ratios + 4 * band), limit_half(2 * (band - 1)) + limit_half(2 * band), left);
        const auto grid_sum = c.u32(g + band_records + 12 * (band - 1) + 4) + c.u32(g + band_records + 12 * band + 4);
        c.put32(g + band_offsets + 4 * band, u32(sra(s32(grid_sum - u32(middle) + 1), 1)));
    }
    for (std::uint32_t k = 0; k < 4; ++k) c.put32(g + limit_words + 4 * k, u32(limit_half(2 * k)));
    c.put32(g + limits_unordered, (limit_half(6) > limit_half(4) || limit_half(4) > limit_half(2) || limit_half(2) > limit_half(0)) ? 1 : 0);
    // The stem widths: three to a group, scaled with the x size (the first four groups) or the y size, never under 0x21, then each snapped to the group's
    // first entry (the reference, when within 0x20) through 0x10753d20.
    for (std::uint32_t first : {x_stems, y_stems}) {
        const std::uint32_t source = first == x_stems ? x_stem_widths : y_stem_widths;
        const std::uint32_t stem_scale = first == x_stems ? x_scale : y_scale;
        for (std::uint32_t group = 0; group < 4; ++group) {
            for (std::uint32_t k = 0; k < 3; ++k) {
                const std::uint32_t entry = first + 3 * group + k;
                const std::uint32_t width = u32(c.i16(g + source + 6 * group + 2 * k));
                c.put16(g + heights + 2 * entry, width);
                if (scale != 0 && width != 0) {
                    auto value = divw(width * stem_scale + half_em, u32(em));
                    if (value < 0x21) value = 0x21;
                    c.put32(g + scaled + 4 * entry, u32(value));
                }
            }
        }
        const std::uint32_t reference = c.u32(g + scaled + 4 * first);
        for (std::uint32_t k = 1; k < 12; ++k) {
            const std::uint32_t entry = first + k;
            const std::uint32_t value = c.u32(g + scaled + 4 * entry);
            if (value != 0) {
                c.set({{3, reference}, {4, value}, {9, r9}, {10, r10}});
                c.call(0x10753d20, first == x_stems ? 0x10751960 : 0x10751a98);
                c.put32(g + scaled + 4 * entry, c.r(3));
            }
        }
    }
    // The byte: the sign of ((v ^ 0x60) >> 1) - ((v ^ 0x60) & 0x60) for the first x stem v, 0 when the keep-flat word says so. (The assertion
    // after it, 0x60 < the last entry index 0x5f, cannot fire.)
    const std::uint32_t mixed = c.u32(g + scaled + 4 * x_stems) ^ 0x60;
    c.put8(gray_out, (u32(sra(s32(mixed), 1)) - (mixed & 0x60)) >> 31);
    if (c.u32(g + keep_flat) != 0) c.put8(gray_out, 0);
    c.set({{4, 0x60}, {9, r9}, {10, r10}});
    return 0;
}

// 0x10751b10: for each of the glyph's points and its four phantom points (the hint input's count, +2) the value in 26.6 from the band the y falls in:
// the band's offset plus the 16.16 product of its ratio and y (written to the input's word array, +0x1c).
void band_coordinates(Cpu& c, std::uint32_t g, std::uint32_t input) {
    using namespace autogrid;
    const std::uint32_t output = c.u32(input + 0x1c);
    const std::int32_t count = c.i16(input + 2);
    const std::uint32_t ys_table = c.u32(g + ys);
    const std::int32_t top = c.i16(g + band_limits), upper = c.i16(g + band_limits + 2), lower = c.i16(g + band_limits + 4), bottom = c.i16(g + band_limits + 6);
    for (std::int32_t n = 0; n < count + 4; ++n) {
        const std::int32_t y = c.i16(ys_table + 2 * u32(n));
        std::uint32_t band;
        if (y < upper) band = y < lower ? (y > bottom ? 3 : 4) : 2;
        else band = y < top ? 1 : 0;
        Leftovers left;
        const auto product = fix_multiply(c.i32(g + band_ratios + 4 * band), y, left);
        c.put32(output + 4 * u32(n), c.u32(g + band_offsets + 4 * band) + u32(product));
    }
}

// 0x10751bd0: the height record nearest to the point's y, for the zone it can sit in, or -1: `best` runs over the ten records' two values each (the
// height as code 2i, the overshoot as code 2i+1). Only the records of the mode (2 and 6 in the mode that reads them, the others otherwise) count;
// a point that cannot be a top edge ignores the records above the middle... (see the flags below). Returns the code; r3 is read by the callers.
std::int32_t nearest_height(Cpu& c, std::uint32_t g, std::uint32_t point) {
    using namespace autogrid;
    std::uint32_t r9 = c.r(9), r10 = c.r(10);
    const std::uint32_t word = point << 2, half = point << 1;
    const std::uint32_t forward_x = c.u32(c.u32(g + in_dx) + word);
    // Not a top edge: both directions' x components under 0x3f08 (the edge does not run along +x), and not a local maximum; not a bottom edge likewise.
    r10 = 0;
    if (!(s32(forward_x) > 0x3f07)) { if (!(s32(c.u32(c.u32(g + out_dx) + word)) > 0x3f07)) r10 = 1; }
    bool not_top = r10 != 0;
    r9 = 0;
    if (!(s32(forward_x) < -0x3f07)) { if (!(s32(c.u32(c.u32(g + out_dx) + word)) < -0x3f07)) r9 = 1; }
    bool not_bottom = r9 != 0;
    const std::int32_t previous = c.i16(c.u32(g + previous_point) + half), next = c.i16(c.u32(g + next_point) + half);
    r10 = c.u32(g + ys);
    const std::int32_t y = c.i16(r10 + half);
    if (not_top && (c.u16(c.u32(g + point_flags) + half) & y_extremum) != 0 && c.i16(r10 + twice(previous)) < y && c.i16(r10 + twice(next)) < y) not_top = false;
    if (not_bottom && (c.u16(c.u32(g + point_flags) + half) & y_extremum) != 0 && y < c.i16(r10 + twice(previous)) && y < c.i16(r10 + twice(next))) not_bottom = false;
    if (not_top && not_bottom) { c.set({{9, r9}, {10, r10}}); return -1; }
    const std::int32_t em = c.i16(g + autogrid::units_per_em);
    const std::uint32_t on_curve_table = c.u32(g + on_curve);
    const std::uint32_t high = u32(mulhw(0x3e0f83e1, em));
    r9 = u32(sra(em, 2));
    const std::uint32_t on_curve_here = c.u8(on_curve_table + point);
    const std::int32_t middle = extsh(u32(quotient_trunc(em, 2)));
    r9 = u32(c.i16(g + mode));
    std::int32_t best_distance = sra(s32(high), 4);
    best_distance += s32(u32(best_distance) >> 31);
    r10 = u32(-1);
    bool second_value = true;
    if (on_curve_here != 0) {
        // On-curve: an edge along an axis keeps the second value out when it runs more than a sixteenth of the em from its on-curve neighbour.
        const std::int32_t sixteenth = extsh(u32(quotient_trunc(em, 4)));
        const std::uint32_t xs_table = c.u32(g + xs);
        bool settled = false;
        if (c.u8(on_curve_table + next) != 0) {
            std::int32_t distance = extsh(u32(c.i16(xs_table + half) - c.i16(xs_table + twice(next))));
            if (distance < 0) distance = extsh(u32(-distance));
            if (distance > sixteenth) {
                const std::int32_t forward = s32(forward_x);
                if (forward > 0x3fc2 || forward < -0x3fc2) { second_value = false; settled = true; }
            }
        }
        if (!settled && c.u8(on_curve_table + previous) != 0) {
            std::int32_t distance = extsh(u32(c.i16(xs_table + half) - c.i16(xs_table + twice(previous))));
            if (distance < 0) distance = extsh(u32(-distance));
            if (distance > sixteenth) {
                const std::int32_t backward = c.i32(c.u32(g + out_dx) + word);
                if (backward > 0x3fc2 || backward < -0x3fc2) second_value = false;
            }
        }
    }
    const std::uint32_t mode_value = r9;
    for (std::uint32_t i = 0; i < 10; ++i) {
        const std::uint32_t record = g + height_records + 6 * i;
        const std::int32_t height = c.i16(record);
        if (height > middle ? not_top : not_bottom) continue;
        const bool uses_mode = i == 2 || i == 6;
        if (uses_mode ? mode_value == 0 : mode_value != 0) continue;
        std::int32_t distance = abs32(height - y);
        if (distance < best_distance) {
            best_distance = distance;
            r10 = u32(extsh(2 * i));
            if (distance == 0) { c.set({{9, r9}, {10, r10}}); return s32(r10); }
        }
        if (second_value) {
            distance = abs32(y - c.i16(record + 2));
            if (distance < best_distance) {
                best_distance = distance;
                r10 = u32(extsh(2 * i + 1));
                if (distance == 0) { c.set({{9, r9}, {10, r10}}); return s32(r10); }
            }
        }
    }
    c.set({{9, r9}, {10, r10}});
    return s32(r10);
}

// 0x10751eb0: the vector (*a, *b) scaled to length 0x4000 (2.14): a vector along one axis gets +-0x4000 on it (the other written only when zero: the first
// component zero sets *b, the second zero sets *a), otherwise both are divided by their length found by three Newton-like rounds starting at 0x4000
// (the assertion fires when the rounds reach zero, only through the 32-bit wrap of the squares; the divisions then fault).
void normalize_vector(Cpu& c, std::uint32_t first_pointer, std::uint32_t second_pointer) {
    guest::Frame frame(c, 0x10751eb0, 0x50, {.first_gpr = 27});
    const auto pool = c.toc(toc::autogrid_pool);
    const std::uint32_t a = c.u32(first_pointer);
    if (a == 0) {
        c.put32(second_pointer, s32(c.u32(second_pointer)) < 0 ? u32(-0x4000) : 0x4000);
        return;
    }
    const std::uint32_t b = c.u32(second_pointer);
    if (b == 0) {
        c.put32(first_pointer, s32(a) < 0 ? u32(-0x4000) : 0x4000);
        return;
    }
    const std::int32_t abs_a = abs32(s32(a)), abs_b = abs32(s32(b));
    const std::int32_t reach = s32(abs_a > abs_b ? u32(abs_a) + u32(abs_b >> 1) : u32(abs_b) + u32(abs_a >> 1));
    const std::uint32_t x = u32(divw(a << 14, u32(reach)));
    const std::uint32_t y = u32(divw(b << 14, u32(reach)));
    std::uint32_t length = 0x4000;
    for (;;) {
        const std::uint32_t quotient = u32(divw(x * x + y * y, length));
        const std::uint32_t before = length;
        length = u32(sra(s32(length + (quotient + 1)), 1));
        if (before == length) break;
    }
    if (length == 0) report(c, pool, 0x37, 0x236, 0x10751f9c, c.r(9), c.r(10));
    const std::uint32_t first = u32(divw(x << 14, length)), second = u32(divw(y << 14, length));
    c.put32(first_pointer, first);
    c.put32(second_pointer, second);
}

// 0x10751fd0: for every contour of two or more points, each point's direction to the next point (in_dx, in_dy: the next point less this one, or for an
// off-curve point the line between the midpoints of its two edges) and from the previous point (out_dx, out_dy; a copy of the first when the point is
// off-curve or one of the two vectors has no length), both normalised to 2.14 through 0x10751eb0 on locals of this frame (sp+0x44/0x40 the first,
// sp+0x3c/0x38 the second), and the curve point (curve_xs, curve_ys: the point, or for an off-curve one the middle of its edges' midpoints).
void point_directions(Cpu& c, std::uint32_t g) {
    using namespace autogrid;
    guest::Frame frame(c, 0x10751fd0, 0x90, {.first_gpr = 15});
    const auto pool = c.toc(toc::autogrid_pool);
    const std::uint32_t on_curve_table = c.u32(g + on_curve), xs_table = c.u32(g + xs), ys_table = c.u32(g + ys);
    if (g == 0) report(c, pool, 0x41, 0x24e, 0x10752008, c.r(9), c.r(10));
    const std::uint32_t forward_x = frame.at(0x44), forward_y = frame.at(0x40), backward_x = frame.at(0x3c), backward_y = frame.at(0x38);
    for (std::int32_t k = 0; k < c.i16(g + contour_count); ++k) {
        const std::uint32_t contour_offset = 2 * u32(k);
        const std::int32_t end = c.i16(c.u32(g + contour_ends) + contour_offset), start = c.i16(c.u32(g + contour_starts) + contour_offset);
        if (end > start) {
            std::int32_t previous = end;
            std::int32_t px = half_at(c, xs_table, end), py = half_at(c, ys_table, end);
            for (std::int32_t i = start; i <= end; ++i) {
                const std::uint32_t at = twice(i);
                const std::int32_t next = c.i16(c.u32(g + next_point) + at);
                const std::uint32_t on = c.u8(on_curve_table + u32(i));
                const std::int32_t xi = c.i16(xs_table + at), yi = c.i16(ys_table + at);
                std::int32_t nx = half_at(c, xs_table, next), ny = half_at(c, ys_table, next);
                std::int32_t curve_x, curve_y;
                if (on != 0) {
                    c.put32(forward_x, u32(nx - xi));
                    c.put32(forward_y, u32(ny - yi));
                    c.put32(backward_x, u32(xi - px));
                    c.put32(backward_y, u32(yi - py));
                    curve_x = xi; curve_y = yi;
                } else {
                    if (c.u8(on_curve_table + u32(next)) == 0) {
                        nx = extsh(u32(sra(extsh(u32(xi + nx + 1)), 1)));
                        ny = extsh(u32(sra(extsh(u32(yi + ny + 1)), 1)));
                    }
                    if (c.u8(on_curve_table + u32(previous)) == 0) {
                        px = extsh(u32(sra(extsh(u32(xi + px + 1)), 1)));
                        py = extsh(u32(sra(extsh(u32(yi + py + 1)), 1)));
                    }
                    const std::int32_t before_x = extsh(u32(sra(px + xi + 1, 1))), before_y = extsh(u32(sra(py + yi + 1, 1)));
                    const std::int32_t after_x = extsh(u32(sra(xi + nx + 1, 1))), after_y = extsh(u32(sra(yi + ny + 1, 1)));
                    const std::int32_t dx = after_x - before_x, dy = after_y - before_y;
                    c.put32(forward_x, u32(dx));
                    c.put32(backward_x, u32(dx));
                    c.put32(forward_y, u32(dy));
                    c.put32(backward_y, u32(dy));
                    curve_x = extsh(u32(sra(after_x + before_x + 1, 1)));
                    curve_y = extsh(u32(sra(after_y + before_y + 1, 1)));
                }
                c.link(0x107521b0);
                normalize_vector(c, forward_x, forward_y);
                c.put32(c.u32(g + in_dx) + 4 * u32(i), c.u32(forward_x));
                c.put32(c.u32(g + in_dy) + 4 * u32(i), c.u32(forward_y));
                const bool copy = c.u8(on_curve_table + u32(i)) == 0 || (c.u32(backward_x) == 0 && c.u32(forward_x) == 0) ||
                                  (c.u32(backward_y) == 0 && c.u32(forward_y) == 0);
                if (copy) {
                    c.put32(c.u32(g + out_dx) + 4 * u32(i), c.u32(c.u32(g + in_dx) + 4 * u32(i)));
                    c.put32(c.u32(g + out_dy) + 4 * u32(i), c.u32(c.u32(g + in_dy) + 4 * u32(i)));
                } else {
                    c.link(0x10752234);
                    normalize_vector(c, backward_x, backward_y);
                    c.put32(c.u32(g + out_dx) + 4 * u32(i), c.u32(backward_x));
                    c.put32(c.u32(g + out_dy) + 4 * u32(i), c.u32(backward_y));
                }
                c.put16(c.u32(g + curve_xs) + at, u32(curve_x));
                c.put16(c.u32(g + curve_ys) + at, u32(curve_y));
                previous = i;
                px = xi;
                py = yi;
            }
        }
    }
}

// 0x107522b0: marks the extrema of every contour of two or more points: the points where x (flag 4) or y (flag 8) turns, over runs of equal
// coordinates (a run is marked at its first point). The previous run's coordinate is found by walking back at most 0x81 points from the contour's
// first point; the walk forward visits each run until the next run starts at or before the previous one's first point.
void mark_extrema(Cpu& c, std::uint32_t g) {
    using namespace autogrid;
    std::uint32_t r9 = c.r(9), r10 = c.r(10);
    const std::uint32_t flags = c.u32(g + point_flags), next_table = c.u32(g + next_point);
    for (std::int32_t k = 0; k < c.i16(g + contour_count); ++k) {
        const std::uint32_t at = 2 * u32(k);
        r9 = c.u32(g + contour_starts);
        const std::int32_t start = c.i16(r9 + at), end = c.i16(c.u32(g + contour_ends) + at);
        if (!(end > start)) continue;
        // The x pass.
        {
            const std::uint32_t coordinates = c.u32(g + xs);
            r10 = u32(start);
            r9 = 0x80;
            const std::int32_t first = half_at(c, coordinates, start);
            std::int32_t walker = start, value;
            for (;;) {
                walker = half_at(c, c.u32(g + previous_point), walker);
                value = half_at(c, coordinates, walker);
                r10 = u32(walker);
                if (first != value || walker == start) break;
                const bool again = s32(r9) > 0;
                r9 -= 1;
                if (!again) break;
            }
            std::int32_t r11 = value, r30 = half_at(c, coordinates, start), r29 = start;
            for (;;) {
                r10 = u32(r11);
                r11 = r30;
                r9 = u32(r29);
                const std::int32_t run_value = extsh(u32(r30));
                for (;;) {
                    r29 = half_at(c, next_table, r29);
                    r30 = half_at(c, coordinates, r29);
                    if (r30 != run_value || r29 == s32(r9)) break;
                }
                const std::int32_t here = extsh(u32(r11)), before = extsh(u32(r10));
                if ((here > r30 && here > before) || (here < r30 && here < before))
                    c.put16(flags + twice(s32(r9)), c.u16(flags + twice(s32(r9))) | x_extremum);
                if (!(r29 > s32(r9))) break;
            }
        }
        // The y pass (the first point is read again; the registers play other parts).
        {
            const std::uint32_t coordinates = c.u32(g + ys);
            r9 = 0x80;
            std::int32_t r30 = c.i16(c.u32(g + contour_starts) + at);
            r10 = u32(r30);
            const std::int32_t first = half_at(c, coordinates, r30);
            std::int32_t walker = r30, r29;
            for (;;) {
                walker = half_at(c, c.u32(g + previous_point), walker);
                r29 = half_at(c, coordinates, walker);
                r10 = u32(walker);
                if (first != r29 || walker == r30) break;
                const bool again = s32(r9) > 0;
                r9 -= 1;
                if (!again) break;
            }
            r9 = u32(half_at(c, coordinates, r30));
            std::int32_t r11;
            for (;;) {
                r10 = u32(r29);
                r29 = s32(r9);
                r11 = r30;
                const std::int32_t run_value = extsh(r9);
                for (;;) {
                    r30 = half_at(c, next_table, r30);
                    r9 = u32(half_at(c, coordinates, r30));
                    if (s32(r9) != run_value || r30 == r11) break;
                }
                const std::int32_t here = extsh(u32(r29)), next = s32(r9), before = extsh(r10);
                if ((here > next && here > before) || (here < next && here < before))
                    c.put16(flags + twice(r11), c.u16(flags + twice(r11)) | y_extremum);
                if (!(r30 > r11)) break;
            }
        }
    }
    c.set({{9, r9}, {10, r10}});
}

// 0x107524b0: marks the inflections (flag 0x80) of every contour of three or more points. Each point's turn is the cross product of its two edges over
// their approximate length (the sum vector's); a point counts as turning when that is above max(em / 256, 1) in magnitude (corners, flag 0x100, are
// skipped by the first scan). The scan starts at the first turning point (the first with a nonzero turn when there is none) and goes round the contour;
// when the sign of a turn differs from the last turning point's, the point half way between the two (counted along the contour) is marked unless a point
// of either triple is a corner.
void mark_inflections(Cpu& c, std::uint32_t g) {
    using namespace autogrid;
    std::uint32_t r9 = c.r(9), r10 = c.r(10);
    const std::uint32_t flags = c.u32(g + point_flags), next_table = c.u32(g + next_point);
    const std::int32_t em = c.i16(g + autogrid::units_per_em);
    std::int32_t tolerance = quotient_trunc(em, 8);
    if (tolerance == 0) tolerance = 1;
    // The turn at `cur` between the edge from `prev` and the one to `next` (the callers' arithmetic: wrapping products, halfword lengths).
    const auto turn = [&](std::int32_t prev, std::int32_t cur, std::int32_t next) {
        const std::uint32_t xs_table = c.u32(g + xs), ys_table = c.u32(g + ys);
        const std::int32_t in_x = half_at(c, xs_table, cur) - half_at(c, xs_table, prev), out_x = half_at(c, xs_table, next) - half_at(c, xs_table, cur);
        const std::int32_t in_y = half_at(c, ys_table, cur) - half_at(c, ys_table, prev), out_y = half_at(c, ys_table, next) - half_at(c, ys_table, cur);
        const std::uint32_t cross = u32(in_x) * u32(out_y) - u32(out_x) * u32(in_y);
        return divw(cross, u32(approximate_length(u32(in_x + out_x), u32(in_y + out_y)) + 1));
    };
    std::int32_t r29 = 0;
    for (std::int32_t k = 0; k < c.i16(g + contour_count); ++k) {
        const std::uint32_t at = 2 * u32(k);
        const std::uint32_t starts = c.u32(g + contour_starts);
        r10 = c.u32(g + contour_ends);
        std::int32_t r30 = c.i16(starts + at);
        r9 = u32(r30);
        const std::int32_t start = r30, end = c.i16(r10 + at);
        if (!(end > start + 1)) continue;
        const std::uint32_t previous_table = c.u32(g + previous_point);
        r10 = previous_table;
        std::int32_t r12 = half_at(c, previous_table, start);
        std::int32_t r27 = 0;
        // The first scan: the first point that is not a corner whose turn is beyond the tolerance.
        for (std::int32_t n = end + 1 - start; n > 0; --n) {
            r29 = half_at(c, next_table, r30);
            if ((c.u16(flags + twice(r30)) & corner) == 0) {
                r27 = turn(r12, r30, r29);
                if (r27 < -tolerance || r27 > tolerance) break;
            }
            r12 = r30;
            r30 += 1;
        }
        if (r27 == 0) {
            // The second scan: the first point with any turn at all (corners included); r9 and r10 end as the last test left them.
            r30 = start;
            r12 = half_at(c, previous_table, start);
            for (std::int32_t n = end + 1 - start; n > 0; --n) {
                r29 = half_at(c, next_table, r30);
                const std::uint32_t xs_table = c.u32(g + xs), ys_table = c.u32(g + ys);
                const std::int32_t in_x = half_at(c, xs_table, r30) - half_at(c, xs_table, r12), out_x = half_at(c, xs_table, r29) - half_at(c, xs_table, r30);
                const std::int32_t in_y = half_at(c, ys_table, r30) - half_at(c, ys_table, r12), out_y = half_at(c, ys_table, r29) - half_at(c, ys_table, r30);
                const std::uint32_t cross = u32(in_x) * u32(out_y) - u32(out_x) * u32(in_y);
                std::int32_t a = extsh(u32(in_x + out_x)), b = extsh(u32(in_y + out_y));
                if (a < 0) a = extsh(u32(-a));
                if (b < 0) b = extsh(u32(-b));
                r10 = u32(a);
                r9 = u32(b);
                r27 = divw(cross, u32(approximate_length(u32(in_x + out_x), u32(in_y + out_y)) + 1));
                if (r27 != 0) break;
                r12 = r30;
                r30 += 1;
            }
        }
        if (r30 > end) continue;
        // The main scan round the contour from r30 until it comes back to it.
        std::int32_t last_prev = r12, last_cur = r30, last_next = r29, last_turn = r27;
        std::int32_t last_sign = r27 < 0 ? -1 : 1;
        const std::int32_t stop = r30;
        for (;;) {
            const std::int32_t cur = r30;
            const std::uint32_t cur_at = twice(cur);
            r9 = cur_at;
            r29 = half_at(c, next_table, cur);
            r12 = half_at(c, previous_table, cur);
            r10 = twice(r12);
            const std::int32_t value = turn(r12, cur, r29);
            const std::int32_t sign = value < 0 ? -1 : 1;
            bool update = false;
            if (value != 0 && (value < -tolerance || value > tolerance)) {
                update = true;
                if (sign != last_sign && !(last_turn >= -tolerance && last_turn <= tolerance)) {
                    const std::uint32_t f_prev = c.u16(flags + twice(last_prev)), f_cur = c.u16(flags + twice(last_cur)), f_next = c.u16(flags + twice(last_next));
                    if (((f_prev | f_cur | f_next) & corner) == 0) {
                        const std::uint32_t g_prev = c.u16(flags + twice(r12)), g_cur = c.u16(flags + cur_at), g_next = c.u16(flags + twice(r29));
                        r10 = (g_prev | g_cur | g_next) & corner;
                        if (r10 == 0) {
                            // The point half way from the previous turn's previous point to this point's next point.
                            std::int32_t steps = 0, walker = last_prev;
                            while (walker != r29) { walker = half_at(c, next_table, walker); ++steps; }
                            const std::int32_t half_steps = (steps + 1 + s32(u32(steps + 1) >> 31)) >> 1;
                            std::int32_t middle = last_prev;
                            for (std::int32_t n = 0; n < half_steps; ++n) middle = half_at(c, next_table, middle);
                            const std::uint32_t before = c.u16(flags + twice(middle));
                            r10 = before & corner;
                            if (r10 == 0) { r10 = before | inflection; c.put16(flags + twice(middle), r10); }
                        }
                    }
                }
            }
            if (update) { last_prev = r12; last_cur = cur; last_next = r29; last_turn = value; last_sign = sign; }
            if ((c.u16(flags + cur_at) & corner) != 0) last_turn = 0;
            r30 = half_at(c, next_table, cur);
            if (r30 == stop) break;
        }
    }
    c.set({{9, r9}, {10, r10}});
}

// 0x10752990: classifies the points. The flags are cleared (every point and the four phantom points); the extrema are marked (0x107522b0); an extremum
// flag on an off-curve point moves to an on-curve neighbour within em/1000-ish pixels (the threshold (em / 100 / 10) + 1, in font units); each point gets
// its corner flag (the two directions' dot product under 0x3fc3 in 2.14) and the flags for a direction within 0x3fc2 of an axis; off-curve points and
// on-curve points between off-curve neighbours within (em >> 7) + 1 of their extremum get the near-extremum flags 0x200 and 0x400; the inflections are marked
// (0x107524b0); every point with a 4 or 0x100 flag gets x_edge, with 8 or 0x100 y_edge; the phantom points 1 and 2 get x_edge; and a long edge (at least em/10
// between two on-curve points) gets the edge flags of its direction on both ends.
void classify_points(Cpu& c, std::uint32_t g) {
    using namespace autogrid;
    std::uint32_t r9 = c.r(9), r10 = c.r(10);
    const std::uint32_t flags = c.u32(g + point_flags), on_curve_table = c.u32(g + on_curve);
    const std::int32_t em = c.i16(g + autogrid::units_per_em);
    const std::int32_t contours = c.i16(g + contour_count);
    const std::int32_t last = c.i16(c.u32(g + contour_ends) + (u32(contours) << 1) - 2);
    std::int32_t hundredth = sra(mulhw(0x51eb851f, em), 5);
    hundredth += s32(u32(hundredth) >> 31);
    std::int32_t tenth = sra(mulhw(0x66666667, hundredth), 2);
    tenth += s32(u32(tenth) >> 31);
    const std::int32_t near_threshold = tenth + 1;
    // Clear the flags of every point and the phantom points, the last first.
    if (last + 4 >= 0)
        for (std::int32_t n = last + 4; n >= 0; --n) c.put16(flags + twice(n), 0);
    c.set({{9, r9}, {10, r10}});
    mark_extrema(c, g);
    r9 = c.r(9); r10 = c.r(10);
    const std::uint32_t last2 = twice(last);
    // An x or y extremum flagged on an off-curve point moves to a neighbouring on-curve point close enough along that axis.
    const auto move_extremum = [&](std::uint32_t table, std::uint16_t bit, std::int32_t point, std::int32_t neighbour) {
        const std::uint32_t coordinates = c.u32(g + table);
        const std::int32_t distance = abs32(half_at(c, coordinates, point) - half_at(c, coordinates, neighbour));
        if (c.u8(on_curve_table + u32(neighbour)) != 0 && distance <= near_threshold) {
            c.put16(flags + twice(point), c.u16(flags + twice(point)) & ~bit);
            c.put16(flags + twice(neighbour), c.u16(flags + twice(neighbour)) | bit);
            return true;
        }
        return false;
    };
    for (std::int32_t i = last; i >= 0; --i) {
        const std::uint32_t at = twice(i);
        if (c.u8(on_curve_table + u32(i)) != 0) continue;
        r9 = c.u16(flags + at);
        const std::int32_t previous = half_at(c, c.u32(g + previous_point), i), next = half_at(c, c.u32(g + next_point), i);
        if ((r9 & x_extremum) != 0) {
            r10 = twice(next);
            move_extremum(xs, x_extremum, i, next);
            r9 = twice(previous);
            move_extremum(xs, x_extremum, i, previous);
        }
        const std::uint32_t after = c.u16(flags + at);
        if ((after & y_extremum) != 0) {
            r9 = twice(next);
            move_extremum(ys, y_extremum, i, next);
            move_extremum(ys, y_extremum, i, previous);
        }
    }
    // Corners, and which axes each direction lies along.
    for (std::int32_t i = last; i >= 0; --i) {
        const std::uint32_t at = twice(i), word = 4 * u32(i);
        const std::int32_t fx = c.i32(c.u32(g + in_dx) + word), bx = c.i32(c.u32(g + out_dx) + word), fy = c.i32(c.u32(g + in_dy) + word), by = c.i32(c.u32(g + out_dy) + word);
        const auto set = [&](std::uint16_t bit) { c.put16(flags + at, c.u16(flags + at) | bit); };
        if (sra(s32(u32(fx) * u32(bx) + u32(fy) * u32(by)), 14) <= 0x3fc2) set(corner);
        if (fx > 0x3fc2 || fx < -0x3fc2) set(in_horizontal);
        else if (fy > 0x3fc2 || fy < -0x3fc2) set(in_vertical);
        if (bx > 0x3fc2 || bx < -0x3fc2) set(out_horizontal);
        else if (by > 0x3fc2 || by < -0x3fc2) set(out_vertical);
    }
    // The near-extremum flags.
    const std::int32_t reach = sra(em, 7) + 1;
    for (std::int32_t i = last; i >= 0; --i) {
        const std::uint32_t at = twice(i);
        r9 = c.u16(flags + at);
        if ((r9 & corner) != 0) continue;
        const bool on = c.u8(on_curve_table + u32(i)) != 0;
        const std::int32_t next = half_at(c, c.u32(g + next_point), i), previous = half_at(c, c.u32(g + previous_point), i);
        const auto near = [&](std::uint32_t table, std::int32_t neighbour) {
            return abs32(half_at(c, c.u32(g + table), neighbour) - half_at(c, c.u32(g + table), i)) < reach;
        };
        const auto off = [&](std::int32_t neighbour) { return c.u8(on_curve_table + u32(neighbour)) == 0; };
        if (on) {
            if ((r9 & x_extremum) != 0 && off(next)) {
                r10 = c.u32(g + xs);
                if (near(xs, next) && off(previous) && near(xs, previous)) c.put16(flags + at, r9 | near_x_extremum);
            }
            const std::uint32_t current = c.u16(flags + at);
            if ((current & y_extremum) != 0 && off(next)) {
                r9 = c.u32(g + ys);
                r10 = u32(half_at(c, c.u32(g + ys), i));
                if (near(ys, next) && off(previous) && near(ys, previous)) c.put16(flags + at, current | near_y_extremum);
            }
        } else {
            if ((r9 & x_extremum) != 0) {
                bool mark = false;
                if (off(next)) { r9 = c.u32(g + xs); mark = near(xs, next); }
                if (!mark && off(previous)) { r9 = c.u32(g + xs); mark = near(xs, previous); }
                if (mark) c.put16(flags + at, c.u16(flags + at) | near_x_extremum);
            }
            if ((c.u16(flags + at) & y_extremum) != 0) {
                bool mark = false;
                if (off(next)) mark = near(ys, next);
                if (!mark && off(previous)) mark = near(ys, previous);
                if (mark) c.put16(flags + at, c.u16(flags + at) | near_y_extremum);
            }
        }
    }
    c.set({{9, r9}, {10, r10}});
    mark_inflections(c, g);
    // Edge marks from the extremum and corner flags (the phantom points included), the phantom points 1 and 2 as x edges.
    for (std::int32_t n = last + 4; n >= 0; --n) {
        auto f = c.u16(flags + twice(n));
        if ((f & (x_extremum | corner)) != 0) c.put16(flags + twice(n), f | x_edge);
        f = c.u16(flags + twice(n));
        if ((f & (y_extremum | corner)) != 0) c.put16(flags + twice(n), f | y_edge);
    }
    c.put16(flags + last2 + 2, c.u16(flags + last2 + 2) | x_edge);
    c.put16(flags + last2 + 4, c.u16(flags + last2 + 4) | x_edge);
    // A long edge between two on-curve points (at least em/10) marks both ends: x_edge for a direction near the y axis (|in_dx| <= 0x2cb), y_edge near
    // the x axis, both otherwise.
    std::int32_t tenth_em = sra(mulhw(0x66666667, em), 2);
    tenth_em += s32(u32(tenth_em) >> 31);
    for (std::int32_t i = last; i >= 0; --i) {
        const std::uint32_t at = twice(i);
        const std::int32_t next = half_at(c, c.u32(g + next_point), i);
        if (c.u8(on_curve_table + u32(i)) == 0 || c.u8(on_curve_table + u32(next)) == 0) continue;
        const std::int32_t length = approximate_length(u32(half_at(c, c.u32(g + xs), i) - half_at(c, c.u32(g + xs), next)),
                                                       u32(half_at(c, c.u32(g + ys), i) - half_at(c, c.u32(g + ys), next)));
        if (length < tenth_em) continue;
        const std::int32_t dx = c.i32(c.u32(g + in_dx) + 4 * u32(i)), dy = c.i32(c.u32(g + in_dy) + 4 * u32(i));
        std::uint16_t bits = 3;
        if (dx >= -0x2cb && dx <= 0x2cb) bits = x_edge;
        else if (dy >= -0x2cb && dy <= 0x2cb) bits = y_edge;
        c.put16(flags + at, c.u16(flags + at) | bits);
        c.put16(flags + twice(next), c.u16(flags + twice(next)) | bits);
    }
}

// 0x10753110: for every edge point (flags 0x83 and a direction flag 0x7800, whose next point is another) the nearest edge point on its other side:
// partner_before (the nearest by a distance estimate, that is also in a roughly opposite direction by the in-directions' dot product and on the left
// of the edge), partner_after (the same with the out direction; none when in and out directions are equal: it then copies partner_before). Candidates
// must share a horizontal or vertical kind with the point; the score is the distance estimate plus the offset across the edge.
void link_edge_points(Cpu& c, std::uint32_t g) {
    using namespace autogrid;
    std::uint32_t r9 = c.r(9), r10 = c.r(10);
    const std::uint32_t flags = c.u32(g + point_flags);
    std::int32_t edges = 0;
    std::uint32_t edge_offset = 0;
    r9 = 0;
    {
        const std::int32_t count = c.i16(g + contour_count);
        const std::int32_t last = c.i16(c.u32(g + contour_ends) + (u32(count) << 1) - 2);
        if (last >= 0) {
            for (std::int32_t n = 0; n <= last; ++n) {
                const std::uint32_t at = twice(n);
                c.put16(c.u32(g + partner_before) + at, 0xffff);
                c.put16(c.u32(g + partner_after) + at, 0xffff);
                const std::uint32_t f = c.u16(flags + at);
                if ((f & 0x83) != 0 && (f & 0x7800) != 0 && s32(r9) != half_at(c, c.u32(g + next_point), n)) {
                    ++edges;
                    c.put16(c.u32(g + extrema) + edge_offset, r9);
                    edge_offset += 2;
                }
                r9 += 1;
            }
        }
    }
    std::int16_t j = 0;
    while (j < edges) {
        const std::int32_t p = half_at(c, c.u32(g + extrema), j);
        const std::uint32_t word = 4 * u32(p), at = twice(p);
        r10 = 0;
        const std::int32_t in_x_p = c.i32(c.u32(g + in_dx) + word), in_y_p = c.i32(c.u32(g + in_dy) + word);
        const std::int32_t out_x_p = c.i32(c.u32(g + out_dx) + word), out_y_p = c.i32(c.u32(g + out_dy) + word);
        const std::uint32_t flags_p = c.u16(flags + at);
        if (out_x_p == in_x_p && out_y_p == in_y_p) r10 = 1;
        std::int32_t best_in = 0x7fff, best_out = 0x7fff;
        for (std::int32_t m = 0; m < edges; ++m) {
            const std::int32_t q = half_at(c, c.u32(g + extrema), m);
            if (q == p) continue;
            const std::uint32_t q_at = twice(q), qw = 4 * u32(q);
            const std::uint32_t flags_q = c.u16(flags + q_at);
            const bool horizontal_kind = (flags_q & 0x2800) != 0, vertical_kind = (flags_q & 0x5000) != 0;
            if (!((horizontal_kind && (flags_p & 0x2800) != 0) || (vertical_kind && (flags_p & 0x5000) != 0))) continue;
            // The distance estimate between the curve points.
            const std::uint32_t curve_x_table = c.u32(g + curve_xs), curve_y_table = c.u32(g + curve_ys);
            const std::int32_t dx = c.i16(curve_x_table + q_at) - c.i16(curve_x_table + at), dy = c.i16(curve_y_table + q_at) - c.i16(curve_y_table + at);
            const std::int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
            const std::int32_t distance = ax > ay ? (ax >> 1) + (ay >> 2) : (ax >> 2) + (ay >> 1);
            // The point q qualifies for the direction (dir) when the original coordinates put it on the positive side of dir's cross product and one of
            // its own two directions is nearly opposite: its in-direction when that points across, else its out-direction.
            const auto partner = [&](std::int32_t dir_x, std::int32_t dir_y, std::int32_t cross_x, std::int32_t cross_y, bool first) {
                const std::int32_t x_gap = cross_x, y_gap = cross_y;
                if (!(s32(u32(dir_y) * u32(x_gap) - u32(dir_x) * u32(y_gap)) > 0)) return false;
                const std::int32_t qin_x = c.i32(c.u32(g + in_dx) + qw), qin_y = c.i32(c.u32(g + in_dy) + qw);
                if (!first) r9 = u32(qin_y);
                if (s32(u32(qin_y) * u32(x_gap) - u32(qin_x) * u32(y_gap)) < 0 && sra(s32(u32(dir_x) * u32(qin_x) + u32(dir_y) * u32(qin_y)), 14) <= -0x3e14) return true;
                const std::int32_t qout_x = c.i32(c.u32(g + out_dx) + qw), qout_y = c.i32(c.u32(g + out_dy) + qw);
                return s32(u32(qout_y) * u32(x_gap) - u32(qout_x) * u32(y_gap)) < 0 && sra(s32(u32(dir_x) * u32(qout_x) + u32(dir_y) * u32(qout_y)), 14) <= -0x3e14;
            };
            const std::uint32_t xs_table = c.u32(g + xs), ys_table = c.u32(g + ys);
            if (distance < best_in && ((horizontal_kind && (flags_p & 0x800) != 0) || (vertical_kind && (flags_p & 0x1000) != 0))) {
                const std::int32_t score = abs32(sra(s32(u32(-in_y_p) * u32(dx) + u32(in_x_p) * u32(dy)), 14)) + distance;
                if (score < best_in) {
                    const std::int32_t x_gap = c.i16(xs_table + q_at) - c.i16(xs_table + at), y_gap = c.i16(ys_table + q_at) - c.i16(ys_table + at);
                    r9 = u32(x_gap);
                    if (partner(in_x_p, in_y_p, x_gap, y_gap, true)) { c.put16(c.u32(g + partner_before) + at, u32(q)); best_in = score; }
                }
            }
            if (r10 == 0 && distance < best_out && ((horizontal_kind && (flags_p & 0x2000) != 0) || (vertical_kind && (flags_p & 0x4000) != 0))) {
                const std::int32_t score = abs32(sra(s32(u32(-out_y_p) * u32(dx) + u32(out_x_p) * u32(dy)), 14)) + distance;
                if (score < best_out) {
                    const std::int32_t x_gap = c.i16(xs_table + q_at) - c.i16(xs_table + at), y_gap = c.i16(ys_table + q_at) - c.i16(ys_table + at);
                    if (partner(out_x_p, out_y_p, x_gap, y_gap, false)) { c.put16(c.u32(g + partner_after) + at, u32(q)); best_out = score; }
                }
            }
        }
        if (r10 != 0) c.put16(c.u32(g + partner_after) + at, c.u16(c.u32(g + partner_before) + at));
        j = std::int16_t(j + 1);
    }
    c.set({{9, r9}, {10, r10}});
}
}
