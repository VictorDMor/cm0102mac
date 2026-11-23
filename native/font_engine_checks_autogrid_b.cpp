// The font engine, block `autogrid_b` (AUTOGRID.C's second half, 0x10753560..0x107559a0): cases for the hint strategy's callbacks, the passes that
// drive the hint execution, the hint execution itself and the hint handle's creation, run and deletion. The game never runs any of it (FONT.CPP
// passes hint = 0): the hinted end-to-end mode runs the real glyphs of the shipped fonts through these entries (the block's own report names which it
// reaches); these cases add what real glyphs do not reach: the edge values, every branch of each callback, bad stamps and the assertion exits, the
// fixed tables' limits (twelve stem widths, ten height records, capacity of the record array) and the allocator's failure exits.
// States. A handle and a hint input record are laid out in the scratch area from a seed (`Fixture`): the handle's stamps, the per-point arrays
// (rings, flags, partners, directions, original coordinates), the tables of scaled values and stem widths, the records; a case that allocates
// (the handle's creation, the stem width collection, the deletion) uses the snapshot's real memory object and, for the handle, one the ORIGINAL
// constructor builds in a `prepare`. Callees outside the block (the first half's analyses, the strategy, the reporter) are answered where a case
// does not need them. Labels start with "autogrid_b:" (CM_FONT_ENGINE_BLOCK=autogrid_b).
#include "font_engine_checks.hpp"
#include "font_engine_adapter.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t game_toc = 0x108bba90, game_malloc = 0x10775130, game_free = 0x10775170;
constexpr std::uint32_t area = scratch_base + 0x20000, handle_at = area, input_at = area + 0x600, arrays_at = area + 0x1000, out_at = area + 0x6000;
// Offsets (autogrid:: and hint_input:: in the detail header, repeated here: the checks do not include it).
namespace H {
constexpr std::uint32_t stamp = 0, count = 4, starts = 8, ends = 0xc, on_curve = 0x10, xs = 0x14, ys = 0x18, mode = 0x1c, next = 0x20, previous = 0x24, extrema = 0x28,
    flags = 0x2c, before = 0x38, after = 0x3c, in_dx = 0x44, in_dy = 0x48, out_dx = 0x4c, out_dy = 0x50, record_count = 0x54, record_capacity = 0x58, records = 0x5c,
    scaled = 0x60, units = 0x2a0, size_x = 0x2a4, size_y = 0x2a8, flavor = 0x2ac, unordered = 0x34c, capacity = 0x350, heights = 0x354, x_widths = 0x390,
    y_widths = 0x3a8, original_xs = 0x3d0, original_ys = 0x3d4, high = 0x428, low = 0x42c, mid = 0x430, steps = 0x434, error = 0x438, moved = 0x43c,
    ready = 0x44c, data_set = 0x44d, mem = 0x450, keep_flat = 0x454, end_stamp = 0x458;
}
namespace I {
[[maybe_unused]] constexpr std::uint32_t count = 0, points = 2, starts = 4, ends = 8, xs = 0xc, ys = 0x10, on_curve = 0x14, x_words = 0x18, y_words = 0x1c, advance = 0x20, pixels = 0x24;
}
constexpr std::uint32_t stamp_a = 0xa5a0f5a5, stamp_b = 0x0fa55af0;
constexpr std::uint32_t array_size = 0x100;  // bytes between the per-point arrays
// Array addresses.
constexpr std::uint32_t next_at = arrays_at, previous_at = arrays_at + 1 * array_size, extrema_at = arrays_at + 2 * array_size, flags_at = arrays_at + 3 * array_size,
    before_at = arrays_at + 4 * array_size, after_at = arrays_at + 5 * array_size, in_dx_at = arrays_at + 6 * array_size, in_dy_at = arrays_at + 8 * array_size,
    out_dx_at = arrays_at + 10 * array_size, out_dy_at = arrays_at + 12 * array_size, original_x_at = arrays_at + 14 * array_size, original_y_at = arrays_at + 16 * array_size,
    records_at = arrays_at + 18 * array_size, starts_at = arrays_at + 24 * array_size, ends_at = starts_at + 0x40, xs_at = starts_at + 0x80, ys_at = xs_at + 0x80,
    on_at = ys_at + 0x80, x_words_at = on_at + 0x80, y_words_at = x_words_at + 0x200;
constexpr unsigned max_points = 40;

using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;

struct Fixture {
    std::vector<Write> memory;
    unsigned contours = 0, points = 0;
    std::vector<unsigned> first, last;
};
struct Rng {
    std::mt19937 g;
    explicit Rng(std::uint32_t seed) : g(seed) {}
    std::uint32_t any() { return g(); }
    int range(int lo, int hi) { return lo + int(g() % std::uint32_t(hi - lo + 1)); }
    bool chance(unsigned percent) { return g() % 100 < percent; }
    // A value around the edges the callbacks compare against: small, around 0x20/0x40/0x60/0x2a, random, negative.
    std::uint32_t edge() {
        static const std::array<int, 24> edges = {0, 1, -1, 0x1f, 0x20, 0x21, 0x3f, 0x40, 0x41, 0x5f, 0x60, 0x61, 0x29, 0x2a, 0x2b, 0x80, 0xc0, 0x100, -0x20, -0x40, -0x60, -0x61, 0x7fff, -0x8000};
        switch (g() % 4) {
        case 0: return std::uint32_t(edges[g() % edges.size()]);
        case 1: return std::uint32_t(range(-0x200, 0x200));
        case 2: return std::uint32_t(range(-0x40, 0x40));
        default: return g();
        }
    }
};

// A glyph of `contour_sizes` points per contour, random coordinates, and the handle that goes with it. `units` the resolution; options set by the case.
struct Options {
    bool rings = true;           // next/previous point rings built (else left zero)
    int units = 0;               // 0: random
    bool unordered = false;
    bool keep_flat = false;
    unsigned flavor = 0;         // 0: random 1 or 2
    std::uint32_t mem = 0;
    bool stems = true;           // stem width tables random (else zero)
    unsigned flag_mask = 0x7fff;
    bool partners = true;
};
Fixture make_fixture(Rng& r, std::vector<unsigned> sizes, const Options& o = {}) {
    Fixture f;
    auto& m = f.memory;
    unsigned n = 0;
    for (const auto size : sizes) { f.first.push_back(n); n += size; f.last.push_back(n - 1); }
    f.contours = unsigned(sizes.size());
    f.points = n;
    const auto put16 = [&](std::uint32_t at, std::uint32_t v) { m.push_back(w16(at, v)); };
    const auto put32 = [&](std::uint32_t at, std::uint32_t v) { m.push_back(w32(at, v)); };
    // The hint input record.
    put16(input_at + I::count, f.contours);
    put16(input_at + I::points, n);
    put32(input_at + I::starts, starts_at);
    put32(input_at + I::ends, ends_at);
    put32(input_at + I::xs, xs_at);
    put32(input_at + I::ys, ys_at);
    put32(input_at + I::on_curve, on_at);
    put32(input_at + I::x_words, x_words_at);
    put32(input_at + I::y_words, y_words_at);
    for (unsigned c = 0; c < f.contours; ++c) { put16(starts_at + 2 * c, f.first[c]); put16(ends_at + 2 * c, f.last[c]); }
    for (unsigned p = 0; p < n + 4; ++p) {
        if (p < n) {
            put16(xs_at + 2 * p, std::uint32_t(r.range(-300, 300)));
            put16(ys_at + 2 * p, std::uint32_t(r.range(-300, 300)));
            m.push_back(w8(on_at + p, r.chance(70)));
        }
        const bool pixel = r.chance(40);
        const auto x = pixel ? r.range(-40, 40) * 64 : r.range(-0x4000, 0x4000), y = pixel ? r.range(-40, 40) * 64 : r.range(-0x4000, 0x4000);
        put32(x_words_at + 4 * p, std::uint32_t(x));
        put32(y_words_at + 4 * p, std::uint32_t(y));
        put32(original_x_at + 4 * p, r.chance(30) ? std::uint32_t(x) : std::uint32_t(x + r.range(-0x80, 0x80)));
        put32(original_y_at + 4 * p, r.chance(30) ? std::uint32_t(y) : std::uint32_t(y + r.range(-0x80, 0x80)));
        put16(flags_at + 2 * p, r.any() & o.flag_mask);
        const auto neighbour = [&] { return r.chance(25) ? 0xffffu : std::uint32_t(r.range(0, int(n) - 1)); };
        if (o.partners) { put16(before_at + 2 * p, neighbour()); put16(after_at + 2 * p, neighbour()); }
        const auto vector = [&] { return r.chance(20) ? std::uint32_t(r.range(0x3d00, 0x3e00) * (r.chance(50) ? 1 : -1)) : std::uint32_t(r.range(-0x4000, 0x4000)); };
        put32(in_dx_at + 4 * p, vector()); put32(in_dy_at + 4 * p, vector());
        put32(out_dx_at + 4 * p, vector()); put32(out_dy_at + 4 * p, vector());
    }
    if (o.rings) {
        for (unsigned c = 0; c < f.contours; ++c)
            for (unsigned p = f.first[c]; p <= f.last[c]; ++p) {
                put16(next_at + 2 * p, p == f.last[c] ? f.first[c] : p + 1);
                put16(previous_at + 2 * p, p == f.first[c] ? f.last[c] : p - 1);
            }
    }
    // The handle.
    put32(handle_at + H::stamp, stamp_a);
    put32(handle_at + H::end_stamp, stamp_b);
    put16(handle_at + H::count, f.contours);
    put32(handle_at + H::starts, starts_at);
    put32(handle_at + H::ends, ends_at);
    put32(handle_at + H::on_curve, on_at);
    put32(handle_at + H::xs, xs_at);
    put32(handle_at + H::ys, ys_at);
    put16(handle_at + H::mode, r.chance(50));
    put32(handle_at + H::next, next_at);
    put32(handle_at + H::previous, previous_at);
    put32(handle_at + H::extrema, extrema_at);
    put32(handle_at + 0x30, arrays_at + 0x3000);
    put32(handle_at + 0x34, arrays_at + 0x3100);
    put32(handle_at + 0x40, arrays_at + 0x3200);
    put32(handle_at + H::flags, flags_at);
    put32(handle_at + H::before, before_at);
    put32(handle_at + H::after, after_at);
    put32(handle_at + H::in_dx, in_dx_at);
    put32(handle_at + H::in_dy, in_dy_at);
    put32(handle_at + H::out_dx, out_dx_at);
    put32(handle_at + H::out_dy, out_dy_at);
    put32(handle_at + H::records, records_at);
    put32(handle_at + H::record_count, 0);
    put32(handle_at + H::record_capacity, 40);
    put32(handle_at + H::capacity, max_points);
    put32(handle_at + H::original_xs, original_x_at);
    put32(handle_at + H::original_ys, original_y_at);
    put32(handle_at + H::mem, o.mem);
    put16(handle_at + H::units, o.units ? std::uint32_t(o.units) : std::uint32_t(r.chance(30) ? 0x800 : r.range(0x80, 0x1000)));
    put32(handle_at + H::size_x, std::uint32_t(r.chance(20) ? r.range(0, 0x80) : r.range(8, 0x30)));
    put32(handle_at + H::size_y, std::uint32_t(r.chance(20) ? r.range(0, 0x80) : r.range(8, 0x30)));
    put32(handle_at + H::flavor, o.flavor ? o.flavor : unsigned(r.range(1, 2)));
    put32(handle_at + H::unordered, o.unordered);
    put32(handle_at + H::keep_flat, o.keep_flat || r.chance(40));
    put32(handle_at + H::high, 0x2000); put32(handle_at + H::low, 0x800); put32(handle_at + H::mid, std::uint32_t(r.range(0x400, 0x2000)));
    put32(handle_at + H::steps, std::uint32_t(r.chance(30) ? 0 : r.range(0, 16)));
    put32(handle_at + H::error, r.edge()); put32(handle_at + H::moved, r.any() & 1);
    m.push_back(w8(handle_at + H::data_set, 1)); m.push_back(w8(handle_at + H::ready, 1));
    for (unsigned k = 0; k < 0x60; ++k) put32(handle_at + H::scaled + 4 * k, r.chance(15) ? 0 : std::uint32_t(r.range(-0x400, 0x4000)));
    for (unsigned k = 0; k < 12; ++k) {
        put16(handle_at + H::x_widths + 2 * k, o.stems && !r.chance(25) ? std::uint32_t(r.range(-40, 600)) : 0);
        put16(handle_at + H::y_widths + 2 * k, o.stems && !r.chance(25) ? std::uint32_t(r.range(-40, 600)) : 0);
    }
    for (unsigned k = 0; k < 10; ++k) {
        const auto a = r.range(0, 800), b = r.range(0, 100);
        put16(handle_at + H::heights + 6 * k, a); put16(handle_at + H::heights + 6 * k + 2, a + b); put16(handle_at + H::heights + 6 * k + 4, b);
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
}

FONT_ENGINE_CASES("autogrid_b") {
    const auto mem = memory_object(scenario);
    std::mt19937 seeds(0xa6b1d);
    const auto toc = std::pair<unsigned, std::uint32_t>{2, game_toc};
    // The case counts below are those of the exhaustive set (CM_FONT_ENGINE_EXHAUSTIVE=1); the default is a sixth (at least 16): the block's share of the equivalence test
    // stays within two minutes and the patterns of the random cases (their moduli) are still covered.
    const bool exhaustive = std::getenv("CM_FONT_ENGINE_EXHAUSTIVE") != nullptr;
    const auto cases_of = [exhaustive](unsigned count) { return exhaustive ? count : std::max(count / 6, std::min(count, 16u)); };
    const std::uint32_t reporter_entry = 0x107755c0;
    const auto add = [&](std::uint32_t entry, Registers registers, std::vector<Write> memory, const std::string& label) -> Case& {
        registers.push_back(toc);
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = "autogrid_b: " + label});
        cases.back().answers = {{reporter_entry, 0}};
        return cases.back();
    };
    const auto stack = [&](std::uint32_t at, std::uint32_t value) { return w32(scenario.sp() + at, value); };
    const auto fixture = [&](Rng& r, std::vector<unsigned> sizes, const Options& o = {}) { return make_fixture(r, std::move(sizes), o); };
    const std::vector<std::vector<unsigned>> shapes = {{4}, {1}, {2}, {3, 3}, {8}, {5, 2, 1}, {12, 6}, {20}, {9, 9, 9}};

    // ---- the callbacks the strategy calls ----
    // The two stubs the strategy calls (a `blr` each): every register stays as it was.
    for (const std::uint32_t entry : {0x10753d50u, 0x10753d60u})
        add(entry, {{3, handle_at}, {4, 0x44}, {5, 0x55}, {6, 0x66}, {7, 0x77}, {8, 0x88}, {9, 0x99}, {10, 0xaa}}, {}, "stub " + hex(entry));
    for (unsigned n = 0; n < 2; ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {4});
        add(0x10753a80, {{3, handle_at}, {4, 0x1234}, {5, 0x5678}}, f.memory, "reset the bisection " + std::to_string(n));
    }
    // limit_to_band: the entry's index (a halfword), none (0x270f), the value around the table entry's.
    for (unsigned n = 0; n < cases_of(160); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {4});
        const std::uint32_t index = n % 8 == 0 ? 0x270f : n % 8 == 1 ? 0x1270f : n % 8 == 2 ? 0xffff : std::uint32_t(r.range(0, 0x5f));
        add(0x10753cd0, {{3, handle_at}, {4, index}, {5, r.edge()}}, f.memory, "limit to band " + std::to_string(n));
    }
    // limit_to_band against the real table entry: values at the band's edges.
    for (unsigned n = 0; n < cases_of(60); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {4});
        const auto entry = std::uint32_t(n % 6);
        const auto at = handle_at + H::scaled + 4 * entry;
        const std::int32_t reference = std::int32_t(r.range(-0x300, 0x900));
        std::vector<Write> memory = f.memory;
        memory.push_back(w32(at, std::uint32_t(reference)));
        const std::array<int, 8> deltas = {0, 0x5f, 0x60, 0x61, -0x5f, -0x60, -0x61, 1};
        add(0x10753cd0, {{3, handle_at}, {4, entry}, {5, std::uint32_t(reference + deltas[n % 8])}}, memory, "limit to band edge " + std::to_string(n));
    }
    for (unsigned n = 0; n < cases_of(120); ++n) {
        Rng r(seeds());
        std::uint32_t a = r.edge(), b;
        switch (n % 6) {
        case 0: b = a + 0x20; break;
        case 1: b = a - 0x20; break;
        case 2: b = a + 0x21; break;
        case 3: b = a - 0x21; break;
        case 4: b = a; break;
        default: b = r.edge(); break;
        }
        if (n == 6) { a = 0x80000000; b = 0; }
        if (n == 7) { a = 0; b = 0x80000000; }
        add(0x10753d20, {{3, a}, {4, b}}, {}, "hold if close " + std::to_string(n));
    }
    const auto flag_values = std::array<std::uint32_t, 5>{0, 1, 0x10000, 0xffff, 0x7fff};
    for (const bool maximum : {true, false})
        for (unsigned n = 0; n < cases_of(120); ++n) {
            Rng r(seeds());
            const auto f = fixture(r, {5});
            const auto i = std::uint32_t(r.range(0, 8)), j = std::uint32_t(r.range(0, 8));
            auto memory = f.memory;
            if (n % 5 == 0) memory.push_back(w32(x_words_at + 4 * j, std::uint32_t(r.edge())));
            add(maximum ? 0x10753bf0 : 0x10753c30, {{4, input_at}, {5, flag_values[n % 5]}, {6, n % 7 == 0 ? j : i}, {7, j}}, memory,
                std::string(maximum ? "take maximum " : "take minimum ") + std::to_string(n));
        }
    for (unsigned n = 0; n < cases_of(160); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {5});
        const auto i = std::uint32_t(r.range(0, 8)), j = std::uint32_t(r.range(0, 8)), k = std::uint32_t(r.range(0, 8));
        auto memory = f.memory;
        const auto words = (n % 2) ? x_words_at : y_words_at;
        memory.push_back(w32(words + 4 * j, std::uint32_t(r.range(100, 400))));
        memory.push_back(w32(words + 4 * i, std::uint32_t(r.range(-100, 300))));
        add(0x10753c70, {{4, input_at}, {5, flag_values[(n / 2) % 5]}, {6, i}, {7, n % 11 == 0 ? i : j}, {8, n % 13 == 0 ? j : k}}, memory, "remove excess " + std::to_string(n));
    }
    // The excess is taken as a halfword: differences past it.
    for (unsigned n = 0; n < 40; ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {5});
        auto memory = f.memory;
        const auto words = (n % 2) ? x_words_at : y_words_at;
        memory.push_back(w32(words + 4 * 3, std::uint32_t(r.range(0x7000, 0x20000))));
        memory.push_back(w32(words + 4 * 1, std::uint32_t(r.range(-0x20000, 0x100))));
        add(0x10753c70, {{4, input_at}, {5, n % 2 ? 1u : 0u}, {6, 1u}, {7, 3u}, {8, std::uint32_t(r.range(0, 8))}}, memory, "remove excess large " + std::to_string(n));
    }
    for (const bool x : {true, false})
        for (unsigned n = 0; n < cases_of(90); ++n) {
            Rng r(seeds());
            const auto f = fixture(r, {5});
            auto memory = f.memory;
            const auto index = std::uint32_t(r.range(0, 8));
            if (n % 3 == 0) memory.push_back(w32((x ? x_words_at : y_words_at) + 4 * index, std::uint32_t(r.range(-3, 3) * 0x20 + r.range(-1, 1))));
            add(x ? 0x10753f70 : 0x10753fa0, {{4, input_at}, {5, flag_values[n % 5]}, {6, index}}, memory, std::string("round word ") + (x ? "x " : "y ") + std::to_string(n));
        }
    for (const bool x : {true, false})
        for (unsigned n = 0; n < cases_of(160); ++n) {
            Rng r(seeds());
            const auto f = fixture(r, {5});
            const std::uint32_t reference = n % 6 == 0 ? 0x270f : n % 6 == 1 ? 0x1270f : std::uint32_t(r.range(0, 4));
            auto memory = f.memory;
            if (n % 3 == 0) {
                const auto at = (x ? x_words_at : y_words_at) + 4 * 2;
                const auto scaled = handle_at + H::scaled + 4 * (reference & 0xffff);
                if (reference < 5) {
                    memory.push_back(w32(scaled, std::uint32_t(r.range(-0x300, 0x300))));
                    const std::array<int, 8> deltas = {0, 0x5f, 0x60, 0x61, -0x5f, -0x60, -0x61, 0x1000};
                    memory.push_back(w32(at, std::uint32_t(int(r.range(-0x300, 0x300)) + deltas[n % 8])));
                }
            }
            add(x ? 0x10753fd0 : 0x10754050, {{3, handle_at}, {4, input_at}, {5, flag_values[(n / 3) % 5]}, {6, std::uint32_t(r.range(0, 8))}, {7, reference}}, memory,
                std::string("round in band ") + (x ? "x " : "y ") + std::to_string(n));
        }
    for (const bool x : {true, false})
        for (unsigned n = 0; n < cases_of(160); ++n) {
            Rng r(seeds());
            const auto f = fixture(r, {6});
            auto memory = f.memory;
            const auto a = std::uint32_t(r.range(0, 5)), b = std::uint32_t(r.range(0, 5)), c_index = std::uint32_t(r.range(0, 5));
            if (n % 10 == 0) memory.push_back(w16((x ? xs_at : ys_at) + 2 * c_index, r.edge()));
            if (n % 10 == 1) memory.push_back(w16((x ? xs_at : ys_at) + 2 * c_index, 0));  // a zero span faults
            add(x ? 0x107540d0 : 0x10754130, {{4, input_at}, {5, a}, {6, b}, {7, c_index}}, memory, std::string("interpolate ") + (x ? "x " : "y ") + std::to_string(n));
        }
    // place_edge_x / y: i and j are the caller's stack words.
    for (const bool x : {true, false})
        for (unsigned n = 0; n < cases_of(600); ++n) {
            Rng r(seeds());
            Options o;
            if (n % 29 == 0) o.units = 0x8000;
            if (n % 31 == 0) o.units = 1;
            if (n % 37 == 0) o.units = 0;
            const auto f = fixture(r, {6}, o);
            auto memory = f.memory;
            const auto i = std::uint32_t(r.range(0, 5)), j = std::uint32_t(r.range(0, 5));
            memory.push_back(stack(0x38, i));
            memory.push_back(stack(0x3c, j));
            // Distances that land the scaled width around the table entry, and widths that round to nothing.
            if (n % 4 == 0) memory.push_back(w16((x ? xs_at : ys_at) + 2 * j, std::uint32_t(int(r.range(-2, 2)))));
            const std::uint32_t width_index = n % 7 == 0 ? 0xffffffffu : n % 7 == 1 ? 0x10003u : n % 7 == 2 ? 0x8000u : std::uint32_t(r.range(0, 0x5f));
            add(x ? 0x10753d70 : 0x10753e70, {{3, handle_at}, {4, input_at}, {5, width_index}, {7, n % 3 == 0 ? 0u : n % 3 == 1 ? 1u : 0x10000u}}, memory,
                std::string("place edge ") + (x ? "x " : "y ") + std::to_string(n));
        }
    // Distances that land on the thresholds of the width logic (units 64, size k: the scaled distance is dy * k).
    for (const bool x : {true, false})
        for (const int target : {0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x29, 0x2a, 0x2b, 0x40}) {
            for (const unsigned keep : {0u, 1u}) {
                Rng r(seeds());
                Options o;
                o.units = 64;
                o.keep_flat = keep != 0;
                const auto f = fixture(r, {6}, o);
                auto memory = f.memory;
                memory.push_back(w32(handle_at + H::keep_flat, keep));
                memory.push_back(w32(handle_at + (x ? H::size_x : H::size_y), 1));
                memory.push_back(w32(handle_at + H::flavor, std::uint32_t(target % 2 + 1)));
                memory.push_back(w16((x ? xs_at : ys_at) + 2 * 1, 0));
                memory.push_back(w16((x ? xs_at : ys_at) + 2 * 2, std::uint32_t(target)));
                memory.push_back(stack(0x38, 1));
                memory.push_back(stack(0x3c, 2));
                memory.push_back(w32(handle_at + H::scaled + 4 * 3, std::uint32_t(target + (target % 3 - 1) * 0x2a)));
                add(x ? 0x10753d70 : 0x10753e70, {{3, handle_at}, {4, input_at}, {5, target % 2 ? 3u : 0xffffffffu}, {7, 1u}}, memory,
                    std::string("place edge threshold ") + (x ? "x " : "y ") + std::to_string(target) + (keep ? " flat" : ""));
            }
        }
    // The two forwarding routines (two copies): through the glue, to the x or y placement.
    for (const std::uint32_t entry : {0x10754550u, 0x107545b0u})
        for (unsigned n = 0; n < cases_of(60); ++n) {
            Rng r(seeds());
            const auto f = fixture(r, {6});
            const std::uint32_t xflag = n % 3 == 0 ? 0u : n % 3 == 1 ? 1u : 0x10000u;
            add(entry, {{3, handle_at}, {4, input_at}, {5, std::uint32_t(r.range(0, 5)) | (n % 5 == 0 ? 0xffff0000u : 0)}, {6, std::uint32_t(r.range(0, 5))},
                        {7, std::uint32_t(r.range(0, 5))}, {8, xflag}},
                f.memory, "forward " + hex(entry) + " " + std::to_string(n));
        }
    // settle_between_edges (0x107537c0) and snap_stem_edge (0x10753980).
    for (unsigned n = 0; n < cases_of(700); ++n) {
        Rng r(seeds());
        Options o;
        if (n % 40 == 0) o.units = 0;
        const auto f = fixture(r, {8}, o);
        auto memory = f.memory;
        const auto a = std::uint32_t(r.range(0, 7)), b = std::uint32_t(r.range(0, 7)), c_index = std::uint32_t(r.range(0, 7));
        add(0x107537c0, {{3, handle_at}, {4, input_at}, {5, flag_values[n % 5]}, {7, a}, {8, b}, {9, c_index}}, memory, "settle between edges " + std::to_string(n));
    }
    for (unsigned n = 0; n < cases_of(900); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {8});
        auto memory = f.memory;
        // Words that make the width and the spans land around the thresholds.
        const auto a = std::uint32_t(r.range(0, 7)), b = std::uint32_t(r.range(0, 7)), c_index = std::uint32_t(r.range(0, 7)), d = std::uint32_t(r.range(0, 7));
        if (n % 2 == 0) {
            for (const auto idx : {a, b, c_index, d}) {
                memory.push_back(w32(x_words_at + 4 * idx, std::uint32_t(r.range(-0x200, 0x400))));
                memory.push_back(w32(original_x_at + 4 * idx, std::uint32_t(r.range(-0x200, 0x400))));
            }
        }
        add(0x10753980, {{3, handle_at}, {4, input_at}, {5, a}, {6, b}, {7, c_index}, {8, d}}, memory, "snap stem edge " + std::to_string(n));
    }
    // bisect_step.
    for (const bool x : {true, false})
        for (unsigned n = 0; n < cases_of(300); ++n) {
            Rng r(seeds());
            const auto f = fixture(r, {6});
            auto memory = f.memory;
            const auto index = std::uint32_t(r.range(0, 5));
            if (n % 3 == 0) memory.push_back(w32(handle_at + H::steps, std::uint32_t(r.range(12, 16))));
            if (n % 4 == 0) {
                // The word equal to the rounded limited original (zero error): the search starts over.
                memory.push_back(w32((x ? original_x_at : original_y_at) + 4 * index, 0x80));
                memory.push_back(w32((x ? x_words_at : y_words_at) + 4 * index, 0x80));
                memory.push_back(w32(handle_at + H::scaled + 4 * 3, 0x80));
            }
            add(0x10753ab0, {{3, handle_at}, {4, input_at}, {5, x ? flag_values[n % 5] | 1 : 0u}, {6, n % 4 == 0 ? 3u : std::uint32_t(r.range(0, 0x5f))}, {7, index}}, memory,
                std::string("bisect step ") + (x ? "x " : "y ") + std::to_string(n));
        }
    // find_standard_width, widest_stem.
    for (unsigned n = 0; n < cases_of(1200); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {4});
        auto memory = f.memory;
        const std::uint32_t xflag = n % 4 == 0 ? 0u : n % 4 == 1 ? 1u : n % 4 == 2 ? 0x10000u : 0x1ffffu;
        const std::uint32_t yflag = n % 5 == 0 ? 0u : n % 5 == 1 ? 0x10000u : 1u;
        std::uint32_t value = std::uint32_t(r.range(-20, 700));
        if (n % 3 == 0) {
            // Around a table entry: half to one and a half times it.
            const auto k = std::uint32_t(r.range(0, 11));
            const auto base = std::uint32_t(r.range(2, 300));
            memory.push_back(w16(handle_at + (xflag & 0xffff ? H::x_widths : H::y_widths) + 2 * k, base));
            const int h = int(base / 2);
            const std::array<int, 9> offsets = {0, 1, -1, h, h + 1, -h, -h - 1, -h + 1, h - 1};
            value = base + std::uint32_t(offsets[n % 9]);
        }
        add(0x10754610, {{3, handle_at}, {4, xflag}, {5, yflag}, {6, 0x55555555}, {7, value}, {8, 0x88888888}, {9, 0x99999999}, {10, 0xaaaaaaaa}}, memory,
            "find standard width " + std::to_string(n));
    }
    for (const bool x : {true, false})
        for (unsigned n = 0; n < 40; ++n) {
            Rng r(seeds());
            Options o;
            o.stems = n % 5 != 0;
            const auto f = fixture(r, {4}, o);
            add(x ? 0x10754810 : 0x107548d0, {{3, handle_at}, {4, 0x4444}, {5, 0x5555}, {6, 0x6666}, {7, 0x7777}}, f.memory, std::string("widest stem ") + (x ? "x " : "y ") + std::to_string(n));
        }
    // choose_edge (0x107536b0): the stack words at sp + 0x38 (the byte at +0x3b), 0x3c and 0x40.
    for (unsigned n = 0; n < cases_of(400); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {6});
        auto memory = f.memory;
        const auto i = std::uint32_t(r.range(0, 5)), j = std::uint32_t(r.range(0, 5));
        memory.push_back(stack(0x38, std::uint32_t(r.range(0, 255))));
        memory.push_back(stack(0x3c, i));
        memory.push_back(stack(0x40, j));
        // Make the halfword distance a width in the table so that a standard width is found (the placement then goes through the strategy's forward routines).
        if (n % 3 == 0) {
            const auto width = std::uint32_t(r.range(10, 200));
            memory.push_back(w16(xs_at + 2 * i, 0));
            memory.push_back(w16(xs_at + 2 * j, (n % 6 == 0) ? width : std::uint32_t(-int(width))));
            memory.push_back(w16(handle_at + H::x_widths + 2 * (n % 12), width));
            memory.push_back(w16(handle_at + H::y_widths + 2 * (n % 12), width));
        }
        add(0x107536b0, {{3, handle_at}, {4, input_at}, {5, xs_at}, {6, n % 4 == 0 ? 0u : n % 4 == 1 ? 1u : n % 4 == 2 ? 0x10000u : 5u}, {7, n % 5 == 0 ? 0u : 1u}, {8, 0x8888}, {9, 0x9999}, {10, 0xaaaa}},
            memory, "choose edge " + std::to_string(n));
    }
    // ---- the interpolation of the untouched points ----
    for (const bool x : {true, false})
        for (unsigned n = 0; n < (exhaustive ? 90u : 5u); ++n) {
            Rng r(seeds());
            const auto& sizes = shapes[n % shapes.size()];
            Options o;
            o.flag_mask = x ? 0x7fdf : 0x7fef;
            const auto f = fixture(r, sizes, o);
            auto memory = f.memory;
            // Touch about a third of the points; some equal originals; fitted spans around 0x2000.
            const auto words = x ? x_words_at : y_words_at, originals = x ? original_x_at : original_y_at;
            const std::uint16_t bit = x ? 0x10 : 0x20;
            for (unsigned p = 0; p < f.points; ++p) {
                const auto flags = std::uint32_t(r.any() & o.flag_mask) & ~0x30u;
                const bool touched = r.chance(35);
                memory.push_back(w16(flags_at + 2 * p, flags | (touched ? bit : 0)));
                if (r.chance(25) && p > 0) memory.push_back(w32(originals + 4 * p, std::uint32_t(r.range(-0x40, 0x40) * 0)));
                if (n % 4 == 0 && touched) memory.push_back(w32(words + 4 * p, std::uint32_t(r.range(-0x6000, 0x6000))));
                if (n % 5 == 0) memory.push_back(w32(originals + 4 * p, std::uint32_t(r.range(0, 3) * 0x100)));
            }
            auto& item = add(x ? 0x10754190 : 0x10754370, {{3, handle_at}, {4, input_at}, {9, 0x99999999}, {10, 0xaaaaaaaa}}, memory,
                             std::string("interpolate untouched ") + (x ? "x " : "y ") + std::to_string(n));
            // A mutated ring (next_point) loops for ever on both sides and each such run costs the alarm: most of these cases are not mutated (a prepare, even empty, says so).
            if (n % 45 != 0) item.prepare = [](State&) {};  // (a prepare does not stop the mutation; the case count does)
        }
    // Fitted spans of the untouched points' neighbours at the limit of the 16.16 path (0x2000).
    for (const bool x : {true, false})
        for (const int span : {0x1fff, 0x2000, 0x2001, -0x2001})
            for (unsigned variant = 0; variant < (exhaustive ? 3u : 1u); ++variant) {
                Rng r(seeds());
                Options o;
                const auto f = fixture(r, {4}, o);
                auto memory = f.memory;
                const auto words = x ? x_words_at : y_words_at, originals = x ? original_x_at : original_y_at;
                const std::uint16_t bit = x ? 0x10 : 0x20;
                for (unsigned p = 0; p < 4; ++p) memory.push_back(w16(flags_at + 2 * p, p == 0 || p == 2 ? bit : 0));
                memory.push_back(w32(words + 0, std::uint32_t(span)));
                memory.push_back(w32(words + 8, 0));
                memory.push_back(w32(originals + 0, variant == 0 ? 0x100 : 0x80));
                memory.push_back(w32(originals + 8, variant == 2 ? 0x100 : 0));
                memory.push_back(w32(originals + 4, 0x40));
                memory.push_back(w32(originals + 12, 0x60));
                add(x ? 0x10754190 : 0x10754370, {{3, handle_at}, {4, input_at}}, memory,
                    std::string("interpolate untouched span ") + (x ? "x " : "y ") + std::to_string(span) + " " + std::to_string(variant))
                    .prepare = [](State&) {};
            }
    // ---- link_contours: the first half's analyses stand in (answered), the real acceptance results alternate ----
    for (unsigned n = 0; n < cases_of(120); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, shapes[n % shapes.size()], Options{.rings = false});
        auto& item = add(0x10753560, {{3, handle_at}, {9, 0x99999999}, {10, 0xaaaaaaaa}}, f.memory, "link contours " + std::to_string(n));
        if (n % 20 != 0) item.prepare = [](State&) {};  // (see interpolate untouched)
    }
    // ---- the edge records ----
    for (unsigned n = 0; n < cases_of(400); ++n) {
        Rng r(seeds());
        Options o;
        const auto f = fixture(r, shapes[n % shapes.size()], o);
        auto memory = f.memory;
        std::uint32_t capacity = n % 5 == 0 ? std::uint32_t(r.range(0, 4)) : 40;
        memory.push_back(w32(handle_at + H::record_capacity, capacity));
        // Make partners point back often (the records need it), and flags with 0x40.
        for (unsigned p = 0; p < f.points && n % 2 == 0; ++p) {
            const auto before = std::uint32_t(r.range(0, int(f.points) - 1));
            memory.push_back(w16(before_at + 2 * p, before));
            memory.push_back(w16(after_at + 2 * before, p));
            if (n % 3 == 0) memory.push_back(w16(after_at + 2 * p, before));
            memory.push_back(w16(flags_at + 2 * p, r.any() & 0x7fff));
        }
        add(0x10754990, {{3, handle_at}, {4, out_at}, {5, out_at + 0x400}, {6, 0x66666666}, {7, 0x77777777}, {8, 0x88888888}, {9, 0x99999999}, {10, 0xaaaaaaaa}}, memory,
            "edge records " + std::to_string(n));
    }
    // The direction test of the second partner: the dot product of the in and out directions at 0x376c..0x376e.
    for (const int dot : {0x376c, 0x376d, 0x376e, 0x4000, 0}) {
        Rng r(seeds());
        const auto f = fixture(r, {4});
        auto memory = f.memory;
        for (unsigned p = 0; p < 4; ++p) {
            memory.push_back(w16(before_at + 2 * p, 0xffff));
            memory.push_back(w16(after_at + 2 * p, 0xffff));
            memory.push_back(w16(flags_at + 2 * p, 0));
        }
        memory.push_back(w16(before_at + 0, 1)); memory.push_back(w16(after_at + 0, 1));
        memory.push_back(w16(before_at + 2, 0)); memory.push_back(w16(after_at + 2, 0));
        memory.push_back(w32(in_dx_at, 0x4000)); memory.push_back(w32(in_dy_at, 0));
        memory.push_back(w32(out_dx_at, std::uint32_t(dot))); memory.push_back(w32(out_dy_at, 0));
        add(0x10754990, {{3, handle_at}, {4, out_at}, {5, out_at + 0x400}}, memory, "edge records dot " + hex(std::uint32_t(dot)));
    }
    // Classes at the limit 0x3dd2 of a direction component.
    for (const int component : {0x3dd1, 0x3dd2, 0x3dd3, -0x3dd2, -0x3dd3})
        for (const bool second : {false, true}) {
            Rng r(seeds());
            const auto f = fixture(r, {4});
            auto memory = f.memory;
            for (unsigned p = 0; p < 4; ++p) { memory.push_back(w16(before_at + 2 * p, (p + 1) % 4)); memory.push_back(w16(after_at + 2 * p, 0xffff)); memory.push_back(w16(flags_at + 2 * p, 0x40)); }
            memory.push_back(w32(in_dx_at, second ? 0 : std::uint32_t(component))); memory.push_back(w32(in_dy_at, second ? std::uint32_t(component) : 0));
            memory.push_back(w32(out_dx_at, second ? 0 : std::uint32_t(component))); memory.push_back(w32(out_dy_at, second ? std::uint32_t(component) : 0));
            add(0x10754990, {{3, handle_at}, {4, out_at}, {5, out_at + 0x400}}, memory, "edge records class " + hex(std::uint32_t(component)) + (second ? " y" : " x"));
        }
    // A record array that reaches its capacity inside the second partner test (stops the contour loop).
    for (unsigned capacity : {1u, 2u, 3u}) {
        Rng r(seeds());
        const auto f = fixture(r, {6, 6});
        auto memory = f.memory;
        memory.push_back(w32(handle_at + H::record_capacity, capacity));
        for (unsigned p = 0; p < f.points; ++p) {
            memory.push_back(w16(before_at + 2 * p, (p + 1) % f.points));
            memory.push_back(w16(after_at + 2 * p, (p + f.points - 1) % f.points));
            memory.push_back(w16(flags_at + 2 * p, 0x40));
        }
        add(0x10754990, {{3, handle_at}, {4, out_at}, {5, out_at + 0x400}}, memory, "edge records capacity " + std::to_string(capacity));
    }
    // ---- stem widths: the records are made here (kind 3, classes 1 and 2) ----
    if (mem) {
        for (unsigned n = 0; n < cases_of(200); ++n) {
            Rng r(seeds());
            Options o;
            o.mem = mem;
            const auto f = fixture(r, shapes[n % shapes.size()], o);
            auto memory = f.memory;
            const auto count = std::uint32_t(n % 17 == 0 ? 0 : r.range(1, 20));
            memory.push_back(w32(handle_at + H::record_count, count));
            for (std::uint32_t k = 0; k < count; ++k) {
                const auto at = records_at + 8 * k;
                memory.push_back(w8(at, r.chance(80) ? 3 : std::uint32_t(r.range(0, 4))));
                memory.push_back(w8(at + 1, std::uint32_t(r.chance(40) ? 1 : r.chance(50) ? 2 : r.range(0, 0xff))));
                memory.push_back(w16(at + 4, std::uint32_t(r.range(0, int(f.points) - 1))));
                memory.push_back(w16(at + 6, std::uint32_t(r.range(0, int(f.points) - 1))));
            }
            if (n % 3 == 0) memory.push_back(w16(handle_at + H::units, std::uint32_t(r.range(3, 200))));
            auto& item = add(0x10754cd0, {{3, handle_at}, {4, input_at}, {5, out_at}, {6, out_at + 4}, {7, out_at + 8}, {8, out_at + 12}, {9, 0x99999999}, {10, 0xaaaaaaaa}}, memory,
                             "stem widths " + std::to_string(n));
            item.original_owners = {game_malloc, game_free};
        }
    }
    if (mem) {
        for (const int distance : {99, 100, 101, 102, 103, -101, -102})
            for (const bool x : {true, false}) {
                Rng r(seeds());
                Options o;
                o.mem = mem;
                const auto f = fixture(r, {4}, o);
                auto memory = f.memory;
                memory.push_back(w16(handle_at + H::units, 300));
                memory.push_back(w32(handle_at + H::record_count, 2));
                for (unsigned k = 0; k < 2; ++k) {
                    memory.push_back(w8(records_at + 8 * k, 3)); memory.push_back(w8(records_at + 8 * k + 1, x ? 1 : 2));
                    memory.push_back(w16(records_at + 8 * k + 4, 0)); memory.push_back(w16(records_at + 8 * k + 6, 1));
                }
                memory.push_back(w16((x ? xs_at : ys_at), 0)); memory.push_back(w16((x ? xs_at : ys_at) + 2, std::uint32_t(distance)));
                auto& item = add(0x10754cd0, {{3, handle_at}, {4, input_at}, {5, out_at}, {6, out_at + 4}, {7, out_at + 8}, {8, out_at + 12}}, memory,
                                 std::string("stem widths limit ") + std::to_string(distance) + (x ? " x" : " y"));
                item.original_owners = {game_malloc, game_free};
            }
    }
    // ---- run_hinter (0x10754ff0) through its checked entry (0x10754f70), and glyph hinting (0x107552b0) ----
    const auto run_modes = std::array<std::uint32_t, 8>{0, 1, 2, 3, 0x10000, 0x10002, 4, 0xffff};
    for (unsigned n = 0; n < cases_of(320); ++n) {
        Rng r(seeds());
        Options o;
        o.unordered = n % 5 == 0;
        o.mem = mem;
        const auto f = fixture(r, shapes[n % shapes.size()], o);
        auto memory = f.memory;
        const auto mode = run_modes[n % run_modes.size()];
        const std::uint32_t size_flag = n % 9 == 0 ? 4u : n % 9 == 1 ? 0x10002u : n % 9 == 2 ? 0x10001u : n % 2 ? 2u : 3u;
        if (n % 11 == 0) memory.push_back(w32(handle_at + H::capacity, 3));  // the glyph has more points than the handle's capacity: the assertion
        memory.push_back(stack(0x38, out_at + 16));
        for (unsigned p = 0; p < f.points; ++p) memory.push_back(w16(flags_at + 2 * p, r.any() & 0x7fff));
        const bool checked = n % 2 == 0;
        auto& item = add(checked ? 0x10754f70 : 0x10754ff0,
                         checked ? Registers{{3, handle_at}, {4, input_at}, {5, std::uint32_t(r.range(0, 1))}, {6, size_flag}, {7, out_at}, {8, out_at + 4}, {9, out_at + 8},
                                             {10, out_at + 12}}
                                 : Registers{{3, handle_at}, {4, input_at}, {5, std::uint32_t(r.range(0, 1))}, {6, size_flag}, {7, mode}, {8, out_at}, {9, out_at + 4}, {10, out_at + 8}},
                         memory, std::string("run hinter ") + (checked ? "checked " : "") + std::to_string(n));
        if (n % 7 == 3) item.answers.clear();  // the reporter runs for real
        if (checked && n % 4 == 0) {  // a handle with bad stamps
            item.memory.push_back(w32(handle_at + (n % 8 == 0 ? H::stamp : H::end_stamp), 0x12345678));
        }
        if (checked && n % 14 == 2) item.registers[0] = {3, 0};
        item.original_owners = {game_malloc, game_free};
        if (n % 9 != 0) item.prepare = [](State&) {};  // the strategy's loops under a mutated ring cost the alarm (see interpolate untouched)
    }
    // set_hint_data (0x107551c0): the 0x6c bytes of hint data copied and each checked.
    for (unsigned n = 0; n < cases_of(80); ++n) {
        Rng r(seeds());
        const auto f = fixture(r, {4});
        auto memory = f.memory;
        const auto data = area + 0x4000;
        for (unsigned k = 0; k < 10; ++k) {
            const auto a = r.range(0, 800), b = r.range(0, 100);
            memory.push_back(w16(data + 6 * k, std::uint32_t(a)));
            memory.push_back(w16(data + 6 * k + 2, std::uint32_t(a + b + (n % 3 == 2 && k == n % 10 ? 1 : 0))));
            memory.push_back(w16(data + 6 * k + 4, std::uint32_t(b)));
        }
        const std::uint32_t pointer = n % 7 == 0 ? 0u : data;
        auto& item = add(0x107551c0, {{3, n % 9 == 0 ? 0u : handle_at}, {4, pointer}, {5, n % 2 ? 1u : 2u}, {6, 0x66666666}, {7, 0x77777777}, {8, 0x88888888}, {9, 0x99999999}, {10, 0xaaaaaaaa}},
                         memory, "set hint data " + std::to_string(n));
        if (n % 9 == 3) item.memory.push_back(w32(handle_at + H::end_stamp, 0));
        if (n % 9 == 4) item.memory.push_back(w32(handle_at + H::stamp, 0));
        if (n % 4 == 0) item.answers = {};  // the reporter runs for real (the dialog's answer is the checker's)
    }
    // hint_glyph (0x107552b0): the copy of the glyph's words (every count around the passes of eight), the flavor and data checks, then the run.
    for (unsigned n = 0; n < cases_of(240); ++n) {
        Rng r(seeds());
        std::vector<unsigned> sizes = {static_cast<unsigned>(n % 24 + 1)};
        Options o;
        o.mem = mem;
        const auto f = fixture(r, sizes, o);
        auto memory = f.memory;
        if (n % 6 == 0) memory.push_back(w32(handle_at + H::flavor, std::uint32_t(r.range(0, 4))));
        if (n % 7 == 0) memory.push_back(w8(handle_at + H::data_set, 0));
        memory.push_back(w16(input_at + I::points, f.points));
        const std::uint32_t quality = n % 5 == 0 ? 3u : n % 5 == 1 ? 0x10000u : n % 5 == 2 ? 1u : n % 5 == 3 ? 0u : 0x10003u;
        const std::uint32_t phantom = n % 8 == 0 ? 0xfffffffcu : 4u;
        auto& item = add(0x107552b0, {{3, n % 17 == 0 ? 0u : handle_at}, {4, input_at}, {5, quality}, {6, std::uint32_t(r.range(2, 3))}, {7, std::uint32_t(r.range(0, 3))},
                                      {8, n % 3 == 0 ? 0u : n % 3 == 1 ? 1u : 0x10000u}, {9, phantom}}, memory, "hint glyph " + std::to_string(n));
        if (n % 3 == 0) item.answers.clear();  // the reporter runs for real (it reads what the registers hold)
        if (n % 13 == 0) item.memory.push_back(w32(handle_at + H::end_stamp, 0));
        if (n % 19 == 2) item.memory.push_back(w32(handle_at + H::stamp, 0));  // the first stamp wrong
        item.original_owners = {game_malloc, game_free};
    }
    // A real longjmp through the handle's constructor: the transformation (T2K.C's API entry, which sets the jump buffer) with the hinter on, from each of the
    // snapshot's three scalers, the table of the memory object nearly full so that the k-th allocation of the constructor raises 0x271c; the entry catches it and
    // frees the object's blocks (the original's constructor is generated and the rewrite's native: both unwind through the same entry).
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4) {
            if (scenario.r32(std::uint32_t(at)) != 0x5a1234a5 || scenario.r32(std::uint32_t(at) + 0x5f4) != 0xa5fedc5a) continue;
            const auto t2k = std::uint32_t(at), scaler_mem = scenario.r32(t2k + 4);
            for (unsigned k = 1; k <= 9; ++k) {
                std::vector<Write> memory = {w32(out_at + 0x100, 0x10000), w32(out_at + 0x104, 0), w32(out_at + 0x108, 0), w32(out_at + 0x10c, 0x10000),
                                             w32(scaler_mem + 4, scenario.r32(scaler_mem + 8) - k), w32(t2k + 0x148, 0)};
                auto& item = add(0x1075eef0, {{3, t2k}, {4, 1}, {5, 14}, {6, 14}, {7, out_at + 0x100}, {8, 0}, {9, out_at}, {10, 0xa0a0a0a0}}, memory,
                                 "transformation with the hinter, table full after " + std::to_string(k) + " allocations (real longjmp) scaler " + hex(t2k));
                item.answers = {{reporter_entry, 0}};
            }
        }
    }
    // ---- the handle: made by the original constructor in a prepare, then deleted, resized; and made by the routine under check ----
    if (mem) {
        for (unsigned n = 0; n < cases_of(60); ++n) {
            const std::uint32_t points = n < 8 ? n : 1 + (n * 7) % 90, units = n % 4 == 0 ? 0x800 : 0x100 + n * 16;
            auto& item = add(0x107555d0, {{3, mem}, {4, points}, {5, units}, {6, out_at}, {7, 0x77}, {8, 0x88}, {9, 0x99}, {10, 0xaa}}, {}, "new handle " + std::to_string(points));
            item.original_owners = {game_malloc, game_free};
        }
        // The table made nearly full so that the k-th allocation of the creation raises (the longjmp answered: both sides carry on after it).
        for (unsigned k = 1; k <= 9; ++k) {
            auto& item = add(0x107555d0, {{3, mem}, {4, 20}, {5, 0x800}, {6, out_at}}, {}, "new handle, table full at allocation " + std::to_string(k));
            item.original_owners = {game_malloc, game_free};
            item.answers = {{reporter_entry, 0}, {0x10773f30, 0}};
            item.prepare = [mem, k](State& s) {
                guest::Cpu c(s);
                const auto capacity = std::uint32_t(c.u32(mem + 8));
                c.put32(mem + 4, capacity - (k - 1));
            };
        }
        // Created by the original, then the deletion, the size and the stamps under check.
        const auto prepared = [&](std::uint32_t entry, Registers (*regs)(std::uint32_t), const std::string& label, unsigned seed_n) -> Case& {
            auto& item = add(entry, {}, {}, label);
            item.prepare = [=](State& s) {
                guest::Cpu c(s);
                const auto saved = s.registers;
                c.set({{3, mem}, {4, 12 + seed_n % 20}, {5, 0x800}, {6, out_at}});
                font_engine_run_original(s, 0x107555d0);
                const auto handle = c.u32(out_at);
                s.registers = saved;
                for (const auto& [reg, value] : regs(handle)) c.set(reg, value);
                c.set(2, game_toc);
                // The font's hint data (ten height records {height, height + shift, shift}): the size setup divides by them.
                for (std::uint32_t k = 0; k < 10; ++k) {
                    c.put16(handle + H::heights + 6 * k, 80 * k + 40);
                    c.put16(handle + H::heights + 6 * k + 2, 80 * k + 40 + 10 + k);
                    c.put16(handle + H::heights + 6 * k + 4, 10 + k);
                }
                c.put8(handle + H::data_set, 1);
                if (seed_n % 5 == 3) c.put32(handle + H::end_stamp, 0);
                if (seed_n % 5 == 4) c.put32(handle + H::stamp, 0x1234);
                if (seed_n % 11 == 5) c.set(3, 0);
            };
            item.original_owners = {game_malloc, game_free};
            return item;
        };
        for (unsigned n = 0; n < 40; ++n)
            prepared(0x10755840, [](std::uint32_t handle) { return Registers{{3, handle}, {4, 0x44}, {5, 0x55}, {6, 0x66}, {7, 0x77}, {8, 0x88}, {9, 0x99}, {10, 0xaa}}; },
                     "delete handle " + std::to_string(n), n);
        for (unsigned n = 0; n < 40; ++n) {
            prepared(0x10755930, [](std::uint32_t handle) { return Registers{{3, handle}, {4, 20}, {5, 22}, {6, out_at}, {7, 0x77}, {8, 0x88}, {9, 0x99}, {10, 0xaa}}; },
                                  "set hint size " + std::to_string(n), n);
        }
    }
}
}
