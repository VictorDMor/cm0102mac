// The font engine, block `autogrid_b` (AUTOGRID.C's second half, 0x10753560..0x107559a0): what the hint strategy (HSTRAT97.C) calls to place the
// edges of a glyph's stems, the hint execution that drives the passes (0x10754ff0) and the hint handle's creation, per-glyph run and deletion.
// The game never runs any of it (FONT.CPP passes hint = 0), so the cases and the hinted end-to-end mode are the only evidence.
//
// Conventions. `h` is the hint input record (hint_input:: in the detail header: the glyph's halfword arrays and the 26.6 word arrays the passes move),
// `ag` the autogrid object (autogrid::). Most routines here are called by pointer (descriptors the strategy loads from the TOC) or by direct `bl` from
// the strategy; the arguments are in registers and the strategy's stack words at the caller's sp + 0x38.. . Every routine leaves r3-r10 exactly as the
// original does (they are compared in full): `Vol` is the shadow of those registers, updated where the original's instructions write them.
#include "font_engine_detail.hpp"
#include <array>
#include <utility>

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
namespace ag = autogrid;
constexpr std::uint32_t indirect_glue = 0x10781320;
constexpr std::uint32_t pool_file = 0xb;  // the unit's file name in its pool

// A register as lha/extsh leave it, and the shifts and the multiply-high of the original's instructions.
constexpr std::uint32_t extsh(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
std::uint32_t lha(const Cpu& c, std::uint32_t at) { return std::uint32_t(std::int32_t(c.i16(at))); }
constexpr std::uint32_t extsb(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int8_t(v))); }
constexpr std::int32_t s32(std::uint32_t v) { return std::int32_t(v); }
constexpr std::uint32_t sra(std::uint32_t v, unsigned n) { return std::uint32_t(std::int32_t(v) >> n); }
// srawi followed by addze: an arithmetic shift rounded towards zero.
constexpr std::uint32_t sra_toward_zero(std::uint32_t v, unsigned n) {
    const bool carry = s32(v) < 0 && (v & ((1u << n) - 1)) != 0;
    return sra(v, n) + (carry ? 1u : 0u);
}
constexpr std::uint32_t mulhw(std::uint32_t a, std::uint32_t b) { return std::uint32_t((std::int64_t(s32(a)) * s32(b)) >> 32); }
// divw: a zero divisor faults as the generated code does.
std::uint32_t divw(std::uint32_t a, std::uint32_t b) { return std::uint32_t(divide_signed<4>(a, b)); }
// The grid: round to a whole pixel (26.6), the way the strategy does it: (v + 0x20) & ~0x3f.
constexpr std::uint32_t grid(std::uint32_t v) { return (v + 0x20) & 0xffffffc0u; }

// r3-r10 as the original's instructions hold them (index = register number).
struct Vol {
    std::array<std::uint32_t, 11> r{};
    explicit Vol(const Cpu& c) { load(c); }
    void load(const Cpu& c) { for (unsigned n = 3; n <= 10; ++n) r[n] = c.r(n); }
    void store(Cpu& c) const { for (unsigned n = 3; n <= 10; ++n) c.set(n, r[n]); }
    std::uint32_t& operator[](unsigned n) { return r[n]; }
};

// `bl 0x10781320` with r12 = the descriptor in a TOC slot; the callee returns through the glue and the TOC is reloaded from sp + 0x14.
void call_glue(Cpu& c, Vol& v, std::int32_t slot, std::uint32_t back) {
    v.store(c);
    c.set(12, c.toc(slot));
    c.call(indirect_glue, back);
    c.set(2, c.u32(c.sp() + 0x14));
    v.load(c);
}
// The hint handle's two stamps; the loaded words are what the original leaves in registers.
struct Stamps { bool valid; std::uint32_t first, second; };
Stamps check_stamps(const Cpu& c, std::uint32_t handle) {
    Stamps s{false, 0, 0};
    if (handle == 0) return s;
    s.first = c.u32(handle + ag::stamp);
    if (s.first != ag::stamp_value) return s;
    s.second = c.u32(handle + ag::end_stamp);
    s.valid = s.second == ag::end_stamp_value;
    return s;
}
// The strategy's words: x when `x` else y.
constexpr std::uint32_t words_of(bool x) { return x ? hint_input::x_words : hint_input::y_words; }
}

// ---- the callbacks the strategy reaches by pointer or direct call ----------------------------------------------------------------------------

// 0x10753a80: the bisection of the stem width starts over: bounds 0x800..0x2000, the middle 0x1000, no step taken (r4 and r5 keep the constants).
void reset_bisection(Cpu& c, std::uint32_t handle) {
    c.put32(handle + ag::search_mid, 0x1000);
    c.put32(handle + ag::search_low, 0x800);
    c.put32(handle + ag::search_high, 0x2000);
    c.put32(handle + ag::search_steps, 0);
    c.set({{4, 0x2000}, {5, 0x800}});
}

// 0x10753cd0: the scaled value of table entry `index` (0x270f: none, the value itself) limited to within 0x60 of `value`; r3 is the only register it writes.
std::uint32_t limit_to_band(Cpu& c, std::uint32_t handle, std::uint32_t index, std::uint32_t value) {
    const auto entry = extsh(index);
    std::uint32_t result;
    if (entry == 0x270f) {
        result = value;
    } else {
        const auto reference = c.u32(handle + ag::scaled + (entry << 2));
        if (s32(value) > s32(reference)) {
            const auto low = value - 0x60;
            result = s32(reference) <= s32(low) ? low : reference;
        } else {
            const auto high = value + 0x60;
            result = s32(reference) >= s32(high) ? high : reference;
        }
    }
    c.set(3, result);
    return result;
}

// 0x10753d20: the old value when the new one is within half a pixel (0x20) of it, else the new one. r4 is the result as well, r5 the sign of the difference.
std::uint32_t hold_if_close(Cpu& c, std::uint32_t old_value, std::uint32_t new_value) {
    const auto difference = old_value - new_value;
    const auto sign = sra(difference, 31);
    const auto magnitude = (sign ^ difference) - sign;
    const auto result = s32(magnitude) > 0x20 ? new_value : old_value;
    c.set({{3, result}, {4, result}, {5, sign}});
    return result;
}

// 0x10753bf0 and 0x10753c30: word[to] raised to word[from] (take_maximum) or lowered to it (take_minimum) in the hint input's x words (`x` nonzero) or
// y words. r3 is the old word[to], r4 the byte offset of `to`, r5 the array.
void take_extreme(Cpu& c, std::uint32_t h, std::uint32_t x, std::uint32_t from, std::uint32_t to, bool maximum) {
    const auto words = c.u32(h + words_of(extsh(x) != 0));
    const auto to_at = extsh(to) << 2, from_at = extsh(from) << 2;
    const auto old_to = c.u32(words + to_at), source = c.u32(words + from_at);
    c.set({{3, old_to}, {4, to_at}, {5, words}});
    if (maximum ? s32(old_to) < s32(source) : s32(old_to) > s32(source)) c.put32(words + to_at, source);
}

// 0x10753c70: where word[j] is above word[i], the excess (as a halfword) comes off word[j] and off word[k].
void remove_excess(Cpu& c, std::uint32_t h, std::uint32_t x, std::uint32_t i, std::uint32_t j, std::uint32_t k) {
    const auto words = c.u32(h + words_of(extsh(x) != 0));
    const auto j_index = extsh(j), j_at = j_index << 2, i_at = extsh(i) << 2;
    const auto at_j = c.u32(words + j_at), at_i = c.u32(words + i_at);
    c.set({{3, j_index}, {4, at_j}, {6, j_at}, {9, words}});
    if (s32(at_j) <= s32(at_i)) return;
    const auto excess = extsh(at_j - at_i);
    const auto k_at = extsh(k) << 2;
    const auto new_j = at_j - excess;
    c.put32(words + j_at, new_j);
    const auto at_k = c.u32(words + k_at);
    c.put32(words + k_at, at_k - excess);
    c.set({{3, k_at}, {4, new_j}, {5, excess}});
}

// 0x10753f70 and 0x10753fa0: word[index] of the x (y) words rounded to a whole pixel, when `enabled` (a halfword) is nonzero.
void round_word(Cpu& c, std::uint32_t h, std::uint32_t enabled, std::uint32_t index, bool x) {
    if (extsh(enabled) == 0) return;
    const auto words = c.u32(h + words_of(x));
    const auto at = index << 2;
    const auto old_value = c.u32(words + at);
    c.put32(words + at, old_value + 0x20);
    c.put32(words + at, c.u32(words + at) & 0xffffffc0u);
    c.set({{3, old_value}, {4, at}, {5, words}});
}

// 0x10753fd0 and 0x10754050: the same after word[index] is limited to within 0x60 of the scaled table entry `reference` (0x270f: none).
void round_word_in_band(Cpu& c, std::uint32_t ag_, std::uint32_t h, std::uint32_t enabled, std::uint32_t index, std::uint32_t reference, bool x) {
    if (extsh(enabled) == 0) return;
    const auto entry = extsh(reference);
    const auto words = c.u32(h + words_of(x));
    const auto at = index << 2;
    auto value = c.u32(words + at);
    if (entry != 0x270f) {
        const auto scaled = c.u32(ag_ + ag::scaled + (entry << 2));
        if (s32(value) > s32(scaled)) {
            const auto low = value - 0x60;
            value = s32(scaled) <= s32(low) ? low : scaled;
        } else {
            const auto high = value + 0x60;
            value = s32(scaled) >= s32(high) ? high : scaled;
        }
    }
    c.put32(words + at, value);
    const auto stored = c.u32(words + at);
    c.put32(words + at, stored + 0x20);
    c.put32(words + at, c.u32(words + at) & 0xffffffc0u);
    // r3 is the value limited (before the rounding), r6 the value the limit left in the register.
    c.set({{3, stored}, {4, at}, {5, words}, {6, value}});
}

// 0x107540d0 and 0x10754130: word[b] placed between word[a] and word[c] in proportion to the halfword coordinates (the original positions): the
// y version uses the ys and the y words. A zero span faults (divw).
void interpolate_word(Cpu& c, std::uint32_t h, std::uint32_t a, std::uint32_t b, std::uint32_t c_index, bool x) {
    const auto halves = c.u32(h + (x ? hint_input::xs : hint_input::ys));
    const auto words = c.u32(h + words_of(x));
    const auto a_half = lha(c, halves + (a << 1)), b_half = lha(c, halves + (b << 1)), c_half = lha(c, halves + (c_index << 1));
    const auto a_word = c.u32(words + (a << 2)), c_word = c.u32(words + (c_index << 2));
    const auto from_a_to_b = b_half - a_half, word_span = c_word - a_word, half_span = c_half - a_half;
    const auto numerator = from_a_to_b * word_span + sra(half_span, 1);
    const auto quotient = divw(numerator, half_span);
    const auto placed = a_word + quotient;
    c.put32(words + (b << 2), placed);
    c.set({{3, placed}, {4, sra(half_span, 1)}, {5, half_span}, {6, a_word}, {7, c_index << 1}, {8, a_half}, {9, halves}, {10, words}});
}

// 0x10753d70 (x) and 0x10753e70 (y): the point j of the hint input's words is placed at the point i's word plus or minus the width of the two
// points' original (halfword) distance scaled by the size, fitted: `width_index` (a halfword, negative: none) names the scaled table entry it snaps to,
// `keep_visible` makes a width that rounds to nothing one pixel. i and j are the caller's stack words (sp + 0x38 and + 0x3c).
void place_edge_x(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t width_index, std::uint32_t keep_visible) {
    const auto i = c.u32(c.sp() + 0x38), j = c.u32(c.sp() + 0x3c);
    const auto xs = c.u32(handle + ag::xs);
    const auto x_i = lha(c, xs + (i << 1)), x_j = lha(c, xs + (j << 1));
    const auto scale = c.u32(handle + ag::size_x) << 6;
    const auto resolution = lha(c, handle + ag::units_per_em);
    auto width = divw((x_j - x_i) * scale + sra(resolution, 1), resolution);
    std::uint32_t sign = 1;
    if (s32(width) < 0) { width = negate(width); sign = 0xffffffffu; }
    const auto flat = c.u32(handle + ag::keep_flat);
    std::uint32_t r8 = flat;
    if (flat != 0) {
        if (s32(width) < 0x20 && extsh(keep_visible) != 0) {
            width += width;
            if (s32(width) > 0x20) width = 0x20;
        }
    } else {
        r8 = extsh(width_index);
        if (s32(r8) >= 0) {
            const auto reference = c.u32(handle + ag::scaled + (extsh(width_index) << 2));
            if (s32(width) > s32(reference)) {
                const auto low = width - 0x2a;
                width = s32(reference) <= s32(low) ? low : reference;
            } else {
                const auto high = width + 0x2a;
                width = s32(reference) >= s32(high) ? high : reference;
            }
        }
        width = grid(width);
        if (width == 0 && extsh(keep_visible) != 0) width = 0x40;
    }
    const auto words = c.u32(h + hint_input::x_words);
    const auto base = c.u32(words + (i << 2));
    const auto placed = base + width * extsh(sign);
    c.put32(words + (j << 2), placed);
    c.set({{3, placed}, {4, base}, {5, words}, {6, i}, {8, r8}, {9, width}, {10, x_i}});
}
void place_edge_y(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t width_index, std::uint32_t keep_visible) {
    const auto i = c.u32(c.sp() + 0x38), j = c.u32(c.sp() + 0x3c);
    const auto ys = c.u32(handle + ag::ys);
    const auto y_i = lha(c, ys + (i << 1)), y_j = lha(c, ys + (j << 1));
    const auto scale = c.u32(handle + ag::size_y) << 6;
    const auto resolution = lha(c, handle + ag::units_per_em);
    auto distance = divw((y_j - y_i) * scale + sra(resolution, 1), resolution);
    std::uint32_t sign = 1;
    if (s32(distance) < 0) { distance = negate(distance); sign = 0xffffffffu; }
    const auto words = c.u32(h + hint_input::y_words);  // r10
    std::uint32_t r5 = width_index, r8 = distance, r9 = distance;
    if (s32(extsh(width_index)) >= 0) {
        r5 = c.u32(handle + ag::scaled + (extsh(width_index) << 2));
        std::uint32_t fitted;
        if (s32(distance) > s32(r5)) {
            fitted = distance - 0x2a;
            if (!(s32(r5) <= s32(fitted))) fitted = r5;
        } else {
            fitted = distance + 0x2a;
            if (!(s32(r5) >= s32(fitted))) fitted = r5;
        }
        r8 = fitted;
    }
    if (c.u32(handle + ag::keep_flat) != 0) {
        if (s32(r8) >= 0x24 && c.u32(handle + ag::flavor) != 2) r9 = grid(r8);
    } else {
        r9 = grid(r8);
        if (r9 == 0 && extsh(keep_visible) != 0) r9 = 0x40;
    }
    const auto base = c.u32(words + (i << 2));
    const auto placed = base + r9 * extsh(sign);
    c.put32(words + (j << 2), placed);
    c.set({{3, placed}, {4, base}, {5, r5}, {6, i}, {8, r8}, {9, r9}, {10, words}});
}

// 0x10754550 and 0x107545b0 (two copies): the edge placed through the glue by the x callback (`x` nonzero) or the y one, with the fixed arguments
// (r6 = 0, r7 = 1, r8 = 1, r9 = 0x47, r10 = 0x72) and the caller's r6 and r7 as the stack words the callback reads.
void forward_place_edge(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    Vol v(c);
    const auto x = v[8], first = v[6], second = v[7];
    Frame frame(c, entry, 0x40, {});
    c.put32(frame.at(0x38), first);
    c.put32(frame.at(0x3c), second);
    v[6] = 0; v[7] = 1; v[8] = 1; v[9] = 0x47; v[10] = 0x72;
    call_glue(c, v, extsh(x) != 0 ? -0x5b64 : -0x5b68, back);
}

// 0x107537c0: the point `c_index` placed between the edges `a` and `b` of the x (flag nonzero) or y words from the original coordinates and the width
// the bisection holds; the word then fitted. (A leaf: the original saves r28-r31 below its stack pointer and restores them.)
void settle_between_edges(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t flag, std::uint32_t r7, std::uint32_t r8, std::uint32_t r9) {
    const bool x = extsh(flag) != 0;
    const auto units = lha(c, handle + ag::units_per_em);
    const auto halves = c.u32(handle + (x ? ag::xs : ag::ys));
    const auto words = c.u32(h + words_of(x));
    const auto scale = c.u32(handle + (x ? ag::size_x : ag::size_y)) << 6;
    const auto a = extsh(r7), b = extsh(r8), cc = extsh(r9);
    const auto steps = c.u32(handle + ag::search_steps);
    const auto at_b = lha(c, halves + (b << 1)), at_cc = lha(c, halves + (cc << 1)), at_a = lha(c, halves + (a << 1));
    auto span_b = at_cc - at_b;
    auto span_a = at_cc - at_a;
    const auto half = sra(units, 1);
    const auto mid = c.u32(handle + ag::search_mid);
    const auto word_a = c.u32(words + (a << 2)), word_b = c.u32(words + (b << 2));
    span_a = span_a * scale + half;
    span_b = span_b * scale + half;
    span_a = divw(span_a, units);
    span_b = divw(span_b, units);
    const auto scaled_a = span_a * mid + 0x20;
    const auto scaled_b = span_b * mid + 0x20;
    const auto r10_tmp = sra(scaled_a, 6);
    const auto r9_new = sra_toward_zero(scaled_b + 0, 12);  // (span_b * mid + 0x20) >> 12, rounded towards zero
    auto place_a = sra_toward_zero(r10_tmp, 6);
    auto r10 = r9_new + word_b;
    auto r7_new = place_a + word_a;
    if (steps == 0) {
        if (s32(r7_new) < s32(r10)) {
            r7_new += 0x40;
            if (!(s32(r7_new) <= s32(r10))) r7_new = r10;
        } else {
            r7_new -= 0x40;
            if (!(s32(r7_new) >= s32(r10))) r7_new = r10;
        }
    }
    std::uint32_t r8_new, stored;
    if (c.u32(handle + ag::flavor) == 2) {
        r7_new += 1;
        r8_new = r10 + r7_new;
        stored = sra(r8_new + (r8_new >> 31), 1);
        r7_new = stored;
    } else {
        const auto sum = r10 + r7_new;
        r8_new = mulhw(0x55555556, r10 + (sum + 1));
        stored = r8_new + (r8_new >> 31);
        r7_new = stored;
    }
    c.put32(words + (cc << 2), stored);
    const bool low = c.u32(handle + ag::keep_flat) == 0 || !(s32(r9_new) < s32(extsh(flag)));
    const auto old_word = c.u32(words + (cc << 2));
    c.put32(words + (cc << 2), old_word + 0x20);
    c.put32(words + (cc << 2), c.u32(words + (cc << 2)) & 0xffffffc0u);
    c.set({{3, old_word}, {4, words}, {5, cc << 2}, {6, low ? 1u : 0u}, {7, r7_new}, {8, r8_new}, {9, r9_new}, {10, r10}});
}

// 0x10753980: the x word of the point `d` fitted to the pixel grid and, from the original coordinates of the four points, nudged by half a pixel's
// worth (0x40) in the cases the stem widths ask for, or the point `a` moved with it. (A leaf: r27-r31 are saved below the stack pointer.)
void snap_stem_edge(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t a, std::uint32_t b, std::uint32_t cc, std::uint32_t d) {
    const auto a_at = a << 2, b_at = b << 2, c_at = cc << 2, d_at = d << 2;
    const auto words = c.u32(h + hint_input::x_words);
    const auto originals = c.u32(handle + ag::original_xs);
    const auto word_d = c.u32(words + d_at), word_a = c.u32(words + a_at);
    auto fitted = grid(word_d);
    const auto o_a = c.u32(originals + a_at), o_b = c.u32(originals + b_at), o_c = c.u32(originals + c_at), o_d = c.u32(originals + d_at);
    const auto width = fitted - word_a;
    const auto original_ab = o_b - o_a, original_cd = o_d - o_c;
    std::uint32_t r3 = word_a, r6 = o_d, r7 = o_c, r8 = o_b, r10 = o_a;
    const auto leave = [&] {
        c.put32(words + d_at, fitted);
        c.set({{3, r3}, {4, d_at}, {5, fitted}, {6, r6}, {7, r7}, {8, r8}, {9, words}, {10, r10}});
    };
    if (s32(width) < 0xc0 || s32(original_ab) <= -8 || s32(original_cd) <= -8) { leave(); return; }
    const auto word_c = c.u32(words + c_at), word_b = c.u32(words + b_at);
    r10 = 0;
    const auto sum_cd = fitted - word_c;                // r28
    r7 = 0x19;
    const auto span_ab = word_b - word_a;               // r29
    r8 = sum_cd >> 31;
    const auto both = original_ab + original_cd;       // r12
    const std::uint32_t carry = sum_cd >= 0x20 ? 1 : 0;
    r6 = ~r8 + r10 + carry;                             // subfe: all ones when sum_cd < 0x20 (signed), else 0
    const auto edge_sum = span_ab + sum_cd;             // r27
    r6 = (r7 & r6) + both - 0x20;
    if (s32(edge_sum) < s32(r6)) {
        fitted += 0x40;
        leave();
        return;
    }
    r6 = both + 0x39;
    if (s32(edge_sum) > s32(r6)) {
        r3 += 0x40;
        c.put32(words + a_at, r3);
        leave();
        return;
    }
    r6 = ~r8 + r10 + carry;
    r6 = r7 & ~r6;
    r6 = original_cd - r6;
    if (s32(sum_cd) > s32(r6)) { leave(); return; }
    r6 = original_ab + 0x19;
    if (s32(span_ab) <= s32(r6)) { leave(); return; }
    r3 += 0x40;
    fitted += 0x40;
    c.put32(words + a_at, r3);
    leave();
}

// 0x10753ab0: one step of the bisection that finds the width the next edge is moved by: the error is the word of point `index` less the scaled
// reference (entry `reference` of the table, limited around the original) rounded; while it is not zero (and fewer than 14 steps are made) the bounds
// close on the middle, else the search starts over and the word takes the rounded value. (The original saves r28-r31 and the link register.)
void bisect_step(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t x, std::uint32_t reference, std::uint32_t index) {
    Frame frame(c, 0x10753ab0, 0x50, {.first_gpr = 28});
    const bool is_x = extsh(x) != 0;
    const auto words = c.u32(h + words_of(is_x));
    const auto originals = c.u32(handle + (is_x ? ag::original_xs : ag::original_ys));
    const auto at = extsh(index) << 2;
    const auto word = c.u32(words + at);
    const auto original = c.u32(originals + at);
    const auto clamped = limit_to_band(c, handle, reference, original);
    const auto rounded = grid(clamped);
    c.put32(handle + ag::search_error, word - rounded);
    const auto error = c.u32(handle + ag::search_error);
    const auto steps = c.u32(handle + ag::search_steps);
    std::uint32_t r3, r4 = rounded;
    if (error != 0 && s32(steps) < 0xe) {
        if (steps != 0) {
            const auto mid = c.u32(handle + ag::search_mid);
            if (s32(error) > 0) {
                c.put32(handle + ag::search_high, mid);
                const auto sum = c.u32(handle + ag::search_mid) + c.u32(handle + ag::search_low);
                c.put32(handle + ag::search_mid, sra(sum + (sum >> 31), 1));
            } else {
                c.put32(handle + ag::search_low, mid);
                const auto sum = c.u32(handle + ag::search_mid) + c.u32(handle + ag::search_high);
                c.put32(handle + ag::search_mid, sra(sum + (sum >> 31), 1));
            }
        }
        r3 = c.u32(handle + ag::search_steps) + 1;
        c.put32(handle + ag::search_steps, r3);
        c.put32(handle + ag::search_moved, 1);
    } else {
        c.put32(words + at, rounded);
        c.put32(handle + ag::search_mid, 0x1000);
        c.put32(handle + ag::search_low, 0x800);
        c.put32(handle + ag::search_high, 0x2000);
        c.put32(handle + ag::search_steps, 0);
        c.put32(handle + ag::search_moved, 0);
        r3 = 0x2000;
        r4 = 0x800;
    }
    c.set({{3, r3}, {4, r4}, {5, original}});
}

// 0x10754610: the stem width (halfword table at 0x390 for x, 0x3a8 for y; a width 0 is unused) nearest to `value` among those within half to one and a
// half times it; the table entry index plus 0x48 (x) or 0x54 (y), else -1. `x_flag` or, when it is zero, `y_flag` picks the table; both zero: none.
void find_standard_width(Cpu& c, std::uint32_t handle, std::uint32_t x_flag, std::uint32_t y_flag, std::uint32_t value) {
    Vol v(c);
    v[4] = 0xffffffff;
    v[6] = 0xffffffff;
    std::int32_t best = 0x7fff;
    const bool x = extsh(x_flag) != 0;
    if (!x) {
        v[5] = extsh(y_flag);
        if (extsh(y_flag) == 0) { v[3] = v[6]; v.store(c); return; }
    }
    const auto widths = handle + (x ? ag::x_stem_widths : ag::y_stem_widths);
    for (std::uint32_t k = 0; k < 12; ++k) {
        const auto entry = lha(c, widths + 2 * k);
        if (x) {
            v[8] = entry;
            v[5] = entry;
            if (entry == 0) continue;
            const auto half = sra(entry, 1);
            v[5] = extsh(entry - value);
            v[9] = extsh(entry + half);
            v[8] = extsh(entry - half);
            if (s32(v[5]) < 0) v[5] = extsh(negate(v[5]));
            if (s32(v[5]) < best && s32(value) >= s32(v[8]) && s32(value) <= s32(v[9])) { best = s32(v[5]); v[4] = k; }
        } else {
            v[10] = entry;
            v[5] = entry;
            if (entry == 0) continue;
            const auto half = sra(entry, 1);
            auto difference = extsh(entry - value);  // r31
            v[8] = extsh(entry + half);
            v[5] = entry - half;
            v[10] = extsh(v[5]);
            if (s32(difference) < 0) { v[5] = negate(difference); difference = extsh(v[5]); }
            if (s32(difference) < best && s32(value) >= s32(v[10]) && s32(value) <= s32(v[8])) { best = s32(difference); v[4] = k; }
        }
    }
    v[x ? 10 : 9] = 0x18;
    if (s32(v[4]) >= 0) v[6] = extsh(v[4] + (x ? ag::x_stems : ag::y_stems));
    v[3] = v[6];
    v.store(c);
}

// 0x10754810 and 0x107548d0: the largest (at least 0) of the twelve x (y) stem widths; r4 is the last one read, r5 = 0x18, r6 = 10.
void widest_stem(Cpu& c, std::uint32_t handle, bool x) {
    const auto widths = handle + (x ? ag::x_stem_widths : ag::y_stem_widths);
    std::uint32_t widest = 0, last = 0;
    for (std::uint32_t k = 0; k < 12; ++k) {
        last = lha(c, widths + 2 * k);
        if (s32(last) > s32(extsh(widest))) widest = last;
    }
    c.set({{3, widest}, {4, last}, {5, 0x18}, {6, 10}, {7, widest}});
}

// 0x107536b0: a stem edge placed between two points. The stack words (caller sp + 0x3b a byte, + 0x3c, + 0x40) are the keep-visible byte and the two
// point indices i and j. The distance of the two points' halfword coordinates (array r5) picks a standard width (0x10754610); a width found goes to the
// x (the strategy's 0x10754550 or 0x107545b0 by the sign of the distance), else the placement callback is called with no width.
void choose_edge(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t halves, std::uint32_t direction, std::uint32_t r7, std::uint32_t r8,
                 std::uint32_t r9, std::uint32_t r10) {
    const auto caller_sp = c.sp();
    const auto i = c.u32(caller_sp + 0x3c), j = c.u32(caller_sp + 0x40);
    const auto keep = c.u8(caller_sp + 0x3b);
    Frame frame(c, 0x107536b0, 0x70, {.first_gpr = 22});
    Vol v(c);
    auto distance = lha(c, halves + (j << 1)) - lha(c, halves + (i << 1));
    std::uint32_t forward;
    if (s32(distance) < 0) { distance = negate(distance); forward = 0; } else forward = 1;
    // find_standard_width(handle, direction, r7, distance) with r6 = 0
    v[3] = handle; v[4] = direction; v[5] = r7; v[7] = distance; v[6] = 0;
    v.store(c);
    find_standard_width(c, handle, direction, r7, distance);
    v.load(c);
    v[5] = v[3];
    if (s32(extsh(v[5])) >= 0) {
        v[3] = handle; v[4] = h; v[6] = i; v[7] = j; v[8] = direction;
        call_glue(c, v, forward != 0 ? -0x5b5c : -0x5b60, 0x1075375c);
    } else {
        c.put32(frame.at(0x38), i);
        c.put32(frame.at(0x3c), j);
        v[3] = handle; v[4] = h; v[7] = r8; v[8] = r9; v[9] = r10; v[10] = keep; v[5] = 0xffffffff; v[6] = 1;
        call_glue(c, v, extsh(direction) == 0 ? -0x5b68 : -0x5b64, 0x107537a4);
    }
    v.store(c);
}

// 0x10754190 and 0x10754370: the points that no edge has touched (x: flag 0x10 in the point flags, y: 0x20) interpolated between the touched ones of their
// contour: where the two touched points' original coordinates differ, in proportion to them (16.16 steps when the fitted span is large, else a
// multiply and divide), where they are equal, shifted by the first one's movement. The contour is a ring (next_point).
void interpolate_untouched(Cpu& c, std::uint32_t handle, std::uint32_t h, bool x) {
    Frame frame(c, x ? 0x10754190u : 0x10754370u, 0x70, {.first_gpr = 19});
    Vol v(c);
    const std::uint16_t touched = x ? 0x10 : 0x20;
    const auto ring = c.u32(handle + ag::next_point), flags = c.u32(handle + ag::point_flags);
    const auto originals = [&] { return c.u32(handle + (x ? ag::original_xs : ag::original_ys)); };
    const auto words = [&] { return c.u32(h + words_of(x)); };
    std::uint32_t contour = 0, contour_at = 0;
    while (s32(contour) < s32(lha(c, h + hint_input::contour_count))) {
        const auto ends = c.u32(h + hint_input::contour_ends), starts = c.u32(h + hint_input::contour_starts);
        v[4] = ends; v[3] = starts;
        std::uint32_t end = lha(c, ends + contour_at), first = lha(c, starts + contour_at);
        v[5] = end;
        if (s32(end) > s32(first)) {
            std::uint32_t point = first, at = first << 1;
            v[4] = at; v[3] = flags + at;
            std::uint32_t pointer = flags + at;
            while (!(c.u16(pointer) & touched) && s32(point) <= s32(end)) {
                v[4] += 2; v[3] += 2; pointer += 2; ++point;
            }
            if (s32(point) <= s32(end)) {
                const auto start = point;
                std::uint32_t current = point;
                point = lha(c, ring + (current << 1));
                for (;;) {
                    v[3] = point << 1;
                    while (!(c.u16(flags + (point << 1)) & touched)) {
                        point = lha(c, ring + (point << 1));
                        v[3] = point << 1;
                    }
                    const auto original_current = c.u32(originals() + (current << 2)), original_next = c.u32(originals() + (point << 2));
                    const auto fitted_current = c.u32(words() + (current << 2));
                    v[4] = originals();
                    v[3] = current << 2;
                    v[6] = point << 2;
                    v[5] = words();
                    auto original_span = original_current - original_next;
                    v[4] = original_span;
                    if (original_span == 0) {
                        v[6] = fitted_current - original_current;
                        std::uint32_t walk = lha(c, ring + (current << 1));
                        while (walk != point) {
                            v[5] = words();
                            v[4] = walk << 2;
                            const auto value = c.u32(words() + (walk << 2)) + v[6];
                            v[3] = value;
                            c.put32(words() + (walk << 2), value);
                            walk = lha(c, ring + (walk << 1));
                        }
                    } else {
                        const auto fitted_next = c.u32(words() + (point << 2));
                        const auto fitted_span = fitted_current - fitted_next;
                        v[3] = fitted_span;
                        if (s32(fitted_span) > 0x2000 || s32(fitted_span) < -0x2000) {
                            Leftovers left{v[4], v[5], v[6], v[7], v[8]};
                            const auto ratio = std::uint32_t(fix_divide(s32(fitted_span), s32(original_span), left));
                            v[3] = ratio; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
                            std::uint32_t walk = lha(c, ring + (current << 1));
                            while (walk != point) {
                                v[3] = originals();
                                const auto original = c.u32(originals() + (walk << 2));
                                v[4] = ratio;
                                v[3] = original - original_current;
                                Leftovers left2{v[4], v[5], v[6], v[7], v[8]};
                                const auto product = std::uint32_t(fix_multiply(s32(v[3]), s32(ratio), left2));
                                v[3] = product; v[4] = left2.r4; v[5] = left2.r5; v[6] = left2.r6; v[7] = left2.r7; v[8] = left2.r8;
                                v[4] = words();
                                v[3] = fitted_current + product;
                                c.put32(words() + (walk << 2), v[3]);
                                walk = lha(c, ring + (walk << 1));
                            }
                        } else {
                            std::uint32_t walk = lha(c, ring + (current << 1));
                            while (walk != point) {
                                v[6] = originals();
                                v[7] = walk << 2;
                                v[5] = words();
                                v[6] = c.u32(originals() + (walk << 2));
                                v[6] = v[6] - original_current;
                                v[6] = fitted_span * v[6];
                                v[6] = divw(v[6], original_span);
                                v[6] = fitted_current + v[6];
                                c.put32(words() + (walk << 2), v[6]);
                                walk = lha(c, ring + (walk << 1));
                            }
                        }
                    }
                    if (point == start) break;
                    current = point;
                    point = lha(c, ring + (current << 1));
                }
            }
        }
        contour_at += 2;
        ++contour;
    }
    v.store(c);
}

// ---- the passes that drive the hint execution ----------------------------------------------------------------------------------------------

// 0x10753560: each contour as a ring (next_point and previous_point), then the analysis passes of the first half (0x10751fd0, 0x10752990, 0x10753110),
// then the points whose flag bit 2 is set that 0x10751bd0 accepts (result >= 0) get the bit 0x40.
void link_contours(Cpu& c, std::uint32_t handle) {
    Frame frame(c, 0x10753560, 0x60, {.first_gpr = 24});
    Vol v(c);
    const auto previous = c.u32(handle + ag::previous_point), next = c.u32(handle + ag::next_point);
    v[7] = 0; v[5] = 0; v[10] = previous;
    while (s32(v[7]) < s32(lha(c, handle + ag::contour_count))) {
        v[4] = c.u32(handle + ag::contour_ends);
        v[3] = c.u32(handle + ag::contour_starts);
        v[8] = lha(c, v[4] + v[5]);
        v[9] = lha(c, v[3] + v[5]);
        if (s32(v[8]) <= s32(v[9])) {
            c.put16(previous + (v[9] << 1), v[9]);
            c.put16(next + (v[9] << 1), v[9]);
        } else {
            v[3] = v[8]; v[4] = v[9]; v[6] = v[9] + 1;
            while (s32(v[4]) <= s32(v[8])) {
                const auto at = v[4] << 1;
                c.put16(previous + at, v[3]);
                v[3] = v[4];
                v[4] = v[6];
                c.put16(next + at, v[6]);
                v[6] += 1;
            }
            c.put16(next + (v[8] << 1), v[9]);
        }
        v[5] += 2;
        v[7] += 1;
    }
    const auto hold = [&](std::uint32_t flags, std::uint32_t point, std::uint32_t last, std::uint32_t contour, std::uint32_t at, std::uint32_t pointer) {
        c.set({{24, flags}, {25, point}, {26, last}, {27, contour}, {28, at}, {29, point << 1}, {30, pointer}, {31, handle}});
    };
    c.set(31, handle);
    for (const auto [entry, back] : std::array<std::pair<std::uint32_t, std::uint32_t>, 3>{{{0x10751fd0, 0x10753604}, {0x10752990, 0x1075360c}, {0x10753110, 0x10753614}}}) {
        v[3] = handle;
        v.store(c);
        c.call(entry, back);
        v.load(c);
    }
    const auto flags = c.u32(handle + ag::point_flags);
    std::uint32_t contour = 0, at = 0;
    while (s32(contour) < s32(lha(c, handle + ag::contour_count))) {
        v[4] = c.u32(handle + ag::contour_ends);
        v[3] = c.u32(handle + ag::contour_starts);
        const auto last = lha(c, v[4] + at);
        std::uint32_t point = lha(c, v[3] + at);
        if (s32(last) > s32(point)) {
            std::uint32_t pointer = flags + (point << 1);
            for (; s32(point) <= s32(last); ++point, pointer += 2) {
                if (!(c.u16(pointer) & 2)) continue;
                v[3] = handle; v[4] = point;
                hold(flags, point, last, contour, at, pointer);
                v.store(c);
                c.call(0x10751bd0, 0x10753660);
                v.load(c);
                if (s32(extsh(v[3])) >= 0) c.put16(pointer, c.u16(pointer) | 0x40);
            }
        }
        at += 2;
        ++contour;
    }
    v.store(c);
}

// 0x10754990: the edge records of the contours (see autogrid::records): for each point with a partner a record of kind 1 when the point carries flag bit
// 0x40, and kind 3 links to the partner before and after it when the partner points back (the after partner not when the two partners are the same point
// and the directions into and out of the point are within 30 degrees); each record's class from its direction; the records sorted by priority with
// 8-byte swaps through the game's byte move. The number of records goes to *count_out.
void build_edge_records(Cpu& c, std::uint32_t handle, std::uint32_t out, std::uint32_t count_out) {
    Frame frame(c, 0x10754990, 0x60, {.first_gpr = 24});
    Vol v(c);
    v[10] = 0; v[6] = 0; v[7] = 0;
    std::uint32_t count = 0;
    const auto capacity = [&] { return c.u32(handle + ag::record_capacity); };
    while (s32(v[7]) < s32(lha(c, handle + ag::contour_count)) && s32(count) < s32(capacity())) {
        v[5] = c.u32(handle + ag::contour_ends);
        v[4] = c.u32(handle + ag::contour_starts);
        v[9] = lha(c, v[5] + v[6]);
        v[8] = lha(c, v[4] + v[6]);
        if (s32(v[9]) > s32(v[8])) {
            std::uint32_t remaining = v[9] + 1 - v[8];
            v[4] = v[8] << 1;
            v[5] = v[8] << 2;
            for (bool stop = false; !stop && remaining != 0; --remaining) {
                std::uint32_t before_flag = 0;
                const auto before = lha(c, c.u32(handle + ag::partner_before) + v[4]);
                v[9] = 0;
                if (s32(before) >= 0) before_flag = 1;
                const auto after = lha(c, c.u32(handle + ag::partner_after) + v[4]);
                if (s32(after) >= 0) v[9] = 1;
                if (v[9] != 0 || before_flag != 0) {
                    if (c.u16(c.u32(handle + ag::point_flags) + v[4]) & 0x40) {
                        const auto record = out + v[10];
                        c.put8(record, 1);
                        ++count;
                        v[10] += 8;
                        c.put8(record + 3, 1);
                        c.put16(record + 4, v[8]);
                        c.put16(record + 6, v[8]);
                    }
                    if (before_flag != 0) {
                        const auto partner = before << 1;
                        if (v[8] == lha(c, c.u32(handle + ag::partner_before) + partner) || v[8] == lha(c, c.u32(handle + ag::partner_after) + partner)) {
                            const auto record = out + v[10];
                            c.put8(record, 3);
                            c.put8(record + 3, 5);
                            ++count;
                            c.put8(record + 2, 1);
                            v[10] += 8;
                            c.put16(record + 4, before);
                            c.put16(record + 6, v[8]);
                        }
                    }
                    if (v[9] != 0) {
                        bool place = true;
                        if (after == before) {
                            const auto in_dx = c.u32(c.u32(handle + ag::in_dx) + v[5]), out_dx = c.u32(c.u32(handle + ag::out_dx) + v[5]);
                            const auto in_dy = c.u32(c.u32(handle + ag::in_dy) + v[5]), out_dy = c.u32(c.u32(handle + ag::out_dy) + v[5]);
                            v[9] = sra(in_dx * out_dx + in_dy * out_dy, 14);
                            if (s32(v[9]) > 0x376d) place = false;
                        }
                        if (place) {
                            const auto partner = after << 1;
                            v[9] = lha(c, c.u32(handle + ag::partner_before) + partner);
                            if (v[8] != v[9]) {
                                v[9] = lha(c, c.u32(handle + ag::partner_after) + partner);
                                if (v[8] != v[9]) place = false;
                            }
                        }
                        if (place) {
                            const auto record = out + v[10];
                            c.put8(record, 3);
                            c.put8(record + 3, 5);
                            v[9] = 0;
                            ++count;
                            v[10] += 8;
                            c.put8(record + 2, 0);
                            c.put16(record + 4, after);
                            c.put16(record + 6, v[8]);
                            if (s32(count) >= s32(capacity())) { stop = true; continue; }
                        }
                    }
                }
                v[4] += 2; v[5] += 4; v[8] += 1;
            }
        }
        v[6] += 2;
        v[7] += 1;
    }
    // The class of each record from the direction its point has in (kind 1 of the direction byte) or out of it: 2 when the first component is steep, 1 the
    // second, 0xff neither.
    v[6] = 0;
    if (s32(count) > 0) {
        for (std::uint32_t k = 0; k < count; ++k) {
            v[7] = out + v[6];
            const auto direction = c.u8(v[7] + 2);
            v[4] = lha(c, v[7] + 6);
            std::uint32_t y_value;
            const auto at = v[4] << 2;
            if (direction != 0) {
                v[5] = c.u32(handle + ag::in_dx);
                v[4] = c.u32(handle + ag::in_dy);
            } else {
                v[5] = c.u32(handle + ag::out_dx);
                v[4] = c.u32(handle + ag::out_dy);
            }
            v[5] = c.u32(v[5] + at);
            y_value = c.u32(v[4] + at);
            if (s32(v[5]) > 0x3dd2 || s32(v[5]) < -0x3dd2) c.put8(v[7] + 1, 2);
            else if (s32(y_value) > 0x3dd2 || s32(y_value) < -0x3dd2) c.put8(v[7] + 1, 1);
            else c.put8(v[7] + 1, 0xff);
            v[6] += 8;
        }
    }
    // The records ordered by priority (byte 3), a bubble sort whose swaps go through the byte move.
    const auto last = count - 1;
    const auto temporary = frame.at(0x38);
    for (bool swapped = true; swapped;) {
        swapped = false;
        std::uint32_t k = 0, at = 0;
        for (; s32(k) < s32(last); ++k, at += 8) {
            auto record = out + at;
            v[3] = c.u8(record + 3);
            const auto next_priority = c.u8(record + 0xb);
            if (!(v[3] > next_priority)) continue;
            v[4] = record; v[3] = temporary; swapped = true; v[5] = 8;
            v.store(c);
            c.call(byte_move, 0x10754c70);
            v.load(c);
            v[3] = record;
            v[5] = 8;
            record = out + ((k + 1) << 3);
            v[4] = record;
            v.store(c);
            c.call(byte_move, 0x10754c8c);
            v.load(c);
            v[3] = record; v[4] = temporary; v[5] = 8;
            v.store(c);
            c.call(byte_move, 0x10754c9c);
            v.load(c);
        }
    }
    c.put32(count_out, extsh(count));
    v.store(c);
}

// 0x10754cd0: the widths (halfword distances of the two points' coordinates, at most a third of the units per em plus one) of the kind-3 records whose class
// is 1 (x) and 2 (y), two arrays of the allocator's blocks; the arrays and their counts go through the four pointers. Returns 0. (An allocation never
// returns null, it raises, so the original's failure exit, which releases both blocks and returns -1, cannot run.)
void collect_stem_widths(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t x_array_out, std::uint32_t x_count_out, std::uint32_t y_array_out,
                         std::uint32_t y_count_out) {
    Frame frame(c, 0x10754cd0, 0x70, {.first_gpr = 20});
    Vol v(c);
    const auto units = lha(c, handle + ag::units_per_em);
    auto third = mulhw(0x55555556, units);
    third += third >> 31;
    const auto limit = extsh(third + 1);
    std::uint32_t x_count = 0, y_count = 0;
    v[3] = 0; v[5] = 0; v[4] = 0;
    while (s32(v[4]) < s32(c.u32(handle + ag::record_count))) {
        v[7] = c.u32(handle + ag::records) + v[3];
        if (c.u8(v[7]) == 3) {
            const auto kind = c.u8(v[7] + 1);
            if (kind == 1 || kind == 2) {
                v[6] = lha(c, v[7] + 4);
                const auto b = lha(c, v[7] + 6);
                const auto array = c.u32(h + (kind == 1 ? hint_input::xs : hint_input::ys));
                v[7] = array;
                v[6] = lha(c, array + (v[6] << 1));
                auto distance = extsh(lha(c, array + (b << 1)) - v[6]);
                if (s32(distance) < 0) distance = extsh(negate(distance));
                if (s32(distance) <= s32(limit)) (kind == 1 ? x_count : y_count) += 1;
            }
        }
        v[3] += 8;
        v[4] += 1;
    }
    const auto mem = c.u32(handle + ag::mem);
    v[3] = mem; v[4] = (x_count + 1) * 2;
    v.store(c);
    c.link(0x10754e00);
    const auto x_array = allocate_block(c, mem, (x_count + 1) * 2);
    v.load(c);
    v[3] = mem; v[4] = (y_count + 1) * 2;
    v.store(c);
    c.link(0x10754e18);
    const auto y_array = allocate_block(c, mem, (y_count + 1) * 2);
    v.load(c);
    // Both allocations succeeded (a failed one longjmps out): the arrays are filled with the widths.
    std::uint32_t found_x = 0, found_y = 0;
    v[6] = 0; v[4] = 0; v[3] = 0; v[5] = 0;
    while (s32(v[4]) < s32(c.u32(handle + ag::record_count))) {
        v[8] = c.u32(handle + ag::records) + v[3];
        if (c.u8(v[8]) == 3) {
            const auto kind = c.u8(v[8] + 1);
            if (kind == 1 || kind == 2) {
                v[7] = lha(c, v[8] + 4);
                const auto b = lha(c, v[8] + 6);
                const auto array = c.u32(h + (kind == 1 ? hint_input::xs : hint_input::ys));
                v[8] = array;
                v[7] = lha(c, array + (v[7] << 1));
                v[7] = extsh(lha(c, array + (b << 1)) - v[7]);
                if (s32(v[7]) < 0) v[7] = extsh(negate(v[7]));
                if (s32(extsh(v[7])) <= s32(limit)) {
                    if (kind == 1) { c.put16(x_array + v[5], v[7]); ++found_x; v[5] += 2; }
                    else { c.put16(y_array + v[6], v[7]); ++found_y; v[6] += 2; }
                }
            }
        }
        v[3] += 8;
        v[4] += 1;
    }
    c.put32(x_array_out, x_array);
    v[3] = 0;
    c.put32(x_count_out, found_x);
    c.put32(y_array_out, y_array);
    c.put32(y_count_out, found_y);
    v.store(c);
}

// ---- the hint execution and the hint handle (the hinter's API as the scaler calls it) --------------------------------------------------------

// 0x10754ff0: one run of the hinter over the glyph `h` for the handle. `mode` (3: the analysis of the first half only, 0, 1, 2: the strategy / the
// stem widths), r5 goes to autogrid::mode. r6 must be 2 or 3 (a halfword compare), r8-r10 and the caller's stack word (sp + 0x38) are the four values the
// stem-width collection (mode 2) hands on. Returns 0, -100 (mode 3 with unordered limits), -1 (an unknown mode) or the strategy's/collection's result.
// Assertions (0x107755c0, the unit's pool) report a mode out of range, a glyph with too many points, a strategy that fails and an unknown mode.
void run_hinter(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t mode_word, std::uint32_t size_flag, std::uint32_t mode, std::uint32_t r8,
                std::uint32_t r9, std::uint32_t r10) {
    const auto stack_arg = c.u32(c.sp() + 0x38);
    Frame frame(c, 0x10754ff0, 0x60, {.first_gpr = 23});
    Vol v(c);
    const auto pool = c.toc(toc::autogrid_pool);
    std::uint32_t status = 0;
    const auto hold = [&] {
        c.set({{23, handle}, {24, h}, {25, mode}, {26, r8}, {27, r9}, {28, r10}, {29, stack_arg}, {30, status}, {31, pool}});
    };
    const auto assert_at = [&](std::uint32_t text, std::uint32_t line, std::uint32_t back) {
        v.store(c);
        hold();
        dispatched::assertion(c, pool, text, pool_file, line, back);
        v.load(c);
    };
    v[6] = (size_flag - 2) & 0xffff;
    v[3] = 1 | ~v[6];
    c.put16(handle + ag::mode, mode_word);
    if (!(v[6] <= 1)) assert_at(0x5b, 0xf67, 0x10755068);
    v[4] = c.u32(h + hint_input::contour_ends);
    v[3] = lha(c, v[4] + (lha(c, h + hint_input::contour_count) << 1) - 2) + 5;
    if (s32(v[3]) > s32(c.u32(handle + ag::capacity))) assert_at(0x7c, 0xf69, 0x1075509c);
    v[3] = lha(c, h + hint_input::contour_count);
    c.put16(handle + ag::contour_count, v[3]);
    c.put32(handle + ag::contour_starts, c.u32(h + hint_input::contour_starts));
    c.put32(handle + ag::contour_ends, c.u32(h + hint_input::contour_ends));
    c.put32(handle + ag::on_curve, c.u32(h + hint_input::on_curve));
    c.put32(handle + ag::xs, c.u32(h + hint_input::xs));
    c.put32(handle + ag::ys, c.u32(h + hint_input::ys));
    if (extsh(mode) == 3) {
        if (c.u32(handle + ag::limits_unordered) != 0) {
            status = 0xffffff9c;
        } else {
            v[3] = handle; v[4] = h;
            v.store(c);
            hold();
            c.call(0x10751b10, 0x107550f8);
            v.load(c);
        }
        v[3] = status;
        v.store(c);
        return;
    }
    c.put32(handle + ag::record_count, 0);
    v[3] = handle;
    v.store(c);
    hold();
    c.link(0x10755110);
    link_contours(c, handle);
    v.load(c);
    v[4] = c.u32(handle + ag::records);
    v[3] = handle;
    v[5] = handle + ag::record_count;
    v.store(c);
    hold();
    c.link(0x10755120);
    build_edge_records(c, handle, v[4], handle + ag::record_count);
    v.load(c);
    if (extsh(mode) == 0) {
        v[3] = h; v[4] = handle;
        v.store(c);
        hold();
        c.call(0x1075cbc0, 0x10755134);
        v.load(c);
        status = v[3];
        if (status != 0) assert_at(0xc7, 0xf89, 0x1075514c);
    } else if (extsh(mode) == 2) {
        v[3] = handle; v[4] = h; v[5] = r8; v[6] = r9; v[7] = r10; v[8] = stack_arg;
        v.store(c);
        hold();
        c.link(0x10755178);
        collect_stem_widths(c, handle, h, r8, r9, r10, stack_arg);
        v.load(c);
        status = v[3];
    } else if (extsh(mode) != 1) {
        assert_at(0xd3, 0xfb8, 0x10755198);
        status = 0xffffffff;
    }
    v[3] = status;
    v.store(c);
}

// 0x10754f70: run_hinter for a valid handle (both stamps), with the mode 2 (the stem widths); -1 for another handle. The arguments after the glyph shift
// by one place (r7 = mode 2; r8-r10 and the stack word are the four pointers).
void run_hinter_checked(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t mode_word, std::uint32_t size_flag, std::uint32_t r7, std::uint32_t r8,
                        std::uint32_t r9, std::uint32_t r10) {
    Frame frame(c, 0x10754f70, 0x40, {});
    Vol v(c);
    const auto stamps = check_stamps(c, handle);
    v[9] = 0;
    if (handle != 0) v[8] = stamps.first == ag::stamp_value ? stamps.second : stamps.first;
    if (!stamps.valid) { v[3] = 0xffffffff; v.store(c); return; }
    c.put32(frame.at(0x38), r10);
    v[8] = r7; v[9] = r8; v[10] = r9; v[7] = 2;
    v.store(c);
    c.link(0x10754fdc);
    run_hinter(c, handle, h, mode_word, size_flag, 2, r7, r8, r9);
}

// 0x107551c0: the font's hint data (0x6c bytes of ten height records, copied through the byte move, each checked: the second halfword is the sum of the
// first and third) given to the handle, and the flavor (1 or 2). -1 for another handle. A null `data` only sets the flavor.
void set_hint_data(Cpu& c, std::uint32_t handle, std::uint32_t data, std::uint32_t flavor) {
    Frame frame(c, 0x107551c0, 0x50, {.first_gpr = 28});
    Vol v(c);
    const auto pool = c.toc(toc::autogrid_pool);
    v[6] = 0;
    const auto stamps = check_stamps(c, handle);
    if (handle != 0) v[3] = stamps.first != ag::stamp_value ? stamps.first : stamps.second;
    if (stamps.valid) v[6] = 1;
    if (!stamps.valid) { v[3] = 0xffffffff; v.store(c); return; }
    c.put32(handle + ag::flavor, flavor);
    if (data != 0) {
        v[3] = handle + ag::height_records; v[5] = 0x6c;
        v.store(c);
        c.set({{28, handle}, {31, pool}});
        c.call(byte_move, 0x10755238);
        v.load(c);
        for (std::uint32_t record = 0, at = 0; record <= 9; ++record, at += 6) {
            v[4] = handle + at;
            v[3] = lha(c, v[4] + ag::height_records);
            const auto third = lha(c, v[4] + ag::height_records + 4);
            v[4] = lha(c, v[4] + ag::height_records + 2);
            if (v[4] != v[3] + third) {
                v.store(c);
                c.set({{28, handle}, {29, record}, {30, at}, {31, pool}});
                dispatched::assertion(c, pool, 0xd9, pool_file, 0xfd8, 0x10755270);
                v.load(c);
            }
        }
    }
    c.put8(handle + ag::data_set, 1);
    v[3] = 0;
    v.store(c);
}

// 0x107552b0: the glyph hinted. `quality` zero copies the glyph's x and y words to the originals (and the advance, from the phantom points, to
// hint_input::advance); the data and the flavor must be set (assertions otherwise, and `quality` must be 0 or 3); then run_hinter (mode `quality`, r6/r7
// the caller's, no pointers) and the advance in pixels. r8 nonzero makes keep_flat 1. Returns run_hinter's result.
void hint_glyph(Cpu& c, std::uint32_t handle, std::uint32_t h, std::uint32_t quality, std::uint32_t r6, std::uint32_t r7, std::uint32_t r8, std::uint32_t r9) {
    Frame frame(c, 0x107552b0, 0x60, {.first_gpr = 24});
    Vol v(c);
    const auto pool = c.toc(toc::autogrid_pool);
    const auto hold = [&] {
        c.set({{24, r8}, {25, h}, {26, quality}, {27, r6}, {28, r7}, {29, r9}, {30, handle}, {31, pool}});
    };
    const auto assert_at = [&](std::uint32_t text, std::uint32_t line, std::uint32_t back) {
        v.store(c);
        hold();
        dispatched::assertion(c, pool, text, pool_file, line, back);
        v.load(c);
    };
    v[5] = 0;
    const auto stamps = check_stamps(c, handle);
    if (handle != 0) { v[4] = stamps.first; if (stamps.first == ag::stamp_value) v[3] = stamps.second; }
    if (stamps.valid) v[5] = 1;
    if (v[5] == 0) assert_at(0x139, 0xff0, 0x10755328);
    v[3] = extsh(r8);
    c.put32(handle + ag::keep_flat, v[3] != 0 ? 1 : 0);
    if (extsh(quality) == 0) {
        v[3] = lha(c, h + hint_input::point_count);
        v[9] = c.u32(handle + ag::original_xs);
        v[8] = 0;
        v[4] = v[3] + extsh(r9);
        v[5] = c.u32(h + hint_input::x_words);
        const auto total = v[4];
        const auto copy = [&](std::uint32_t from, std::uint32_t to) {
            for (std::uint32_t n = 0; n < total; ++n) c.put32(to + 4 * n, c.u32(from + 4 * n));
        };
        // The original copies in passes of eight words and then the rest; r6-r8 are what its counters leave.
        if (s32(total) > 0) {
            if (s32(total) > 8) {
                const auto passes = (total - 8 + 7) >> 3;
                v[6] = 0x20 * passes; v[8] = 8 * passes; v[7] = v[9] + 0x20 * (passes - 1);
            }
            copy(v[5], v[9]);
            v[3] = v[9] + 4 * total;
            v[6] = v[5] + 4 * total;
        }
        v[3] = 0;
        const auto points = lha(c, h + hint_input::point_count);
        const auto words = v[5];
        v[6] = words + (points << 2);
        v[5] = c.u32(words + (points << 2));
        c.put32(h + hint_input::advance, c.u32(v[6] + 4) - v[5]);
        v[8] = c.u32(handle + ag::original_ys);
        v[9] = c.u32(h + hint_input::y_words);
        if (s32(total) > 0) {
            std::uint32_t done = 0;
            if (s32(total) > 8) {
                const auto passes = (total - 8 + 7) >> 3;
                done = 8 * passes;
                v[7] = v[8] + 0x20 * (passes - 1);
            }
            copy(v[9], v[8]);
            v[3] = done;
            v[5] = v[8] + 4 * total;
            v[6] = v[9] + 4 * total;
        }
    }
    v[3] = c.u32(handle + ag::flavor);
    if (!(v[3] == 1 || v[3] == 2)) assert_at(0x155, 0x1007, 0x10755514);
    if (extsb(c.u8(handle + ag::data_set)) == 0) assert_at(0x190, 0x1008, 0x10755530);
    v[3] = 0;
    if (extsh(quality) == 3 || extsh(quality) == 0) v[3] = 1;
    if (v[3] == 0) assert_at(0x1ac, 0x1009, 0x10755564);
    c.put32(frame.at(0x38), 0);
    v[3] = handle; v[4] = h; v[5] = r6; v[6] = r7; v[7] = quality; v[8] = 0; v[9] = 0; v[10] = 0;
    v.store(c);
    hold();
    c.link(0x10755590);
    run_hinter(c, handle, h, r6, r7, quality, 0, 0, 0);
    v.load(c);
    const auto points = lha(c, h + hint_input::point_count);
    v[5] = c.u32(h + hint_input::x_words);
    const auto at = points << 2;
    v[4] = v[5] + at;
    v[5] = c.u32(v[5] + at);
    v[4] = c.u32(v[4] + 4) - v[5];
    c.put32(h + hint_input::advance_pixels, sra(v[4] + 0x20, 6));
    v.store(c);
}

// 0x107555d0: the hint handle for a memory object, a point capacity (the font's largest outline's points) and the resolution (units per em): the object
// (0x45c bytes) and its tables from the allocator (the order of the calls is the original's), the constants, stamps and flavor 1; *handle_out gets it.
// Returns 0. (The allocator raises where the original would test for null, so the failure exits cannot run.)
void new_hint_handle(Cpu& c, std::uint32_t mem, std::uint32_t max_points, std::uint32_t units, std::uint32_t handle_out) {
    Frame frame(c, 0x107555d0, 0x60, {.first_gpr = 23});
    Vol v(c);
    const auto allocate = [&](std::uint32_t size, std::uint32_t back) {
        v[3] = mem; v[4] = size;
        v.store(c);
        c.link(back);
        const auto block = allocate_block(c, mem, size);
        v.load(c);
        return block;
    };
    v[4] = 0x45c;
    const auto handle = allocate(0x45c, 0x107555fc);
    const auto capacity = max_points + 4;
    c.put32(handle + ag::mem, mem);
    c.put32(handle + ag::stamp, ag::stamp_value);
    c.put32(handle + ag::end_stamp, ag::end_stamp_value);
    v[3] = ag::end_stamp_value;
    for (const auto [at, value] : {std::pair{0x3c0u, 2u}, {0x3c2u, 0x10u}, {0x3c4u, 0x10u}, {0x3c6u, 0x20u}, {0x3c8u, 0u}, {0x3cau, 0x100u}, {0x3ccu, 0u}})
        c.put16(handle + at, value);
    v[8] = 0x10; v[7] = 0x20; v[6] = 0; v[5] = 0x100;
    c.put32(handle + ag::flavor, 1);
    c.put8(handle + ag::ready, 0);
    c.put8(handle + ag::data_set, 0);
    c.put32(handle + ag::capacity, capacity);
    c.put16(handle + ag::units_per_em, units);
    c.put32(handle + ag::bytes_3d8, allocate(capacity, 0x10755680));
    const auto doubled = capacity << 1, quadrupled = capacity << 2, sixfold = capacity * 6;
    c.put32(handle + ag::original_xs, allocate(capacity << 3, 0x10755694));
    v[5] = c.u32(handle + ag::original_xs);
    c.put32(handle + ag::original_ys, c.u32(handle + ag::original_xs) + quadrupled);
    c.put32(handle + ag::next_point, allocate(sixfold, 0x107556c0));
    c.put32(handle + ag::previous_point, c.u32(handle + ag::next_point) + doubled);
    c.put32(handle + ag::extrema, c.u32(handle + ag::next_point) + quadrupled);
    c.put32(handle + ag::point_flags, allocate(doubled, 0x107556e8));
    c.put32(handle + ag::curve_xs, allocate(quadrupled, 0x107556f8));
    c.put32(handle + ag::curve_ys, c.u32(handle + ag::curve_xs) + doubled);
    c.put32(handle + ag::partner_before, allocate(sixfold, 0x10755714));
    c.put32(handle + ag::partner_after, c.u32(handle + ag::partner_before) + doubled);
    c.put32(handle + 0x40 /* a third halfword array, not read here */, c.u32(handle + ag::partner_before) + quadrupled);
    c.put32(handle + ag::in_dx, allocate(capacity << 4, 0x1075573c));
    v[4] = max_points * 2;
    v[6] = capacity << 3;
    v[7] = c.u32(handle + ag::in_dx);
    v[5] = capacity * 12;
    c.put32(handle + ag::in_dy, c.u32(handle + ag::in_dx) + quadrupled);
    c.put32(handle + ag::out_dx, c.u32(handle + ag::in_dx) + (capacity << 3));
    c.put32(handle + ag::out_dy, c.u32(handle + ag::in_dx) + capacity * 12);
    c.put32(handle + ag::record_capacity, max_points * 2 + 8);
    c.put32(handle + ag::records, allocate(c.u32(handle + ag::record_capacity) << 3, 0x1075578c));
    c.put32(handle_out, handle);
    v[3] = 0;
    v.store(c);
}

// 0x10755840: the handle's blocks and the object released through the allocator's release (a stack leaver, called at this frame's sp with r30 = the
// memory object and r31 = the handle); 0 for no handle, -1 for another pointer.
void delete_hint_handle(Cpu& c, std::uint32_t handle) {
    Frame frame(c, 0x10755840, 0x40, {.first_gpr = 30});
    Vol v(c);
    if (handle == 0) { v[3] = 0; v.store(c); return; }
    const auto stamps = check_stamps(c, handle);
    v[5] = 0;
    v[4] = stamps.first;
    if (stamps.first == ag::stamp_value) { v[3] = stamps.second; if (stamps.valid) v[5] = 1; }
    if (!stamps.valid) { v[3] = 0xffffffff; v.store(c); return; }
    const auto mem = c.u32(handle + ag::mem);
    const std::uint32_t blocks[] = {ag::bytes_3d8, ag::original_xs, ag::next_point, ag::point_flags, ag::curve_xs, ag::partner_before, ag::in_dx, ag::records};
    const std::uint32_t backs[] = {0x107558a8, 0x107558b4, 0x107558c0, 0x107558cc, 0x107558d8, 0x107558e4, 0x107558f0, 0x107558fc, 0x10755908};
    for (unsigned n = 0; n < 9; ++n) {
        const auto pointer = n < 8 ? c.u32(handle + blocks[n]) : handle;
        v[4] = pointer;
        v[3] = mem;
        v.store(c);
        c.set({{30, mem}, {31, handle}});
        c.link(backs[n]);
        release_block(c, mem, pointer);
        v.load(c);
    }
    v[3] = 0;
    v.store(c);
}

// 0x10755930: the size (x and y, the scale words 0x2a4 and 0x2a8) set for a valid handle and the first half's analysis run (0x10751350 with 1 and r6).
// -1 for another handle.
void set_hint_size(Cpu& c, std::uint32_t handle, std::uint32_t size_x, std::uint32_t size_y, std::uint32_t r6) {
    Frame frame(c, 0x10755930, 0x40, {});
    Vol v(c);
    v[8] = 0;
    const auto stamps = check_stamps(c, handle);
    if (handle != 0) v[7] = stamps.first != ag::stamp_value ? stamps.first : stamps.second;
    if (stamps.valid) v[8] = 1;
    if (!stamps.valid) { v[3] = 0xffffffff; v.store(c); return; }
    c.put32(handle + ag::size_x, size_x);
    c.put32(handle + ag::size_y, size_y);
    v[4] = 1;
    v[5] = r6;
    v.store(c);
    c.call(0x10751350, 0x10755990);
}
}
