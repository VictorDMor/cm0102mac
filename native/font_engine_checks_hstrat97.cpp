// The font engine, block `hstrat97` (HSTRAT97.C, 0x1075c980..0x1075e0c0): cases for the two leaves of the hint strategy. The heap sift runs on heaps of record
// pointers built in the scratch area (records of one halfword, an index into a key array: ascending, descending, equal keys, negative and extreme keys, every
// first/last pair around the array, random heaps); the polynomial on every combination of the clamp edges of its three arguments and random values.
// Labels start with "hstrat97:" (CM_FONT_ENGINE_BLOCK=hstrat97).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t sift = 0x1075c980, score = 0x1075ca60;
constexpr std::uint32_t area = scratch_base + 0xa0000, heap_at = area, record_at = area + 0x400, keys_at = area + 0x800;
}

namespace {
// ---- 0x1075cbc0: the strategy ----
// The hint handle (the autogridder's object) and the glyph input it works on are laid out in the scratch area from a seed: the handle's fields the strategy reads
// (contour ends, the two coordinate axes, the per-point flags, the neighbour arrays, the edge records, the units, flavor and mode), the rest of the object for the
// callbacks that run for real, and the memory object of the snapshot (the two blocks the strategy allocates and releases). The callbacks are answered (the
// glue's six targets and the direct ones) in most cases, so the strategy's own decisions are what is compared; the others run the bisection for real (the only
// callee that moves the stem width) or every callee for real on a well-formed handle.
namespace strat {
constexpr std::uint32_t entry = 0x1075cbc0, reporter = 0x107755c0, game_toc = 0x108bba90, game_malloc = 0x10775130, game_free = 0x10775170;
constexpr std::uint32_t area = scratch_base + 0x30000, handle_at = area, input_at = area + 0x600, arrays = area + 0x1000, stride = 0x400;
constexpr std::uint32_t ends_at = arrays, xs_at = arrays + stride, ys_at = arrays + 2 * stride, next_at = arrays + 3 * stride, prev_at = arrays + 4 * stride,
    flags_at = arrays + 5 * stride, mapping_at = arrays + 6 * stride, edges_at = arrays + 7 * stride, starts_at = arrays + 9 * stride, on_at = arrays + 10 * stride,
    x_words_at = arrays + 11 * stride, y_words_at = arrays + 12 * stride, ox_at = arrays + 13 * stride, oy_at = arrays + 14 * stride, extra_at = arrays + 15 * stride;
// Handle fields.
constexpr std::uint32_t h_stamp = 0, h_count = 4, h_starts = 8, h_ends = 0xc, h_on = 0x10, h_xs = 0x14, h_ys = 0x18, h_mode = 0x1c, h_next = 0x20, h_prev = 0x24, h_extrema = 0x28,
    h_flags = 0x2c, h_before = 0x38, h_after = 0x3c, h_mapping = 0x40, h_in_dx = 0x44, h_in_dy = 0x48, h_out_dx = 0x4c, h_out_dy = 0x50, h_edge_count = 0x54, h_edge_capacity = 0x58,
    h_edges = 0x5c, h_scaled = 0x60, h_units = 0x2a0, h_size_x = 0x2a4, h_size_y = 0x2a8, h_flavor = 0x2ac, h_unordered = 0x34c, h_capacity = 0x350, h_heights = 0x354,
    h_x_widths = 0x390, h_y_widths = 0x3a8, h_original_xs = 0x3d0, h_original_ys = 0x3d4, h_high = 0x428, h_low = 0x42c, h_mid = 0x430, h_steps = 0x434, h_error = 0x438,
    h_moved = 0x43c, h_ready = 0x44c, h_data_set = 0x44d, h_mem = 0x450, h_keep_flat = 0x454, h_end_stamp = 0x458;
constexpr std::uint32_t stamp_a = 0xa5a0f5a5, stamp_b = 0x0fa55af0;

struct Rng {
    std::mt19937 g;
    explicit Rng(std::uint32_t seed) : g(seed) {}
    std::uint32_t any() { return g(); }
    int range(int lo, int hi) { return lo + int(g() % std::uint32_t(hi - lo + 1)); }
    bool chance(unsigned percent) { return g() % 100 < percent; }
};
// What a case varies. A zero field takes a random value.
struct Params {
    int points = 0;              // 1.. (the last index is points - 1); -1: the ends array says -1 (no points)
    int units = 0;               // the handle's units per em
    int flavor = 0;              // 1 or 2 (0: random)
    int mode = -1;               // the halfword at +0x1c (-1: random)
    int keep_flat = -1;
    int error = -1;              // the bisection's last error word (nonzero: the points between are interpolated)
    int contours = -1;           // the hint input's first halfword and the handle's contour count
    int edges = -1;              // the number of edge records (-1: random)
    int link_percent = 75;       // how many of them are links (kind 3)
    int flag_percent = 40;       // the chance a flag bit is set
    int spread = 6;              // coordinates are multiples of 40 below spread * 40, plus a little
    unsigned exact_records = 0;  // make the candidate count equal this (the capacity assertion): pad the edge records
    int bad_percent = 0;         // the chance an edge record's point is -1
    bool tight = false;          // two clusters on each axis: points of one cluster share the other coordinate (pairs that fall into one group)
    bool empty = false;          // no points at all (the last index is -1)
    bool real_mem = true;
    bool moved_start = false;
    std::uint32_t mem = 0;
};
struct Fixture {
    std::vector<Write> memory;
    int last = 0;
    unsigned points = 0;
};
Fixture make(Rng& r, const Params& p) {
    Fixture f;
    auto& m = f.memory;
    const auto put16 = [&](std::uint32_t at, std::uint32_t v) { m.push_back(w16(at, v)); };
    const auto put32 = [&](std::uint32_t at, std::uint32_t v) { m.push_back(w32(at, v)); };
    const int n = p.empty ? 0 : p.points != 0 ? p.points : r.range(1, 16);
    const int last = n - 1;
    f.last = last;
    f.points = unsigned(std::max(n, 0));
    const unsigned total = unsigned(std::max(n, 0)) + 4;
    const int contours = p.contours >= 0 ? p.contours : r.range(0, 20);
    const int handle_contours = n > 0 ? r.range(1, std::min(n, 3)) : 1;
    put16(input_at, std::uint32_t(contours));
    put16(input_at + 2, f.points);
    put32(input_at + 4, starts_at);
    put32(input_at + 8, ends_at);
    put32(input_at + 0xc, xs_at);
    put32(input_at + 0x10, ys_at);
    put32(input_at + 0x14, on_at);
    put32(input_at + 0x18, x_words_at);
    put32(input_at + 0x1c, y_words_at);
    // The contours: the last end is the last point.
    for (int c = 0; c < handle_contours; ++c) {
        put16(starts_at + 2 * c, std::uint32_t(c * std::max(n, 1) / handle_contours));
        put16(ends_at + 2 * c, c + 1 == handle_contours ? std::uint32_t(last) : std::uint32_t((c + 1) * std::max(n, 1) / handle_contours - 1));
    }
    const int units = p.units != 0 ? p.units : (r.chance(30) ? 0x800 : r.range(0x80, 0x1000));
    const int spread = std::max(p.spread, 1);
    for (unsigned k = 0; k < total; ++k) {
        const auto cluster = r.range(0, spread - 1);
        const auto base = cluster * 40 + r.range(0, 2) - 1;
        const auto other = p.tight ? cluster * 40 : r.range(0, spread - 1) * 40 + r.range(0, 4);
        put16(xs_at + 2 * k, std::uint32_t(r.chance(10) ? r.range(-300, 300) : base));
        put16(ys_at + 2 * k, std::uint32_t(r.chance(10) ? r.range(-300, 300) : other));
        m.push_back(w8(on_at + k, r.chance(70)));
        put32(x_words_at + 4 * k, std::uint32_t(r.range(-40, 40) * 64 + (r.chance(30) ? r.range(-0x40, 0x40) : 0)));
        put32(y_words_at + 4 * k, std::uint32_t(r.range(-40, 40) * 64 + (r.chance(30) ? r.range(-0x40, 0x40) : 0)));
        put32(ox_at + 4 * k, std::uint32_t(r.range(-0x4000, 0x4000)));
        put32(oy_at + 4 * k, std::uint32_t(r.range(-0x4000, 0x4000)));
        // Flag bits: the extremum bits of both passes, the corner bit, the edge bits, the placed bits (0x10, 0x20) and the rest.
        std::uint32_t flags = 0;
        for (const std::uint32_t bit : {0x4u, 0x8u, 0x10u, 0x20u, 0x40u, 0x800u, 0x1000u, 0x2000u, 0x4000u, 0x1u, 0x2u, 0x80u, 0x100u})
            if (r.chance(p.flag_percent)) flags |= bit;
        put16(flags_at + 2 * k, flags);
        const auto neighbour = [&] { return n <= 0 || r.chance(15) ? 0xffffu : std::uint32_t(r.range(0, last)); };
        put16(next_at + 2 * k, neighbour());
        put16(prev_at + 2 * k, neighbour());
        put16(mapping_at + 2 * k, r.any());
        put16(extra_at + 2 * k, r.any());
    }
    // The edge records (8 bytes: kind, class, 2 spare, the two points).
    int edges = p.edges >= 0 ? p.edges : r.range(0, 12);
    if (p.exact_records != 0) edges = int(p.exact_records);
    for (int e = 0; e < edges; ++e) {
        const auto at = edges_at + 8 * std::uint32_t(e);
        m.push_back(w8(at, r.chance(unsigned(p.link_percent)) ? 3 : unsigned(r.range(0, 2))));
        m.push_back(w8(at + 1, r.chance(50) ? 1 : 2));
        m.push_back(w8(at + 2, r.any()));
        m.push_back(w8(at + 3, r.any()));
        const auto edge_point = [&] { return n > 0 && !(p.bad_percent && r.chance(unsigned(p.bad_percent))) ? std::uint32_t(r.range(0, last)) : n > 0 ? 0xffffu : 0u; };
        put16(at + 4, edge_point());
        put16(at + 6, edge_point());
    }
    // The handle.
    put32(handle_at + h_stamp, stamp_a);
    put32(handle_at + h_end_stamp, stamp_b);
    put16(handle_at + h_count, std::uint32_t(handle_contours));
    put32(handle_at + h_starts, starts_at);
    put32(handle_at + h_ends, ends_at);
    put32(handle_at + h_on, on_at);
    put32(handle_at + h_xs, xs_at);
    put32(handle_at + h_ys, ys_at);
    put16(handle_at + h_mode, p.mode >= 0 ? std::uint32_t(p.mode) : std::uint32_t(r.chance(30)));
    put32(handle_at + h_next, next_at);
    put32(handle_at + h_prev, prev_at);
    put32(handle_at + h_extrema, extra_at);
    put32(handle_at + 0x30, arrays + 0x7000);
    put32(handle_at + 0x34, arrays + 0x7100);
    put32(handle_at + h_flags, flags_at);
    put32(handle_at + h_before, arrays + 0x7200);
    put32(handle_at + h_after, arrays + 0x7300);
    put32(handle_at + h_mapping, mapping_at);
    put32(handle_at + h_in_dx, arrays + 0x7400);
    put32(handle_at + h_in_dy, arrays + 0x7600);
    put32(handle_at + h_out_dx, arrays + 0x7800);
    put32(handle_at + h_out_dy, arrays + 0x7a00);
    put32(handle_at + h_edge_count, std::uint32_t(edges));
    put32(handle_at + h_edge_capacity, 64);
    put32(handle_at + h_edges, edges_at);
    put32(handle_at + h_original_xs, ox_at);
    put32(handle_at + h_original_ys, oy_at);
    put32(handle_at + h_capacity, 64);
    put32(handle_at + h_mem, p.real_mem ? p.mem : 0);
    put16(handle_at + h_units, std::uint32_t(units));
    put32(handle_at + h_size_x, std::uint32_t(r.range(8, 0x30)));
    put32(handle_at + h_size_y, std::uint32_t(r.range(8, 0x30)));
    put32(handle_at + h_flavor, std::uint32_t(p.flavor != 0 ? p.flavor : (r.chance(50) ? 1 : 2)));
    put32(handle_at + h_unordered, 0);
    put32(handle_at + h_keep_flat, p.keep_flat >= 0 ? std::uint32_t(p.keep_flat) : std::uint32_t(r.chance(40)));
    put32(handle_at + h_high, 0x2000);
    put32(handle_at + h_low, 0x800);
    put32(handle_at + h_mid, 0x1000);
    put32(handle_at + h_steps, 0);
    put32(handle_at + h_error, p.error >= 0 ? std::uint32_t(p.error) : std::uint32_t(r.chance(50) ? 0 : r.range(-0x80, 0x80)));
    put32(handle_at + h_moved, p.moved_start ? 1 : 0);
    m.push_back(w8(handle_at + h_data_set, 1));
    m.push_back(w8(handle_at + h_ready, 1));
    for (unsigned k = 0; k < 0x60; ++k) put32(handle_at + h_scaled + 4 * k, r.chance(15) ? 0 : std::uint32_t(r.range(-0x400, 0x4000)));
    for (unsigned k = 0; k < 12; ++k) {
        put16(handle_at + h_x_widths + 2 * k, r.chance(25) ? 0 : std::uint32_t(r.range(-40, 600)));
        put16(handle_at + h_y_widths + 2 * k, r.chance(25) ? 0 : std::uint32_t(r.range(-40, 600)));
    }
    for (unsigned k = 0; k < 10; ++k) {
        const auto a = r.range(0, 800), b = r.range(0, 100);
        put16(handle_at + h_heights + 6 * k, std::uint32_t(a));
        put16(handle_at + h_heights + 6 * k + 2, std::uint32_t(a + b));
        put16(handle_at + h_heights + 6 * k + 4, std::uint32_t(b));
    }
    return f;
}
// The memory object of the snapshot (stamps +0 and +0x178, a table with room).
std::uint32_t memory_object(const Scenario& scenario) {
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x17c <= std::uint64_t(low) + size; at += 4)
            if (scenario.r32(std::uint32_t(at)) == 0xab1500ff && scenario.r32(std::uint32_t(at) + 0x178) == 0xa5a55a5a && scenario.r32(std::uint32_t(at) + 8) == 0x1f5)
                return std::uint32_t(at);
    }
    return 0;
}
// The callees of the strategy: the glue's six targets and the direct ones.
const std::vector<std::uint32_t> glue_targets = {0x10753fd0, 0x10754050, 0x107540d0, 0x10754130, 0x10753f70, 0x10753fa0};
const std::vector<std::uint32_t> direct_callees = {0x10753d50, 0x10753d70, 0x10754810, 0x10753d60, 0x107548d0, 0x10751bd0, 0x10753a80, 0x107537c0, 0x107536b0,
                                                  0x10753bf0, 0x10753c30, 0x10753ab0, 0x10753c70, 0x10753980, 0x10754190, 0x10754370};
}

void strategy_cases(const Scenario& scenario, std::vector<Case>& cases) {
    using namespace strat;
    const auto mem = memory_object(scenario);
    std::mt19937 seeds(0x5c0de);
    // Which callees a case answers: all (the strategy's own decisions are compared), all but the bisection, or none (the callees run for real).
    enum class Callees { answered, bisection_real, real };
    const auto add = [&](Rng& r, const Params& base, Callees callees, const std::string& label) -> Case& {
        Params p = base;
        p.mem = mem;
        const auto f = make(r, p);
        Case item{.entry = entry,
                  .registers = {{2, game_toc}, {3, input_at}, {4, handle_at}, {5, 0x55555555}, {6, 0x66666666}, {7, 0x77777777}, {8, 0x88888888}, {9, 0x99999999}, {10, 0xaaaaaaaa}},
                  .memory = f.memory,
                  .label = "hstrat97: strategy " + label};
        item.answers.push_back({reporter, 0});
        if (callees != Callees::real) {
            for (const auto callee : glue_targets) item.answers.push_back({callee, 0});
            for (const auto callee : direct_callees) {
                if (callees == Callees::bisection_real && (callee == 0x10753ab0)) continue;
                item.answers.push_back({callee, callee == 0x10751bd0 ? std::uint32_t(r.range(-3, 40)) : std::uint32_t(0)});
            }
            if (callees == Callees::bisection_real) item.answers.push_back({0x10753a80, 0});
        }
        if (callees == Callees::real)  // the two passes over the untouched points loop on junk data; the rest runs for real
            for (const auto callee : {0x10754190u, 0x10754370u}) item.answers.push_back({callee, 0});
        item.original_owners = {game_malloc, game_free};
        cases.push_back(std::move(item));
        return cases.back();
    };
    Rng r(seeds());
    // The shapes that reach the rare sites first (the call trace pairs the first 32 cases of an entry).
    for (unsigned n = 0; n < 24; ++n) {
        Params p;
        p.points = int(n % 6 == 0 ? 20 : n % 6 + 4);
        p.edges = 12;
        p.link_percent = 100;
        p.flavor = 1;
        p.mode = 0;
        p.keep_flat = n % 2;
        p.error = n % 3 == 0 ? 0 : 5;
        add(r, p, Callees::answered, "first " + std::to_string(n));
    }
    // The bisection runs for real: it moves the stem width, so the strategy starts the records after the last bisected one over (14 steps at most each).
    for (unsigned n = 0; n < 160; ++n) {
        Params p;
        p.points = n % 4 == 0 ? 0 : 6 + int(n % 30);
        p.edges = int(n % 5) * 6;
        p.spread = 2 + int(n % 5);
        p.flag_percent = 30 + int(n % 3) * 20;
        p.keep_flat = n % 2;
        p.flavor = 1;
        p.mode = 0;
        p.error = int(n % 3 == 0 ? 0 : 7);
        add(r, p, Callees::bisection_real, "bisection real " + std::to_string(n));
    }
    // The callees all run for real, on the same handles.
    for (unsigned n = 0; n < 60; ++n) {
        Params p;
        p.points = 3 + int(n % 12);
        p.edges = int(n % 4) * 3;
        p.flavor = 1 + int(n % 2);
        p.mode = int(n % 3 == 0);
        add(r, p, Callees::real, "real " + std::to_string(n));
    }
    // No points or a negative last index: the loops over the points are skipped and the mapping fill is shorter.
    for (int last = -4; last <= 0; ++last)
        for (unsigned n = 0; n < 4; ++n) {
            Params p;
            p.points = last + 1;
            p.empty = last == -1;
            p.edges = int(n * 3);
            p.flavor = 1 + int(n % 2);
            p.mode = int(n / 2);
            add(r, p, Callees::answered, "last " + std::to_string(last) + " " + std::to_string(n));
        }
    // Records whose point is -1 (the assertions on a negative point), pairs that fall into one group (a record that links to itself), and bigger shapes (the
    // unrolled loops over the records that are withdrawn or cleared).
    for (unsigned n = 0; n < 240; ++n) {
        Params p;
        p.points = 4 + int(n % 20);
        p.edges = 6 + int(n % 10);
        p.link_percent = 100;
        p.bad_percent = n % 3 == 0 ? 25 : 0;
        p.tight = n % 3 != 1;
        p.spread = 2 + int(n % 3);
        p.flag_percent = 30 + int(n % 4) * 15;
        p.flavor = 1;
        p.mode = 0;
        add(r, p, n % 4 == 0 ? Callees::bisection_real : Callees::answered, "pairs " + std::to_string(n));
    }
    for (unsigned n = 0; n < 60; ++n) {
        Params p;
        p.points = 24 + int(n % 16);
        p.edges = 30 + int(n % 20);
        p.link_percent = 100;
        p.spread = 6 + int(n % 4);
        p.flag_percent = 50;
        p.flavor = 1;
        p.mode = 0;
        add(r, p, Callees::bisection_real, "large " + std::to_string(n));
    }
    // Flags cleared in the middle of the run (a callee's doing): the checks on the placed points' flags fire.
    for (unsigned n = 0; n < 80; ++n) {
        Params p;
        p.points = 4 + int(n % 12);
        p.edges = 8;
        p.link_percent = 100;
        p.tight = n % 2 == 0;
        p.spread = 3;
        p.flavor = 1;
        p.mode = 0;
        auto& item = add(r, p, Callees::answered, "flags cleared " + std::to_string(n));
        Case::Poke poke{n % 2 == 0 ? 0x10753980u : 0x10753ab0u, {}};
        const auto points = unsigned(p.points) + 4;
        for (unsigned k = 0; k < points; ++k) poke.writes.push_back(w16(flags_at + 2 * k, n % 4 < 2 ? 0x1u : 0x2u | (k & 1 ? 0x10u : 0u)));
        item.pokes.push_back(poke);
    }
    for (unsigned n = 0; n < 500; ++n) {
        Params p;
        p.points = n % 7 == 0 ? 1 + int(n % 3) : 0;
        p.units = n % 5 == 0 ? 0x800 : 0;
        p.spread = 1 + int(n % 6);
        p.flag_percent = 20 + int(n % 4) * 20;
        add(r, p, Callees::answered, "answered " + std::to_string(n));
    }
}
}

FONT_ENGINE_CASES("hstrat97") {
    std::mt19937 random(0x5497);
    const std::pair<unsigned, std::uint32_t> r7{7, 0x77777777}, r8{8, 0x88888888}, r9{9, 0x99999999}, r10{10, 0xaaaaaaaa};
    // ---- 0x1075c980: the heap sift ----
    const auto heap_case = [&](unsigned count, const std::vector<int>& keys, const std::vector<unsigned>& order, std::uint32_t first, std::uint32_t last, const std::string& label) {
        std::vector<Write> memory;
        for (unsigned n = 0; n < count; ++n) {
            memory.push_back(w32(heap_at + 4 * n, record_at + 4 * order[n]));
            memory.push_back(w16(record_at + 4 * order[n], order[n]));
        }
        for (std::size_t n = 0; n < keys.size(); ++n) memory.push_back(w16(keys_at + 2 * std::uint32_t(n), std::uint32_t(keys[n])));
        cases.push_back({.entry = sift, .registers = {{3, heap_at}, {4, first}, {5, last}, {6, keys_at}, r7, r8, r9, r10}, .memory = memory, .label = "hstrat97: sift " + label});
    };
    for (const unsigned count : {1u, 2u, 3u, 4u, 5u, 7u, 8u, 9u, 15u, 16u, 17u, 31u}) {
        std::vector<unsigned> identity(count);
        for (unsigned n = 0; n < count; ++n) identity[n] = n;
        std::vector<int> ascending(count), descending(count), equal(count, 7), mixed(count);
        for (unsigned n = 0; n < count; ++n) { ascending[n] = int(n) - 3; descending[n] = int(count - n); mixed[n] = int(random() % 21) - 10; }
        for (std::uint32_t first = 0; first < count; ++first)
            for (const std::uint32_t last : {first, first + 1, count - 1u, count > 2 ? count - 2u : 0u, count}) {
                if (last > count) continue;
                const std::string where = " count " + std::to_string(count) + " first " + std::to_string(first) + " last " + std::to_string(last);
                heap_case(count, ascending, identity, first, last, "ascending" + where);
                heap_case(count, descending, identity, first, last, "descending" + where);
                heap_case(count, equal, identity, first, last, "equal" + where);
                heap_case(count, mixed, identity, first, last, "mixed" + where);
            }
        for (unsigned n = 0; n < 30; ++n) {
            std::vector<unsigned> order = identity;
            std::shuffle(order.begin(), order.end(), random);
            std::vector<int> keys(count);
            for (auto& key : keys) key = random() % 3 == 0 ? int(std::int16_t(random())) : int(random() % 9) - 4;
            const std::uint32_t first = random() % (count + 1), last = random() % (count + 1);
            heap_case(count, keys, order, first, last, "random " + std::to_string(count) + "/" + std::to_string(n));
        }
    }
    // The extremes of the keys and a first or last beyond the array.
    heap_case(5, {-32768, 32767, -32768, 32767, 0}, {0, 1, 2, 3, 4}, 0, 4, "extreme keys");
    heap_case(5, {1, 2, 3, 4, 5}, {0, 1, 2, 3, 4}, 0, 0x7fffffff, "last far past the array");
    heap_case(5, {1, 2, 3, 4, 5}, {0, 1, 2, 3, 4}, 3, 1, "first after last");
    heap_case(5, {1, 2, 3, 4, 5}, {0, 1, 2, 3, 4}, 0xffffffffu, 4, "first -1");
    heap_case(5, {1, 2, 3, 4, 5}, {0, 1, 2, 3, 4}, 2, 0xffffffffu, "last -1");
    // ---- 0x1075ca60: the polynomial ----
    const auto score_case = [&](std::uint32_t a, std::uint32_t b, std::uint32_t flags, const std::string& label) {
        cases.push_back({.entry = score, .registers = {{3, a}, {4, b}, {5, flags}, {6, 0x66666666}, r7, r8, r9, r10}, .label = "hstrat97: score " + label});
    };
    const std::vector<std::uint32_t> firsts{0xfffffff0u, 0xffffffffu, 0, 1, 2, 3, 4, 5, 8, 15, 16, 17, 100, 0x7fffffff, 0x80000000u};
    const std::vector<std::uint32_t> seconds{0xffffffffu, 0, 1, 0x55, 0x56, 0x57, 0x80, 0x12e, 0x12f, 0x130, 1000, 0x7fffffff, 0x80000000u};
    for (const auto a : firsts)
        for (const auto b : seconds)
            for (const std::uint32_t flags : {0u, 1u, 4u, 5u, 6u, 14u, 15u, 16u, 20u, 0x3fu, 0x40u, 0x41u, 0xffffffffu})
                score_case(a, b, flags, hex(a) + " " + hex(b) + " " + hex(flags));
    // The whole region the zeroing rule names: a up to 3 (and 4), b up to 0x56 (and 0x57), the flags' low bits up to 0xe (and 0xf).
    for (std::uint32_t a = 0; a <= 4; ++a)
        for (std::uint32_t b = 0; b <= 0x57; ++b)
            for (std::uint32_t flags = 0; flags <= 0xf; ++flags) score_case(a, b, flags, "region " + std::to_string(a) + " " + hex(b) + " " + hex(flags));
    for (unsigned n = 0; n < 4000; ++n) score_case(random() % 6 == 0 ? random() : random() % 20, random() % 6 == 0 ? random() : random() % 0x140, random() % 4 == 0 ? random() : random() % 0x50, "random " + std::to_string(n));
    strategy_cases(scenario, cases);
}
}
