// The font engine, block `hstrat97` (HSTRAT97.C, 0x1075c980..0x1075e0c0): the hint strategy of the autogridder (0x1075cbc0) and the two small leaves it
// calls (0x1075c980, 0x1075ca60); all three leave r3-r10 as the original does (the strategy's callers pass r9 and r10 on).
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
constexpr std::int32_t extsh(std::uint32_t v) { return std::int16_t(v); }
constexpr std::uint32_t reg(std::int32_t v) { return std::uint32_t(v); }
constexpr std::uint32_t shift_left_masked(std::uint32_t v, unsigned n) { return (v << n) & ~((1u << n) - 1); }  // rlwinm r,r,n,0,31-n
constexpr std::uint32_t sra(std::uint32_t v, unsigned n) { return reg(std::int32_t(v) >> n); }
}

// 0x1075c980 (heap, first, last, keys): the sift-down of a heap sort over an array of record pointers (a record's first halfword indexes `keys`, a
// halfword array): from `first` the larger of the two children is swapped up while it is inside `last`, then the element is carried back up
// (the parent's key not above its own) as far as `first`.
void sift_heap(Cpu& c, std::uint32_t heap, std::uint32_t first, std::uint32_t last, std::uint32_t keys) {
    std::uint32_t r5 = last, r7 = c.r(7), r8 = c.r(8), r9 = c.r(9), r10 = first, r11 = 0;
    const auto key = [&](std::uint32_t record) { return reg(extsh(c.u16(keys + shift_left_masked(reg(extsh(c.u16(record))), 1)))); };
    r9 = r10 + 1;
    r9 = r10 + r9;
    while (std::int32_t(r9) <= std::int32_t(r5)) {
        if (std::int32_t(r9 + 1) <= std::int32_t(r5)) {
            r7 = heap + shift_left_masked(r9, 2);
            r8 = c.u32(r7 + 4);
            r7 = c.u32(r7);
            const auto second = key(r8), first_key = key(r7);
            r8 = reg(extsh(c.u16(r8)));
            r7 = second;
            if (!(std::int32_t(second) <= std::int32_t(first_key))) r9 += 1;
            // r8: the second child's index (a halfword), r7: its key, as the loads leave them.
            (void)first_key;
        }
        r8 = shift_left_masked(r10, 2);
        r7 = shift_left_masked(r9, 2);
        r11 = c.u32(heap + r8);
        r10 = r9;
        c.put32(heap + r8, c.u32(heap + r7));
        c.put32(heap + r7, r11);
        r9 = r10 + 1;
        r9 = r10 + r9;
    }
    for (;;) {
        const auto r0 = r10 - 1;
        r11 = sra(r0, 1);
        if (std::int32_t(r11) < std::int32_t(first)) break;
        if (r11 == r10) break;
        r8 = shift_left_masked(r11, 2);
        r5 = shift_left_masked(r10, 2);
        r10 = c.u32(heap + r8);
        r9 = c.u32(heap + r5);
        r7 = key(r10);
        const auto lower = key(r9);
        if (std::int32_t(r7) > std::int32_t(lower)) break;
        c.put32(heap + r5, r10);
        r10 = r11;
        c.put32(heap + r8, r9);
    }
    c.set({{5, r5}, {7, r7}, {8, r8}, {9, r9}, {10, r10}});
}

// 0x1075ca60 (a, b, flags): whether a cubic-style polynomial of three clamped terms is positive: a (at most 0x10), b (at most 0x12f) and the low
// six bits of the flags are each scaled to a fixed-point term, the sum of the products (a table of coefficients as immediates) is taken, and the small
// cases (a <= 3, b <= 0x56, flags' low bits <= 0xe and below 5a, sum below 0x14d) are made zero; the result is 1 when the sum is positive, else 0
// ((-v & ~v) >> 31). r4-r10 are left as the instructions leave them (r26-r31 are saved below the stack pointer and restored, untouched).
std::uint32_t strategy_score(Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t d) {
    std::uint32_t r3 = a, r4 = b, r5 = d;
    if (std::int32_t(r3) > 0x10) r3 = 0x10;
    std::uint32_t r6 = r3 * 0x162;
    std::uint32_t r11 = r6 - 0x4e9;
    if (std::int32_t(r4) > 0x12f) r4 = 0x12f;
    r6 = r4 * 0x12;
    const std::uint32_t r10 = r5 & 0x3f;
    const std::uint32_t r12 = r6 - 0x55c;
    r5 = r10 * 0x51;
    const std::uint32_t r31 = r5 - 0x4f6;
    std::uint32_t r0 = r11 * r11;
    std::uint32_t r27 = sra(r0, 10);
    r0 = r12 * r12;
    std::uint32_t r26 = sra(r0, 10);
    r0 = r31 * r31;
    const std::uint32_t r28 = sra(r0, 10);
    r0 = r11 * r12;
    const std::uint32_t r29 = sra(r0, 10);
    r0 = r28 * r31;
    r0 = sra(r0, 10);
    r0 = r0 * 0x57;
    r5 = r26 * r12;
    const std::uint32_t r9 = sra(r0, 10);
    r0 = r27 * r11;
    const std::uint32_t r8 = sra(r5, 0x12);
    r0 = sra(r0, 10);
    r5 = r0 * 0x39;
    r0 = r29 * r31;
    const std::uint32_t r7 = sra(r5, 10);
    r0 = sra(r0, 10);
    r0 = r0 * 0x4e;
    r6 = sra(r0, 10);
    r0 = r28 * 0x2ad;
    r5 = sra(r29, 4);
    r26 = r26 * 0x8c;
    r0 = sra(r0, 10);
    r27 = r27 * 0x133;
    r26 = sra(r26, 10);
    r27 = sra(r27, 10);
    std::uint32_t r30 = sra(r11 * 0x16c, 10);
    const std::uint32_t shifted_r12 = sra(r12, 3);
    std::uint32_t r29b = r31 * 0x3ac;
    r30 += 0x237;
    const std::uint32_t r11b = r11 * r31;
    r30 = r30 - shifted_r12;
    r29b = sra(r29b, 10);
    r29b = r30 + r29b;
    std::uint32_t r11c = sra(r11b, 10);
    r29b = r29b - r27;
    r30 = r11c * 0x64;
    r29b = r29b - r26;
    r11c = r12 * r31;
    std::uint32_t r12b = r29b - r0;
    r0 = sra(r30, 10);
    r12b = r12b + r0;
    r0 = sra(r11c, 10);
    r0 = r0 * 0x172;
    r0 = sra(r0, 10);
    r0 = r12b + r0;
    r0 = r5 + r0;
    r0 = r0 - r6;
    r0 = r0 + r8;
    r5 = r0 + r9;
    r5 = r7 + r5;
    if (std::int32_t(r5) > 0 && std::int32_t(r5) < 0x14d && std::int32_t(r3) <= 3 && std::int32_t(r10) <= 0xe && std::int32_t(r4) <= 0x56 &&
        std::int32_t(r3 * 5) > std::int32_t(r10))
        r5 = 0;
    r0 = 0u - r5;
    r0 = r0 & ~r5;
    r3 = r0 >> 31;
    c.set({{3, r3}, {4, r4}, {5, r5}, {6, r6}, {7, r7}, {8, r8}, {9, r9}, {10, r10}});
    return r3;
}

// ---- 0x1075cbc0: the hint strategy ----------------------------------------------------------------------------------------------------------
namespace {
namespace hs {
constexpr std::int32_t pool_slot = -0x3c40;  // TOC slot of HSTRAT97.C's pool: its assertion texts, the file name at +0x16
constexpr std::uint32_t file_name = 0x16;
// The hint handle (`autogrid::` in the detail header) as the strategy reads it.
constexpr std::uint32_t contour_count = 0x04 /* hw */, contour_ends = 0x0c, xs = 0x14, ys = 0x18, mode = 0x1c /* hw */, next_point = 0x20, previous_point = 0x24,
    point_flags = 0x2c, mapping = 0x40 /* hw per point: the group a point's record fell in (-1 none) */, record_count = 0x54, edges = 0x5c, units = 0x2a0 /* hw */,
    flavor = 0x2ac, search_error = 0x438, search_moved = 0x43c, mem = 0x450, keep_flat = 0x454;
constexpr std::uint32_t reset_400 = 0x400, reset_402 = 0x402, reset_404 = 0x404, set_448 = 0x448, reset_44a = 0x44a;  // halfwords the strategy sets first
// The edge records the analysis built (8 bytes each, `autogrid::records`): +0 kind (3: a link of two points), +1 class (1 x, 2 y), +4 and +6 the two points.
constexpr std::uint32_t link_kind = 3;
// The strategy's own record per candidate (14 bytes, from the block its first allocation makes): +0 the point (the first halfword), +2 its partner (or -1), +4 the height code (-1 none,
// 9999), +6 the group, +8 the group of the partner's record, +0xa the record that points at this one, +0xc the state bits below.
constexpr std::uint32_t record_size = 14, partner = 2, code = 4, group = 6, link = 8, back_link = 0xa, state = 0xc;
constexpr std::uint32_t merged = 1, placed = 2, height = 4;
constexpr std::uint32_t corner_flag = 0x40;  // the point flag of a corner, read in the second pass
constexpr std::uint32_t indirect_glue = 0x10781320;
// The TOC slots of the callbacks' descriptors (x pass, y pass): round in a band 0x10753fd0 / 0x10754050, interpolate 0x107540d0 / 0x10754130, round 0x10753f70 / 0x10753fa0.
constexpr std::int32_t round_band_x = -0x5b24, round_band_y = -0x5b28, round_x = -0x5b2c, round_y = -0x5b30, interpolate_x = -0x5b34, interpolate_y = -0x5b38;
}

constexpr std::int32_t s32(std::uint32_t v) { return std::int32_t(v); }
constexpr std::uint32_t scale2(std::uint32_t v) { return (v << 1) & ~1u; }  // rlwinm r,r,1,0,30
constexpr std::uint32_t scale4(std::uint32_t v) { return (v << 2) & ~3u; }  // rlwinm r,r,2,0,29
constexpr std::uint32_t mulhw(std::uint32_t a, std::uint32_t b) { return std::uint32_t((std::int64_t(s32(a)) * s32(b)) >> 32); }
constexpr std::uint32_t ext16(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }

// Outcomes no data reaches (proved, so the cases do not hit them; the transcription keeps the tests): (1) the assertions on the group number of a kept record
// (0x244) and on the order of the kept records (0x24c): the records are heap-sorted by one key and groups are runs of that order, a merge keeps one record a
// group, so the kept records' groups are 0, 1, 2.. and their keys ascend; (2) the assertions on an unplaced neighbour not past the current record (0x2bf, 0x369):
// the records before the current one are placed, the current one is placed before the second assertion and its back link never names itself; (3) in the merge
// the current index is at least 1 and the anchor at most count - 2, and the count is at least 2 (the two end records), so the tests for 0, count - 1 and
// `count <= 1` have only one outcome; the mapping fill's second bound test and the unrolled fill's negative test likewise (8 * (n >> 3) <= n); (4) after a
// moved stem width the loop restarts at a record past the last bisected one, so `last > bisected` always holds there. The assertions on a negative point
// (0x2cb, 0x36b), on a flag a callee cleared (0x2d2, 0x3c2, 0x3c3, 0x3d2, 0x3d3) and the capacity (0x1c3) are reached by the cases (edge records with point -1,
// flags poked away at a callee's dispatch, a record count equal to the capacity).
//
// One call of the strategy. `g` holds the original's non-volatile registers r13-r31 (the aliases name the ones that keep one meaning; the others are the
// loop variables the listing reuses), `v` holds r3-r10 as the original's instructions leave them (the callbacks reached through the glue, the assertion
// reporter and the leaf routines read them), and the stack locals are members. The callees save the registers they are held at, so every call sets them.
class Strategy {
public:
    Strategy(Cpu& cpu, guest::Frame& f, std::uint32_t input_record, std::uint32_t handle) : c(cpu), frame(f), input(g[15]), h(g[16]), records(g[21]), flags(g[22]),
        other(g[23]), coords(g[24]), first_pass(g[25]), bit(g[26]), pool(g[31]) {
        for (unsigned n = 13; n <= 31; ++n) g[n] = c.r(n);
        for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n);
        input = input_record;
        h = handle;
        pool = c.toc(hs::pool_slot);
    }
    void run();

private:
    Cpu& c;
    guest::Frame& frame;
    std::array<std::uint32_t, 32> g{};  // r13-r31
    std::array<std::uint32_t, 11> v{};  // r3-r10
    // r15 the glyph input, r16 the hint handle, r21 the array of record pointers, r22 the point flags, r23 / r24 the other axis and the pass's axis (halfword
    // coordinates), r25 nonzero (as a halfword) in the first pass (x), r26 the pass's "placed" flag bit, r31 the pool.
    std::uint32_t &input, &h, &records, &flags, &other, &coords, &first_pass, &bit, &pool;
    // The stack locals the original keeps at sp + 0x48..0xa0: only the parameter words at sp + 0x38.. (read by callees) are written to the frame.
    std::uint32_t mapping = 0, block = 0, capacity = 0, small_gap = 0, group_gap = 0, score = 0, low_index = 0, high_index = 0, last_plus_2 = 0, last = 0;
    std::uint32_t kind = 0, pass = 0, kept = 0, count = 0, current = 0, saved_a = 0, saved_b = 0, saved_c = 0, edge_bits = 0;
    std::uint32_t loop_index = 0, loop_pointer = 0, loop_row = 0, loop_kind = 0;

    // ---- memory, as the loads and stores of the listing ----
    std::uint32_t lha(std::uint32_t at) const { return std::uint32_t(std::int32_t(c.i16(at))); }
    std::uint32_t lhz(std::uint32_t at) const { return c.u16(at); }
    std::uint32_t lwz(std::uint32_t at) const { return c.u32(at); }
    void sth(std::uint32_t at, std::uint32_t value) { c.put16(at, value); }
    void stw(std::uint32_t at, std::uint32_t value) { c.put32(at, value); }
    // ---- registers and calls ----
    void store() { for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]); }
    void load() { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }
    void hold() { for (unsigned n = 13; n <= 31; ++n) c.set(n, g[n]); }
    void call(std::uint32_t entry, std::uint32_t back) { store(); hold(); c.call(entry, back); load(); }
    // A routine written in C++ and called directly: the registers as the original holds them at the `bl`, and the link register.
    void direct(std::uint32_t back) { store(); hold(); c.link(back); }
    // `bl 0x10781320` with r12 = the descriptor in a TOC slot; the TOC is reloaded from sp + 0x14 afterwards.
    void glue(std::int32_t slot, std::uint32_t back) {
        store();
        hold();
        c.set(12, c.toc(slot));
        c.call(hs::indirect_glue, back);
        c.set(2, c.u32(c.sp() + 0x14));
        load();
    }
    void report(std::uint32_t text, std::uint32_t line, std::uint32_t back) {
        v[3] = pool + text;
        v[4] = pool + hs::file_name;
        v[5] = line;
        call(assertion_report, back);
    }
    bool x_pass() const { return ext16(first_pass) != 0; }  // extsh. r0,r25

    void prologue();
    void begin_pass();
    void collect_candidates();
    void end_records();
    void sort_records();
    void clear_mapping();
    void assign_groups();
    void assign_codes();
    void merge_neighbours();
    void compact();
    void check_order();
    void place_all();
    bool place_current();
    void restart_after_move();
    void finish_pass();
};

// 0x1075cbc0..0x1075ccd0: the sizes the passes work with (the units per em in hundredths and tenths), two blocks (the 14-byte records, the pointer array).
void Strategy::prologue() {
    const auto units = lha(h + hs::units);
    const auto contours = lha(h + hs::contour_count);
    auto q = mulhw(0x51eb851f, units);  // r8
    const auto ends = lwz(h + hs::contour_ends);
    mapping = lwz(h + hs::mapping);
    flags = lwz(h + hs::point_flags);
    q = sra(q, 5);
    last = lha(ends + scale2(contours) - 2);
    q += q >> 31;
    const auto tenth = mulhw(0x66666667, q);  // r7
    sth(h + hs::reset_404, 0);
    sth(h + hs::reset_402, 0);
    sth(h + hs::reset_400, 0);
    sth(h + hs::set_448, 1);
    capacity = scale2(last + 1) + 0x10;
    auto t = sra(tenth, 2);
    t += t >> 31;
    group_gap = t + 1;
    small_gap = q * 3 + 1;
    sth(h + hs::reset_44a, 0);
    last_plus_2 = last + 2;
    score = 0;
    v[3] = lwz(h + hs::mem);
    v[4] = capacity * hs::record_size;
    v[5] = q * 3;
    v[6] = 0;
    v[7] = tenth;
    v[8] = q;
    direct(0x1075cc9c);
    block = allocate_block(c, v[3], v[4]);
    load();
    v[3] = lwz(h + hs::mem);
    v[4] = scale4(capacity);
    direct(0x1075ccb4);
    records = allocate_block(c, v[3], v[4]);
    load();
    v[4] = last;
    count = last + 1;
}

// 0x1075cccc..0x1075cdd8: the pass: the callbacks that reset the bisection, find the widest stems and (first pass, out of the plain mode) round the outline's
// phantom points, then the axis the pass works on and its masks.
void Strategy::begin_pass() {
    g[14] = 0;
    first_pass = 0;
    if (pass == 0) {
        v[3] = h;
        call(0x10753d50, 0x1075cce8);
        if (lha(h + hs::mode) != 0 || lwz(h + hs::flavor) != 1) {
            v[3] = last;
            v[4] = input;
            v[5] = 1;
            const auto r0 = v[3] + 1;
            v[3] = h;
            g[13] = ext16(r0);
            v[6] = g[13];
            call(0x10753f70, 0x1075cd20);
            v[3] = last;
            v[4] = input;
            stw(frame.at(0x38), g[13]);
            v[5] = 0xffffffffu;
            const auto second = ext16(v[3] + 2);
            v[3] = h;
            v[6] = 0;
            stw(frame.at(0x3c), second);
            v[7] = 0;
            v[8] = 1;
            v[9] = 0x47;
            v[10] = 0x72;
            call(0x10753d70, 0x1075cd58);
        }
        v[3] = h;
        first_pass = 1;
        call(0x10754810, 0x1075cd64);
        coords = lwz(h + hs::xs);
        other = lwz(h + hs::ys);
        bit = 0x10;
        kind = 1;
        v[10] = 4;
        edge_bits = 0x5000;
        v[8] = 1;
    } else {
        v[3] = h;
        call(0x10753d60, 0x1075cd90);
        v[3] = h;
        g[14] = 1;
        call(0x107548d0, 0x1075cd9c);
        v[3] = h;
        call(0x10754810, 0x1075cda4);
        coords = lwz(h + hs::ys);
        other = lwz(h + hs::xs);
        bit = 0x20;
        kind = 2;
        v[10] = 8;
        edge_bits = 0x2800;
        v[8] = 2;
    }
    v[3] = 0;
    g[17] = records;
    v[4] = v[3];
    g[13] = 0;
    v[5] = v[3];
    v[7] = 0;
}


// 0x1075cdc4..0x1075cfe0: a candidate record for each link edge of the pass's class (its partner kept only where the pair is far apart on this axis, else
// -1), then one for each point of the pass's flags (a corner kept for the second pass; a point with the extremum bit; a point with the edge bits): the lowest
// and the highest coordinate among the extremum points are noted. g[13] counts the records, v[3] / v[4] are the byte offsets of the next ones.
void Strategy::collect_candidates() {
    for (;; v[3] += 8, v[7] += 1) {
        if (!(s32(v[7]) < s32(lwz(h + hs::record_count)))) break;
        v[6] = lwz(h + hs::edges) + v[3];
        auto r0 = std::uint32_t(c.u8(v[6]));
        if (r0 != hs::link_kind) continue;
        r0 = c.u8(v[6] + 1);
        v[6] = v[8] & 0xff;
        if (v[6] != r0) continue;
        r0 = block;
        v[9] = v[3] + 6;
        g[18] = 0xffffffffu;
        v[6] = r0 + v[4];
        r0 = v[3] + 4;
        stw(g[17], v[6]);
        auto r12 = lwz(h + hs::edges);
        r0 = lha(r12 + r0);
        sth(v[6], r0);
        g[19] = scale2(r0);
        r12 = lwz(h + hs::edges);
        r0 = lha(coords + g[19]);
        v[9] = lha(r12 + v[9]);
        g[19] = lha(other + g[19]);
        g[20] = scale2(v[9]);
        r12 = lha(coords + g[20]);
        g[20] = lha(other + g[20]);
        r12 = r0 - r12;
        r0 = sra(r12, 31);
        g[19] = g[19] - g[20];
        g[20] = sra(g[19], 31);
        g[19] = g[20] ^ g[19];
        r12 = r0 ^ r12;
        g[19] = g[19] - g[20];
        r12 = r12 - r0;
        r0 = sra(g[19], 3);
        if (!(s32(r12) <= s32(r0))) g[18] = v[9];
        sth(v[6] + hs::partner, g[18]);
        r0 = 0;
        v[5] += 4;
        g[17] += 4;
        sth(v[6] + hs::state, r0);
        g[13] += 1;
        v[4] += hs::record_size;
    }
    g[17] = lha(coords);
    const auto mask = v[10] & 0xffff;
    v[5] = 0;
    v[9] = 0;
    low_index = 0;
    v[5] = 0;
    v[10] = count;
    auto counter = v[10];
    v[8] = v[9];
    high_index = 0;
    v[7] = flags;
    auto r12 = g[17];
    v[6] = coords;
    v[3] = scale4(g[13]);
    v[5] = 0;
    v[4] = g[13] * hs::record_size;
    v[10] = last;
    if (s32(v[10]) >= 0) {
        for (; counter != 0; --counter) {
            v[10] = ext16(g[14]);
            if (v[10] != 0) {
                v[10] = lhz(v[7]);
                v[10] &= hs::corner_flag;
                if (v[10] != 0) {
                    v[10] = block;
                    g[18] = 0xffffffffu;
                    g[13] += 1;
                    g[19] = v[10] + v[4];
                    v[10] = 4;
                    stw(records + v[3], g[19]);
                    v[3] += 4;
                    v[4] += hs::record_size;
                    sth(g[19], v[9]);
                    sth(g[19] + hs::partner, g[18]);
                    sth(g[19] + hs::state, v[10]);
                }
            }
            g[18] = lhz(v[7]);
            v[10] = mask & g[18];
            if (v[10] != 0) {
                v[10] = lha(v[6]);
                v[5] += 1;
                if (s32(v[10]) > s32(r12)) {
                    high_index = v[9];
                    r12 = v[10];
                } else if (s32(v[10]) < s32(g[17])) {
                    g[17] = v[10];
                    low_index = v[9];
                }
                v[10] = edge_bits & 0xffff;
                v[10] &= g[18];
                if (v[10] != 0) {
                    v[10] = block;
                    g[18] = 0xffffffffu;
                    g[13] += 1;
                    g[19] = v[10] + v[4];
                    v[10] = 0;
                    stw(records + v[3], g[19]);
                    v[3] += 4;
                    v[4] += hs::record_size;
                    sth(g[19], v[9]);
                    sth(g[19] + hs::partner, g[18]);
                    sth(g[19] + hs::state, v[10]);
                }
            }
            v[8] += 2;
            v[6] += 2;
            v[7] += 2;
            v[9] += 1;
        }
    }
    // In the first pass of a font of 1,000 glyphs or more the score of the shape decides whether the end records are height records.
    if (x_pass() && lwz(h + hs::flavor) == 2) {
        v[3] = lha(input);
        v[4] = last;
        direct(0x1075cfe0);
        score = strategy_score(c, v[3], v[4], v[5]);
        load();
    }
}

// 0x1075cfe4..0x1075d0f4: the records of the lowest and highest extremum points (height records when the score said so), and in the first pass of the plain
// mode two for the phantom points after the outline; the pointer array has to have room.
void Strategy::end_records() {
    auto r0 = score;
    v[6] = g[13] * hs::record_size;
    v[3] = scale4(g[13]);
    v[4] = 0u - r0;
    r0 = block;
    v[5] = g[13] + 1;
    g[13] += 2;
    v[9] = r0 + v[6];
    r0 = 0xffffffffu;
    stw(records + v[3], v[9]);
    v[6] = v[5] * hs::record_size;
    v[3] = score;
    v[5] = scale4(v[5]);
    v[8] = v[4] | v[3];
    v[3] = low_index;
    v[7] = sra(v[8], 31);
    v[4] = 4;
    sth(v[9], v[3]);
    v[3] = sra(v[8], 31);
    v[7] = v[4] & v[7];
    sth(v[9] + hs::partner, r0);
    v[4] = v[4] & v[3];
    v[3] = block;
    sth(v[9] + hs::state, v[7]);
    v[6] = v[3] + v[6];
    v[3] = ext16(first_pass);
    stw(records + v[5], v[6]);
    v[3] = high_index;
    sth(v[6], v[3]);
    sth(v[6] + hs::partner, r0);
    sth(v[6] + hs::state, v[4]);
    if (x_pass()) {
        v[3] = lha(h + hs::mode);
        if (v[3] == 0) {
            v[3] = lwz(h + hs::flavor);
            if (v[3] == 1) {
                v[3] = last;
                v[4] = g[13] * hs::record_size;
                v[6] = scale4(g[13]);
                v[8] = v[3] + 1;
                v[3] = block;
                v[5] = g[13] + 1;
                v[7] = 0;
                v[9] = v[3] + v[4];
                v[3] = last;
                stw(records + v[6], v[9]);
                v[6] = v[5] * hs::record_size;
                v[4] = v[3] + 2;
                v[3] = block;
                sth(v[9], v[8]);
                v[5] = scale4(v[5]);
                v[3] = v[3] + v[6];
                sth(v[9] + hs::partner, r0);
                g[13] += 2;
                sth(v[9] + hs::state, v[7]);
                stw(records + v[5], v[3]);
                sth(v[3], v[4]);
                sth(v[3] + hs::partner, r0);
                sth(v[3] + hs::state, v[7]);
            }
        }
    }
    r0 = capacity;
    if (!(s32(g[13]) < s32(r0))) {
        v[3] = pool;
        v[4] = pool + hs::file_name;
        v[5] = 0x1c3;
        call(assertion_report, 0x1075d0f4);
    }
}

// 0x1075d0f4..0x1075d170: the records sorted by the coordinate of their point (a heap sort over the pointer array through 0x1075c980).
void Strategy::sort_records() {
    v[3] = g[13] - 2;
    auto r0 = v[3] >> 31;
    r0 += v[3];
    g[17] = sra(r0, 1);
    while (s32(g[17]) >= 0) {
        v[3] = records;
        v[4] = g[17];
        v[6] = coords;
        v[5] = g[13] - 1;
        direct(0x1075d11c);
        sift_heap(c, v[3], v[4], v[5], v[6]);
        load();
        g[17] -= 1;
    }
    g[19] = g[13] - 1;
    g[18] = scale4(g[19]);
    g[17] = records + g[18];
    while (s32(g[19]) > 0) {
        v[7] = lwz(g[17]);
        g[19] -= 1;
        r0 = lwz(records);
        v[3] = records;
        v[5] = g[19];
        v[6] = coords;
        stw(g[17], r0);
        v[4] = 0;
        g[18] -= 4;
        g[17] -= 4;
        stw(records, v[7]);
        direct(0x1075d168);
        sift_heap(c, v[3], v[4], v[5], v[6]);
        load();
    }
}

// 0x1075d170..0x1075d224: the per-point map of groups starts all -1 (last + 3 halfwords; the original fills eight at a time, then one by one).
void Strategy::clear_mapping() {
    auto r0 = last_plus_2;
    v[3] = 0;
    if (s32(v[3]) > s32(r0)) return;
    v[4] = r0;
    r0 = v[4] + 1;
    v[5] = v[4] - 8;
    if (s32(r0) > 8) {
        r0 = v[5] + 8;
        v[6] = v[3];
        r0 = r0 >> 3;
        v[4] = 0xffffffffu;
        auto counter = r0;
        if (s32(v[5]) >= 0) {
            for (; counter != 0; --counter) {
                r0 = mapping;
                v[3] += 8;
                v[5] = r0 + v[6];
                v[6] += 16;
                for (unsigned k = 0; k < 8; ++k) sth(v[5] + 2 * k, v[4]);
            }
        }
    }
    v[4] = last_plus_2;
    v[7] = scale2(v[3]);
    r0 = mapping;
    v[6] = 0xffffffffu;
    v[4] += 1;
    v[4] = v[4] - v[3];
    v[5] = r0 + v[7];
    auto counter = v[4];
    r0 = last_plus_2;
    if (s32(v[3]) > s32(r0)) return;
    for (; counter != 0; --counter) {
        sth(v[5], v[6]);
        v[7] += 2;
        v[5] += 2;
        v[3] += 1;
    }
}

// 0x1075d224..0x1075d2bc: the first record's group is 0; each next record starts a new group when its point is more than the gap (a tenth of the
// hundredth of the units) from the one before on this axis; the group is stored in the record and in the point's map entry.
void Strategy::assign_groups() {
    auto r5 = lwz(records);
    v[4] = 0;
    auto r0 = g[13] - 1;
    v[3] = 4;
    sth(r5 + hs::group, v[4]);
    r5 = lwz(records);
    r5 = lha(r5);
    v[6] = scale2(r5);
    v[5] = mapping;
    sth(v[5] + v[6], v[4]);
    auto counter = r0;
    if (s32(g[13]) <= 1) return;
    for (; counter != 0; --counter) {
        v[6] = records + v[3];
        v[5] = lwz(v[6] - 4);
        v[7] = lwz(v[6]);
        v[5] = lha(v[5]);
        r0 = lha(v[7]);
        v[5] = scale2(v[5]);
        r0 = scale2(r0);
        v[5] = lha(coords + v[5]);
        r0 = lha(coords + r0);
        v[8] = r0 - v[5];
        v[5] = sra(v[8], 31);
        r0 = v[5] ^ v[8];
        v[8] = r0 - v[5];
        r0 = group_gap;
        if (!(s32(v[8]) <= s32(r0))) v[4] += 1;
        sth(v[7] + hs::group, v[4]);
        v[3] += 4;
        v[5] = lwz(v[6]);
        r0 = lha(v[5]);
        v[5] = mapping;
        r0 = scale2(r0);
        sth(v[5] + r0, v[4]);
    }
}

// 0x1075d2bc..0x1075d354: each record's link is the group of its partner's point, and (for a record flagged as a height record) its code: 9999 when the
// score said the shape is a plain one, else the code of the nearest height record (0x10751bd0); the others get -1.
void Strategy::assign_codes() {
    g[18] = 0;
    g[19] = records;
    g[17] = g[18];
    for (; s32(g[18]) < s32(g[13]); g[17] += 4, g[19] += 4, g[18] += 1) {
        v[4] = lwz(g[19]);
        v[3] = lha(v[4] + hs::partner);
        auto r0 = v[3];
        if (s32(v[3]) < 0) {
            r0 = 0xffffffffu;
        } else {
            r0 = scale2(r0);
            v[3] = mapping;
            r0 = lha(v[3] + r0);
        }
        sth(v[4] + hs::link, r0);
        v[3] = lwz(g[19]);
        r0 = lhz(v[3] + hs::state);
        if ((r0 & hs::height) != 0) {
            r0 = score;
            if (r0 == 0) {
                v[4] = lha(v[3]);
                v[3] = h;
                call(0x10751bd0, 0x1075d328);
                r0 = ext16(v[3]);
            } else {
                r0 = 0x270f;
            }
            v[3] = lwz(g[19]);
            sth(v[3] + hs::code, r0);
        } else {
            r0 = 0xffffffffu;
            sth(v[3] + hs::code, r0);
        }
    }
}

// 0x1075d354..0x1075d4ec: neighbouring records of one group are merged: the pair's points' coordinates on the other axis decide which of the two stays
// (the one nearer an end of the array when the points are level on this axis), the survivor takes the other's state bits, code and link, and the other is
// marked merged (bit 1).
void Strategy::merge_neighbours() {
    auto r0 = g[13] - 1;
    v[7] = 0;
    v[6] = 1;
    v[3] = 4;
    auto counter = r0;
    if (s32(g[13]) <= 1) return;
    for (; counter != 0; --counter, v[3] += 4, v[6] += 1) {
        r0 = scale4(v[7]);
        v[5] = lwz(records + v[3]);
        v[4] = lwz(records + r0);
        v[8] = lha(v[5] + hs::group);
        v[9] = lha(v[4] + hs::group);
        if (v[9] != v[8]) {
            v[7] = v[6];
            continue;
        }
        v[9] = lha(v[5]);
        v[8] = lha(v[4] + hs::partner);
        v[9] = scale2(v[9]);
        const auto r11 = scale2(v[8]);
        const auto r12 = lha(other + v[9]);
        v[8] = lha(other + r11);
        v[9] = lha(coords + v[9]);
        v[10] = r12 ^ v[8];
        const auto shifted = sra(v[10], 1);
        v[8] = lha(coords + r11);
        v[10] = v[10] & r12;
        const bool level = v[9] == v[8];
        v[8] = shifted - v[10];
        v[9] = v[8] >> 31;
        if (!level) {
            if (v[9] != 0) {
                if (v[6] == 0) {
                    v[9] = 0;
                } else {
                    v[8] = g[13] - 1;
                    if (v[6] == v[8]) v[9] = 0;
                }
            } else {
                if (v[7] == 0) {
                    v[9] = 1;
                } else {
                    v[8] = g[13] - 1;
                    if (v[7] == v[8]) v[9] = 1;
                }
            }
        }
        if (v[9] != 0) {
            v[8] = lhz(v[4] + hs::state);
            v[5] = lhz(v[5] + hs::state);
            v[5] = v[8] | v[5];
            sth(v[4] + hs::state, v[5]);
            v[5] = lwz(records + r0);
            v[4] = lha(v[5] + hs::code);
            if (s32(v[4]) < 0) {
                v[4] = lwz(records + v[3]);
                v[4] = lha(v[4] + hs::code);
                sth(v[5] + hs::code, v[4]);
            }
            v[4] = lwz(records + v[3]);
            v[5] = lha(v[4] + hs::link);
            v[4] = v[5];
            if (s32(v[4]) >= 0) {
                v[4] = lwz(records + r0);
                r0 = lha(v[4] + hs::link);
                if (s32(r0) < 0) sth(v[4] + hs::link, v[5]);
            }
            v[4] = lwz(records + v[3]);
            r0 = lhz(v[4] + hs::state);
            r0 |= hs::merged;
            sth(v[4] + hs::state, r0);
        } else {
            v[7] = lhz(v[5] + hs::state);
            v[4] = lhz(v[4] + hs::state);
            v[4] = v[7] | v[4];
            sth(v[5] + hs::state, v[4]);
            v[5] = lwz(records + v[3]);
            v[4] = lha(v[5] + hs::code);
            if (s32(v[4]) < 0) {
                v[4] = lwz(records + r0);
                v[4] = lha(v[4] + hs::code);
                sth(v[5] + hs::code, v[4]);
            }
            v[4] = lwz(records + r0);
            v[7] = lha(v[4] + hs::link);
            v[4] = v[7];
            if (s32(v[4]) >= 0) {
                v[5] = lwz(records + v[3]);
                v[4] = lha(v[5] + hs::link);
                if (s32(v[4]) < 0) sth(v[5] + hs::link, v[7]);
            }
            v[4] = lwz(records + r0);
            v[7] = v[6];
            r0 = lhz(v[4] + hs::state);
            r0 |= hs::merged;
            sth(v[4] + hs::state, r0);
        }
    }
}

// 0x1075d4ec..0x1075d5dc: the records not merged away are kept in front of the array (their count in `kept`) with the back link cleared; a merged one marks
// its point's flag with the pass's kind bit. Then each kept record's group is its position (an assertion otherwise), and the record its link names learns its
// position.
void Strategy::compact() {
    auto r0 = 0u;
    v[5] = records;
    kept = r0;
    v[4] = scale4(r0);
    v[8] = 0;
    v[7] = 0xffffffffu;
    auto counter = g[13];
    if (s32(g[13]) > 0) {
        for (; counter != 0; --counter) {
            v[3] = lwz(v[5]);
            r0 = lhz(v[3] + hs::state);
            if ((r0 & hs::merged) == 0) {
                sth(v[3] + hs::back_link, v[7]);
                v[3] = kept;
                r0 = lwz(v[5]);
                v[3] += 1;
                stw(records + v[4], r0);
                v[4] += 4;
                kept = v[3];
            } else {
                r0 = lha(v[3]);
                v[6] = scale2(r0);
                r0 = kind;
                v[3] = lhz(flags + v[6]);
                r0 = v[3] | r0;
                sth(flags + v[6], r0);
            }
            v[8] += 4;
            v[5] += 4;
        }
    }
    g[17] = 0;
    g[18] = records;
    g[13] = g[17];
    for (; s32(g[17]) < s32(kept); g[13] += 4, g[18] += 4, g[17] += 1) {
        v[3] = lwz(g[18]);
        r0 = lha(v[3] + hs::group);
        if (g[17] != r0) report(0x21, 0x244, 0x1075d59c);
        v[3] = lwz(g[18]);
        v[3] = lha(v[3] + hs::link);
        r0 = v[3];
        if (s32(v[3]) >= 0) {
            r0 = scale4(r0);
            v[3] = lwz(records + r0);
            r0 = lha(v[3] + hs::link);
            if (g[17] != r0) sth(v[3] + hs::back_link, g[17]);
        }
    }
}

// 0x1075d5dc..0x1075d640: the kept records stand in order of their points' coordinates (an assertion otherwise).
void Strategy::check_order() {
    g[17] = 1;
    g[13] = 4;
    for (; s32(g[17]) < s32(kept); g[13] += 4, g[17] += 1) {
        v[3] = records + g[13];
        v[4] = lwz(v[3] - 4);
        v[3] = lwz(v[3]);
        v[4] = lha(v[4]);
        auto r0 = lha(v[3]);
        v[3] = scale2(v[4]);
        r0 = scale2(r0);
        v[3] = lha(coords + v[3]);
        r0 = lha(coords + r0);
        if (!(s32(v[3]) <= s32(r0))) report(0x36, 0x24c, 0x1075d624);
    }
}

// 0x1075d640..0x1075d6e4: the first record is placed where it is (the round callback of the pass's axis, in a band when it has a height code), its point
// marked, and the bisection of the stem width reset.
void Strategy::place_all() {
    v[3] = lwz(records);
    g[18] = 0;
    g[20] = 0;
    v[7] = lha(v[3] + hs::code);
    g[13] = lha(v[3]);
    v[3] = h;
    v[4] = input;
    v[6] = g[13];
    v[5] = 1;
    if (s32(v[7]) >= 0) glue(x_pass() ? hs::round_band_x : hs::round_band_y, 0x1075d684);
    else glue(x_pass() ? hs::round_x : hs::round_y, 0x1075d6b4);
    v[4] = scale2(g[13]);
    v[3] = h;
    auto r0 = lhz(flags + v[4]);
    r0 |= bit;
    sth(flags + v[4], r0);
    v[4] = lwz(records);
    r0 = lhz(v[4] + hs::state);
    r0 |= hs::placed;
    sth(v[4] + hs::state, r0);
    call(0x10753a80, 0x1075d6e0);
    g[28] = 0;
    // The records in turn: g[28] the record, g[18] the last one whose stem was bisected, g[20] the last record a callback placed past it. A bisection that
    // moved the stem width starts the whole sequence of records after g[18] over (restart_after_move).
    bool jumped = false;
    for (;;) {
        if (!jumped && !(s32(g[28]) < s32(kept))) break;
        jumped = false;
        if (place_current()) {
            restart_after_move();
            jumped = true;
            continue;
        }
        g[28] += 1;
    }
}

// 0x1075d6ec..0x1075d7e4: after a move of the stem width, the records after g[18] lose their placed bit and the loop goes on at the last record.
void Strategy::restart_after_move() {
    v[3] = kept;
    g[28] = v[3] - 1;
    v[5] = scale4(g[28]);
    v[4] = g[28] - g[18];
    if (!(s32(g[28]) > s32(g[18]))) return;
    v[3] = v[4] >> 3;
    const auto r0 = 0u - v[4];
    auto counter = v[3];
    bool tail = true;
    if (v[3] != 0) {
        for (; counter != 0; --counter) {
            for (unsigned k = 0; k < 8; ++k) {
                v[6] = lwz(records + v[5]);
                v[5] -= 4;
                v[3] = lhz(v[6] + hs::state);
                v[3] &= 0xfffffffdu;
                sth(v[6] + hs::state, v[3]);
            }
        }
        v[4] &= 7;
        if (v[4] == 0) tail = false;
    }
    if (tail) {
        counter = v[4];
        for (; counter != 0; --counter) {
            v[6] = lwz(records + v[5]);
            v[5] -= 4;
            v[3] = lhz(v[6] + hs::state);
            v[3] &= 0xfffffffdu;
            sth(v[6] + hs::state, v[3]);
        }
    }
    g[28] += r0;
}

// 0x1075d7e4..0x1075ddfc: one record of the placing loop (g[28]). Its neighbours (the partner's record by link, the record that points at it) are looked up:
// g[13] the highest placed one, g[19] the lowest unplaced one past it. An unplaced record is placed between the placed ones (a callback with the neighbours'
// points), the pending interval's stems bisected (0x10753ab0: a moved width returns true: the caller starts over), the unplaced ones in between
// interpolated, and a record that had a lower unplaced neighbour is placed next with the interval's end. Returns whether the stem width moved.
bool Strategy::place_current() {
    auto r0 = scale4(g[28]);
    g[13] = 0xffffffffu;
    g[30] = records + r0;
    g[19] = 0xffffffffu;
    v[3] = lwz(g[30]);
    v[4] = lha(v[3] + hs::link);
    r0 = lha(v[3]);
    g[17] = lha(v[3] + hs::back_link);
    current = r0 & 0xffff;
    if (s32(v[4]) >= 0) {
        r0 = scale4(v[4]);
        v[3] = lwz(records + r0);
        r0 = lhz(v[3] + hs::state);
        if ((r0 & hs::placed) != 0) g[13] = v[4];
        else g[19] = v[4];
    }
    if (s32(g[17]) >= 0) {
        r0 = scale4(g[17]);
        v[3] = lwz(records + r0);
        r0 = lhz(v[3] + hs::state);
        if ((r0 & hs::placed) != 0) {
            if (s32(g[13]) < 0) g[13] = g[17];
            r0 = g[17];
            if (!(s32(g[13]) <= s32(g[17]))) r0 = g[13];
            g[13] = r0;
        } else {
            if (!(s32(g[17]) > s32(g[28]))) report(0x69, 0x2bf, 0x1075d888);
            if (s32(g[19]) < 0) g[19] = g[17];
            r0 = g[17];
            if (!(s32(g[19]) >= s32(g[17]))) r0 = g[19];
            g[19] = r0;
        }
    }
    v[3] = lwz(g[30]);
    r0 = lhz(v[3] + hs::state);
    g[17] = lha(v[3]);
    if ((r0 & hs::placed) == 0) {
        if (s32(g[13]) >= 0) {
            // A placed neighbour: this record's point is placed against that one's with the stem-edge callback 0x107536b0.
            r0 = scale4(g[13]);
            v[3] = lwz(records + r0);
            g[13] = lha(v[3]);
            if (s32(ext16(g[13])) < 0) report(0x73, 0x2cb, 0x1075d8e8);
            r0 = 0x6c;
            v[3] = h;
            stw(frame.at(0x38), r0);
            v[4] = input;
            v[5] = coords;
            v[6] = first_pass;
            stw(frame.at(0x3c), g[13]);
            v[7] = g[14];
            v[8] = 0x40;
            v[9] = 1;
            stw(frame.at(0x40), g[17]);
            v[10] = 0x42;
            call(0x107536b0, 0x1075d91c);
        } else {
            // No placed neighbour: the point goes after the previous record, or (when the previous one is a neighbour of a close pair further back) after that.
            v[5] = lwz(g[30] - 4);
            v[4] = bit & 0xffff;
            v[3] = lwz(h + hs::point_flags);
            g[27] = lha(v[5]);
            g[29] = scale2(g[27]);
            r0 = lhz(v[3] + g[29]);
            if ((v[4] & r0) == 0) report(0x7d, 0x2d2, 0x1075d950);
            if (s32(g[28]) > 1) {
                v[3] = lwz(g[30] - 8);
                v[6] = scale2(g[17]);
                v[5] = lha(coords + g[29]);
                v[4] = lha(v[3]);
                r0 = lha(coords + v[6]);
                v[7] = scale2(v[4]);
                v[3] = lha(other + g[29]);
                r0 = r0 - v[5];
                v[8] = lha(other + v[6]);
                v[6] = sra(r0, 31);
                v[5] = lha(other + v[7]);
                v[3] = v[8] - v[3];
                r0 = v[6] ^ r0;
                v[8] = v[8] - v[5];
                v[7] = sra(v[3], 31);
                v[5] = small_gap;
                r0 = r0 - v[6];
                v[6] = sra(v[8], 31);
                const bool far = s32(r0) > s32(v[5]);
                v[3] = v[7] ^ v[3];
                v[5] = v[6] ^ v[8];
                v[3] = v[3] - v[7];
                v[8] = v[5] - v[6];
                if (far) {
                    r0 = r0 + r0;
                    if (s32(v[3]) > s32(r0)) {
                        r0 = sra(v[3], 3);  // srawi + addze of an absolute difference of halfwords: never negative, so the carry is 0
                        if (s32(v[8]) < s32(r0)) g[27] = v[4];
                    }
                }
            }
            r0 = scale4(g[18]);
            v[3] = h;
            v[7] = lwz(records + r0);
            v[4] = input;
            v[5] = first_pass;
            v[6] = g[14];
            v[7] = lha(v[7]);
            v[8] = g[27];
            v[9] = g[17];
            call(0x107537c0, 0x1075da00);
        }
        v[6] = lwz(g[30] - 4);
        v[3] = h;
        v[4] = input;
        v[5] = first_pass;
        v[6] = lha(v[6]);
        v[7] = g[17];
        call(0x10753bf0, 0x1075da1c);
        if (s32(g[20]) > s32(g[28])) {
            r0 = scale4(g[20]);
            v[3] = h;
            v[6] = lwz(records + r0);
            v[4] = input;
            v[5] = first_pass;
            v[7] = g[17];
            v[6] = lha(v[6]);
            call(0x10753c30, 0x1075da44);
        }
        v[3] = scale2(g[17]);
        r0 = lhz(flags + v[3]);
        r0 |= bit;
        sth(flags + v[3], r0);
        v[3] = lwz(g[30]);
        r0 = lhz(v[3] + hs::state);
        r0 |= hs::placed;
        sth(v[3] + hs::state, r0);
    }
    // da64: the record has a height code and is not the one the interval started at: bisect the stem width for it.
    v[3] = lwz(g[30]);
    v[6] = lha(v[3] + hs::code);
    if (s32(v[6]) >= 0 && g[28] != g[18]) {
        v[7] = ext16(current);
        v[3] = h;
        v[4] = input;
        v[5] = first_pass;
        call(0x10753ab0, 0x1075da90);
        if (lwz(h + hs::search_moved) != 0) {
            g[20] = g[18];
            return true;
        }
        if (lwz(h + hs::search_error) != 0) {
            // The error is non-zero: every record between the interval's start and this one is interpolated.
            v[3] = lwz(g[30]);
            r0 = scale4(g[18]);
            v[4] = lwz(records + r0);
            g[27] = g[18] + 1;
            r0 = lha(v[3]);
            g[29] = scale4(g[27]);
            g[18] = records + g[29];
            saved_a = r0;
            r0 = lha(v[4]);
            saved_b = r0;
            for (; s32(g[27]) < s32(g[28]); g[29] += 4, g[18] += 4, g[27] += 1) {
                v[5] = lwz(g[18]);
                v[7] = saved_a;
                v[3] = h;
                v[6] = lha(v[5]);
                v[4] = input;
                v[5] = saved_b;
                g[13] = v[6];
                glue(x_pass() ? hs::interpolate_x : hs::interpolate_y, 0x1075db10);
                if (lwz(h + hs::keep_flat) == 0) {
                    v[3] = h;
                    v[4] = input;
                    v[6] = g[13];
                    v[5] = 1;
                    glue(x_pass() ? hs::round_x : hs::round_y, 0x1075db48);
                }
            }
        }
        g[18] = g[28];
    }
    // db64: an unplaced lower neighbour (g[19]): placed now with the stem-edge callback, the records between withdrawn or interpolated.
    if (s32(g[19]) >= 0) {
        g[13] = scale4(g[19]);
        v[3] = lwz(records + g[13]);
        r0 = lhz(v[3] + hs::state);
        if ((r0 & hs::placed) == 0) {
            if (!(s32(g[19]) > s32(g[28]))) report(0x9a, 0x369, 0x1075db98);
            v[3] = lwz(records + g[13]);
            g[27] = lha(v[3]);
            if (s32(ext16(g[17])) < 0) report(0x73, 0x36b, 0x1075dbb8);
            v[3] = 0x6c;
            r0 = ext16(g[17]);
            stw(frame.at(0x38), v[3]);
            v[3] = h;
            v[4] = input;
            v[5] = coords;
            stw(frame.at(0x3c), r0);
            v[6] = first_pass;
            v[7] = g[14];
            v[8] = 0x40;
            stw(frame.at(0x40), g[27]);
            v[9] = 1;
            v[10] = 0x42;
            call(0x107536b0, 0x1075dbf0);
            v[4] = lwz(records + g[13]);
            v[3] = scale2(g[27]);
            r0 = lhz(v[4] + hs::state);
            r0 |= hs::placed;
            sth(v[4] + hs::state, r0);
            r0 = lhz(flags + v[3]);
            r0 |= bit;
            sth(flags + v[3], r0);
            if (s32(g[19]) > s32(g[20])) {
                r0 = g[19] - g[18];
                g[20] = g[19];
                v[8] = g[19];
                v[5] = g[13];
                v[3] = records + g[13];
                auto counter = r0;
                if (s32(g[19]) > s32(g[18])) {
                    for (; counter != 0; --counter) {
                        v[4] = lwz(v[3]);
                        r0 = lha(v[4] + hs::code);
                        if (s32(r0) >= 0) break;
                        v[5] -= 4;
                        v[3] -= 4;
                        v[8] -= 1;
                    }
                }
                v[8] -= 1;
                v[6] = 0xffffffffu;
                v[7] = scale4(v[8]);
                v[3] = v[8] - g[18];
                v[4] = records + v[7];
                if (s32(v[8]) > s32(g[18])) {
                    r0 = v[3] >> 3;
                    counter = r0;
                    bool tail = true;
                    if (r0 != 0) {
                        for (; counter != 0; --counter) {
                            for (unsigned k = 0; k < 8; ++k) {
                                v[5] = lwz(v[4] - 4 * k);
                                if (k == 0) v[7] -= 0x20;
                                if (k == 7) v[4] -= 0x20;
                                sth(v[5] + hs::code, v[6]);
                            }
                        }
                        v[3] &= 7;
                        if (v[3] == 0) tail = false;
                    }
                    if (tail) {
                        counter = v[3];
                        for (; counter != 0; --counter) {
                            v[5] = lwz(v[4]);
                            v[7] -= 4;
                            v[4] -= 4;
                            sth(v[5] + hs::code, v[6]);
                        }
                    }
                }
            } else {
                r0 = scale4(g[20]);
                v[3] = h;
                v[6] = lwz(records + r0);
                v[4] = input;
                v[5] = first_pass;
                v[7] = g[27];
                v[6] = lha(v[6]);
                v[8] = g[17];
                call(0x10753c70, 0x1075dd18);
            }
            v[4] = lwz(g[30]);
            g[29] = g[28] + 1;
            v[3] = lwz(records + g[13]);
            g[30] = scale4(g[29]);
            r0 = lha(v[4]);
            g[17] = records + g[30];
            g[13] = lha(v[3]);
            saved_c = r0;
            for (; s32(g[29]) < s32(g[19]); g[30] += 4, g[17] += 4, g[29] += 1) {
                v[3] = lwz(g[17]);
                r0 = lhz(v[3] + hs::state);
                g[27] = lha(v[3]);
                if ((r0 & hs::placed) != 0) continue;
                r0 = lha(v[3] + hs::link);
                if (s32(r0) >= 0) continue;
                r0 = lha(v[3] + hs::back_link);
                if (s32(r0) >= 0) continue;
                v[5] = saved_c;
                v[3] = h;
                v[4] = input;
                v[6] = g[27];
                v[7] = g[13];
                glue(x_pass() ? hs::interpolate_x : hs::interpolate_y, 0x1075dd98);
                v[3] = h;
                v[4] = input;
                v[6] = g[27];
                v[5] = 1;
                glue(x_pass() ? hs::round_x : hs::round_y, 0x1075ddc4);
                v[4] = lwz(g[17]);
                v[3] = scale2(g[27]);
                r0 = lhz(v[4] + hs::state);
                r0 |= hs::placed;
                sth(v[4] + hs::state, r0);
                r0 = lhz(flags + v[3]);
                r0 |= bit;
                sth(flags + v[3], r0);
            }
        }
    }
    return false;
}

// 0x1075de0c..0x1075e05c: after the loop the first pass (plain mode) places the end points (0x10753980), and every kept record's point is checked
// against the points between each pair of neighbours: a point of this kind not yet placed and not at either end's coordinate is interpolated between them.
void Strategy::finish_pass() {
    if (x_pass() && lha(h + hs::mode) == 0 && lwz(h + hs::flavor) == 1) {
        v[5] = last;
        v[3] = h;
        v[8] = last;
        v[4] = input;
        v[6] = low_index;
        v[5] = v[5] + 1;
        v[7] = high_index;
        v[8] += 2;
        call(0x10753980, 0x1075de50);
    }
    auto r0 = kind;
    g[18] = bit & 0xffff;
    v[3] = lwz(records);
    loop_kind = r0;
    r0 = records + 4;
    g[19] = lha(v[3]);
    loop_pointer = r0;
    loop_index = 1;
    for (; s32(loop_index) < s32(kept); loop_pointer += 4, loop_index += 1) {
        g[29] = g[19];
        v[3] = loop_pointer;
        r0 = ext16(g[29]);
        r0 = scale2(r0);
        v[3] = lwz(v[3]);
        loop_row = r0;
        r0 = lhz(flags + r0);
        g[19] = lha(v[3]);
        if ((g[18] & r0) == 0) report(0xa5, 0x3c2, 0x1075deb8);
        g[30] = scale2(g[19]);
        r0 = lhz(flags + g[30]);
        if ((g[18] & r0) == 0) report(0xbc, 0x3c3, 0x1075ded8);
        g[20] = 0;
        g[14] = coords;
        g[17] = g[20];
        g[13] = flags;
        for (; s32(g[20]) <= s32(last); g[17] += 2, g[13] += 2, g[14] += 2, g[20] += 1) {
            v[3] = lhz(g[13]);
            r0 = loop_kind;
            g[23] = lha(g[14]);
            if ((r0 & v[3]) == 0) continue;
            if ((g[18] & v[3]) != 0) continue;
            r0 = loop_row;
            r0 = lha(coords + r0);
            if (s32(g[23]) < s32(r0)) continue;
            r0 = lha(coords + g[30]);
            if (s32(g[23]) > s32(r0)) continue;
            v[5] = lwz(h + hs::previous_point);
            v[3] = 0;
            v[4] = lwz(h + hs::next_point);
            g[27] = lha(v[5] + g[17]);
            g[28] = lha(v[4] + g[17]);
            if (s32(ext16(g[27])) >= 0 && s32(ext16(g[28])) >= 0) v[3] = 1;
            if (v[3] == 0) report(0xd3, 0x3cb, 0x1075df64);
            v[3] = scale2(g[27]);
            r0 = lhz(flags + v[3]);
            if ((g[18] & r0) != 0) {
                r0 = lha(coords + v[3]);
                if (g[23] == r0) continue;
            }
            v[3] = scale2(g[28]);
            r0 = lhz(flags + v[3]);
            if ((g[18] & r0) != 0) {
                r0 = lha(coords + v[3]);
                if (g[23] == r0) continue;
            }
            g[23] = ext16(g[29]);
            if (g[20] == g[23]) report(0xea, 0x3d2, 0x1075dfb8);
            if (g[20] == g[19]) report(0xf3, 0x3d3, 0x1075dfd0);
            v[6] = ext16(g[20]);
            v[3] = h;
            v[4] = input;
            v[5] = g[23];
            v[7] = g[19];
            glue(x_pass() ? hs::interpolate_x : hs::interpolate_y, 0x1075dffc);
            r0 = lhz(g[13]);
            r0 |= bit;
            sth(g[13], r0);
        }
    }
}

void Strategy::run() {
    prologue();
    for (pass = 0; pass < 2;) {
        begin_pass();
        collect_candidates();
        end_records();
        sort_records();
        clear_mapping();
        assign_groups();
        assign_codes();
        merge_neighbours();
        compact();
        check_order();
        if (s32(kept) > 0) {
            place_all();
            finish_pass();
        }
        pass += 1;
        v[3] = pass;
    }
    v[3] = h;
    v[4] = input;
    call(0x10754190, 0x1075e080);
    v[3] = h;
    v[4] = input;
    call(0x10754370, 0x1075e08c);
    v[3] = lwz(h + hs::mem);
    v[4] = block;
    direct(0x1075e098);
    release_block(c, v[3], v[4]);
    load();
    v[3] = lwz(h + hs::mem);
    v[4] = records;
    direct(0x1075e0a4);
    release_block(c, v[3], v[4]);
}
}

// 0x1075cbc0 (input, handle): the hint strategy of the autogridder (HSTRAT97.C). For each axis (x, then y) the points that carry edges or extrema are
// gathered into records, ordered by their coordinate, grouped, merged and kept; then the stems are placed record by record through the autogridder's callbacks
// (round, in a band, interpolate, the stem-edge placing and the bisection of the stem width), the points between them interpolated, and last the points
// left untouched on both axes (0x10754190, 0x10754370). The two blocks it allocates (the records and the array of their pointers) are released at the end.
// Returns 0. Frame 0x100 saving r13-r31 (the release leaver is called at its stack pointer with r28-r31 as the loops left them).
void hint_strategy(Cpu& c, std::uint32_t input, std::uint32_t handle) {
    guest::Frame frame(c, 0x1075cbc0, 0x100, {.first_gpr = 13});
    Strategy strategy(c, frame, input, handle);
    strategy.run();
    c.set(3, 0);
}
}
