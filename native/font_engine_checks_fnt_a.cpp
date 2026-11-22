// The font engine, block `fnt_a` (FNT, first third, 0x107559a0..0x10757fb0): cases for the TrueType interpreter's entry, its loops, the rounding and
// projection callbacks and the opcodes that set vectors, reference points and zone pointers, work the stack and shift points. The game runs the entry
// and the plain loop at load (symbol.ttf's fpgm, six times in the end-to-end run) and no handler of this block: the rest is evidence from these cases.
//
// The interpreter state is built in the scratch area around a real hint font of the snapshot (a copy of its 0x14c bytes, so the graphic state, the
// installed opcode table and the function words are the game's own): an execution context (interpreter::), two zones of the arrays this file lays
// out (current, original and unscaled coordinates, contours, touched bytes), a stack with the values a case pushes, a cvt, and the function words the
// real TOC holds (the axis moves and projects, the round functions). A case calls one entry with r3 = the context and the arguments the original's
// callers set; the checker runs the original and the C++ from that state and compares r3-r10, the saved registers and every byte written. Labels
// start with "fnt_a:" (CM_FONT_ENGINE_BLOCK=fnt_a).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <array>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t area = scratch_base + 0x80000;
constexpr std::uint32_t context_at = area, hint_at = area + 0x200, arrays_at = area + 0x1000, stack_at = area + 0x6000, cvt_at = area + 0x7000,
                        program_at = area + 0x8000, storage_at = area + 0x7800, slots_at = area + 0x9000;
constexpr std::uint32_t points_per_zone = 16, zone_stride = 0x1000;
constexpr std::uint32_t longjmp_entry = 0x10773f30, game_toc = 0x108bba90;

// Toc words the context's function words and the graphic state's round function take.
constexpr int toc_move_x = -0x3c4c, toc_project_x = -0x3c50, toc_move_y = -0x3c70, toc_project_y = -0x3c74, toc_move_any = -0x3c78, toc_project_any = -0x3c7c,
              toc_dual_any = -0x3c80, round_to_grid_slot = -0x5b3c, round_half_slot = -0x5b40, round_double_slot = -0x5b44, round_up_slot = -0x5b48,
              round_down_slot = -0x5b4c, round_off_slot = -0x5b50, round_super_slot = -0x5b54, round_45_slot = -0x5b58;

struct Hint { std::uint32_t at = 0; };
// A hint font of the snapshot with the opcode table installed: the scaler's sfnt's hint font.
Hint real_hint(const Scenario& scenario) {
    auto& s = scenario.s;
    for (const auto& [low, size] : s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4) {
            if (scenario.r32(std::uint32_t(at)) != 0x5a1234a5 || scenario.r32(std::uint32_t(at) + 0x5f4) != 0xa5fedc5a) continue;
            const auto sfnt = scenario.r32(std::uint32_t(at) + 0x144);
            if (!sfnt) continue;
            const auto hint = scenario.r32(sfnt + 4);
            if (!hint) continue;
            const auto table = scenario.r32(hint + 0x18 + 0x8c);
            if (!table) continue;
            if ((scenario.r32(table) & 0xffffff) == (scenario.toc(-0x3c64) & 0xffffff)) return {hint};
        }
    }
    return {};
}

struct Point { std::int32_t x, y, original_x, original_y; std::int16_t unscaled_x, unscaled_y; std::uint8_t touched; };

// A world: the writes that put the interpreter's state in the scratch area.
struct World {
    const Scenario& scenario;
    std::vector<Write> memory;
    std::uint32_t hint = 0;
    std::uint32_t context = context_at, global = hint_at + 0x18, zones = hint_at + 0xf4;
    std::uint32_t zone_at(unsigned n) const { return zones + 0x2c * n; }
    std::uint32_t array(unsigned zone, unsigned kind) const { return arrays_at + zone * zone_stride + kind * 0x100; }
    void w32(std::uint32_t at, std::uint32_t v) { memory.push_back(::cm::aot::font_engine_checks::w32(at, v)); }
    void w16(std::uint32_t at, std::uint32_t v) { memory.push_back(::cm::aot::font_engine_checks::w16(at, v)); }
    void w8(std::uint32_t at, std::uint32_t v) { memory.push_back(::cm::aot::font_engine_checks::w8(at, v)); }
    explicit World(const Scenario& sc, std::uint32_t real_hint_at) : scenario(sc), hint(real_hint_at) {
        for (std::uint32_t at = 0; at < 0x14c; at += 4) w32(hint_at + at, scenario.r32(hint + at));
        // The graphic state: the stack, the cvt, the storage, the round function, the scale callback and the current program flag.
        w32(global + 0x00, stack_at);
        w32(global + 0x04, storage_at);
        w32(global + 0x08, cvt_at);
        w32(global + 0x70, scenario.toc(round_to_grid_slot));
        w32(global + 0x74, 0xffffffc0);
        w16(global + 0x7c, 0x40);
        w16(global + 0x7e, 0);
        w16(global + 0x80, 0x3f);
        w32(global + 0x78, 0x2d413ccd);
        w32(global + 0x58, 0x44);
        w32(global + 0x5c, 0x55);
        w32(global + 0x60, 0x66);
        w32(global + 0x6c, 0x40);
        w32(global + 0xb8, 0x10000);
        w32(global + 0xbc, 0x10000);
        w32(global + 0x10, 0x100000);
        w16(global + 0xc, 16);
        for (unsigned n = 0; n < 64; ++n) w32(cvt_at + 4 * n, 0x40 * n + (n % 3) * 7);
        // The execution context: both axes, the glyph zone for the zone pointers, the x-axis function words.
        w32(context + 0x00, program_at);
        w32(context + 0x04, 0);
        for (const auto zp : {0x08u, 0x0cu, 0x10u}) w32(context + zp, zone_at(1));
        w16(context + 0x14, 0x4000); w16(context + 0x16, 0); w16(context + 0x18, 0x4000); w16(context + 0x1a, 0);
        w16(context + 0x1c, 0x4000); w16(context + 0x1e, 0);
        w32(context + 0x20, stack_at);
        w32(context + 0x24, zones);
        w32(context + 0x28, global);
        w32(context + 0x2c, 0);
        for (const auto rp : {0x30u, 0x34u, 0x38u}) w32(context + rp, 0);
        w16(context + 0x3c, 0); w16(context + 0x3e, 0); w16(context + 0x40, 0x4000);
        w32(context + 0x44, scenario.toc(toc_move_x));
        w32(context + 0x48, scenario.toc(toc_project_x));
        w32(context + 0x4c, scenario.toc(toc_project_x));
        w32(context + 0x50, scenario.toc(-0x3c6c));
        w32(context + 0x54, scenario.toc(-0x3c54));
        w32(context + 0x58, scenario.toc(-0x3c58));
    }
    void set_stack(const std::vector<std::uint32_t>& values) {
        for (std::size_t n = 0; n < values.size(); ++n) w32(stack_at + 4 * std::uint32_t(n), values[n]);
        w32(context + 0x20, stack_at + 4 * std::uint32_t(values.size()));
    }
    // A zone of `points` points in contours of the given ends (inclusive), from the point list.
    void set_zone(unsigned n, const std::vector<Point>& points, const std::vector<std::int16_t>& starts, const std::vector<std::int16_t>& ends) {
        const auto z = zone_at(n);
        for (unsigned kind = 0; kind < 7; ++kind) w32(z + 4 * kind, array(n, kind));
        w16(z + 0x1c, std::uint32_t(starts.size()));
        w32(z + 0x20, array(n, 7)); w32(z + 0x24, array(n, 8)); w32(z + 0x28, array(n, 9));
        for (std::size_t p = 0; p < points.size(); ++p) {
            const auto o = 4 * std::uint32_t(p), h = 2 * std::uint32_t(p);
            w32(array(n, 0) + o, std::uint32_t(points[p].x)); w32(array(n, 1) + o, std::uint32_t(points[p].y));
            w32(array(n, 2) + o, std::uint32_t(points[p].original_x)); w32(array(n, 3) + o, std::uint32_t(points[p].original_y));
            w16(array(n, 4) + h, std::uint16_t(points[p].unscaled_x)); w16(array(n, 5) + h, std::uint16_t(points[p].unscaled_y));
            w8(array(n, 6) + p, p & 1);
            w8(array(n, 9) + p, points[p].touched);
        }
        for (std::size_t c = 0; c < starts.size(); ++c) { w16(array(n, 7) + 2 * std::uint32_t(c), std::uint16_t(starts[c])); w16(array(n, 8) + 2 * std::uint32_t(c), std::uint16_t(ends[c])); }
    }
};
}

namespace {
using Registers = std::vector<std::pair<unsigned, std::uint32_t>>;
constexpr std::array<std::uint32_t, 24> value_edges = {0, 1, 2, 0x0f, 0x10, 0x1f, 0x20, 0x21, 0x3f, 0x40, 0x41, 0x7f, 0x80, 0x1000, 0x7fff, 0x8000, 0x10000, 0x7fffffff,
                                                       0x80000000, 0x80000001, 0xffffffff, 0xffffffc0, 0xffffffe0, 0xffffffe1};
constexpr std::array<std::uint32_t, 10> comp_edges = {0, 1, 0xffffffff, 0x20, 0xffffffe0, 0x40, 0xffffffc0, 5, 0x7fffffff, 0x80000000};
// A random 26.6 coordinate: mostly moderate, now and then an edge.
std::int32_t coordinate(std::mt19937& random) {
    switch (random() % 12) {
        case 0: return 0;
        case 1: return 1;
        case 2: return -1;
        case 3: return 0x7fff;
        case 4: return -0x8000;
        case 5: return std::int32_t(random() % 64) * 64;
        default: return std::int32_t(random() % 6001) - 3000;
    }
}
std::vector<Point> random_points(std::mt19937& random, unsigned count) {
    std::vector<Point> points;
    for (unsigned n = 0; n < count; ++n) {
        Point p{};
        p.x = coordinate(random); p.y = coordinate(random);
        p.original_x = p.x + std::int32_t(random() % 7) - 3; p.original_y = p.y + std::int32_t(random() % 7) - 3;
        p.unscaled_x = std::int16_t(std::int32_t(random() % 4001) - 2000); p.unscaled_y = std::int16_t(std::int32_t(random() % 4001) - 2000);
        p.touched = std::uint8_t(random() % 4);
        points.push_back(p);
    }
    return points;
}
// Two zones of sixteen points in the default three contours.
void default_zones(World& w, std::mt19937& random) {
    const std::vector<std::int16_t> starts = {0, 5, 10}, ends = {4, 9, 15};
    w.set_zone(0, random_points(random, points_per_zone), starts, ends);
    w.set_zone(1, random_points(random, points_per_zone), starts, ends);
}
// Points laid out for the line opcodes: the first eight are on the axes and diagonals, the rest random.
std::vector<Point> line_points(std::mt19937& random) {
    auto points = random_points(random, points_per_zone);
    const std::array<std::pair<int, int>, 10> fixed = {{{0, 0}, {64, 0}, {0, 64}, {64, 64}, {-64, -64}, {1000, 3}, {3, 1000}, {128, 64}, {0, 0}, {-1000, 1000}}};
    for (std::size_t n = 0; n < fixed.size(); ++n) {
        points[n].x = fixed[n].first; points[n].y = fixed[n].second;
        points[n].original_x = fixed[n].first + int(n); points[n].original_y = fixed[n].second - int(n);
    }
    return points;
}
}

FONT_ENGINE_CASES("fnt_a") {
    const auto hint = real_hint(scenario);
    if (!hint.at) return;
    const auto first_case = cases.size();
    std::mt19937 random(0xf47a);
    const auto make = [&] { World w(scenario, hint.at); default_zones(w, random); return w; };
    const auto add = [&](std::uint32_t entry, World& w, Registers registers, const std::string& label) -> Case& {
        registers.insert(registers.begin(), {3, w.context});
        registers.insert(registers.begin(), {2, game_toc});  // a snapshot taken at an import boundary has r2 = 0
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(w.memory), .label = "fnt_a: " + label});
        return cases.back();
    };
    const auto bare = [&](std::uint32_t entry, Registers registers, const std::string& label) -> Case& {
        registers.insert(registers.begin(), {2, game_toc});
        cases.push_back({.entry = entry, .registers = std::move(registers), .label = "fnt_a: " + label});
        return cases.back();
    };
    const auto vec = [](std::int32_t v) { return std::uint32_t(v); };

    // ---- 0x107559a0: the illegal opcode: longjmp(context + 0x5c, 0x1101), answered here; the real transfer is the interpreter's own cases below ----
    {
        auto w = make();
        add(0x107559a0, w, {}, "illegal opcode (longjmp answered)").answers = {{longjmp_entry, 0}};
    }

    // ---- The rounding functions: every value of the edge set against every compensation ----
    for (const auto entry : {0x107559d0u, 0x10755a20u, 0x10755a60u, 0x10755ab0u, 0x10755b00u})
        for (const auto value : value_edges)
            for (const auto compensation : comp_edges) bare(entry, {{3, context_at}, {4, value}, {5, compensation}}, "round " + hex(entry) + " " + hex(value) + " " + hex(compensation));
    for (const auto value : value_edges) bare(0x10755b50, {{3, context_at}, {4, value}, {5, 7}}, "round off " + hex(value));
    // The super rounds: the graphic state's mask (period), phase and threshold; the 2.30 period for the 45-degree round.
    struct Super { std::uint32_t mask; std::int32_t period, phase, threshold; std::uint32_t period45; };
    const std::array<Super, 8> supers = {{{0xffffffe0, 0x20, 0, 0x1f, 0x2d413ccd}, {0xffffffc0, 0x40, 0x20, 0x3f, 0x16a09e66}, {0xffffff80, 0x80, 0x10, 0x20, 0x5a827999},
                                          {0xfffffc18, 0x3e7, 0, 0x1e, 0x40000000}, {0xffffffc0, 0x40, -0x20, -1, 0x2d413ccd}, {0xffffffe0, 0x20, 0x7fff, 0x7fff, 0x10000},
                                          {0xffffffe0, 0x20, -0x8000, -0x8000, 0x7fffffff}, {0xffffffc0, 0x40, 0x10, 0x3f, 0x1}}};
    for (const auto entry : {0x10755b60u, 0x10755be0u})
        for (const auto& super : supers)
            for (const auto value : {0u, 1u, 0x1fu, 0x20u, 0x3fu, 0x40u, 0x7fu, 0x1000u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 0xffffffe0u, 0xffffffc1u, 0xfffff000u})
                for (const auto compensation : {0u, 3u, 0xfffffffdu, 0x20u}) {
                    auto w = make();
                    w.w32(w.global + 0x74, super.mask); w.w16(w.global + 0x7c, vec(super.period)); w.w16(w.global + 0x7e, vec(super.phase)); w.w16(w.global + 0x80, vec(super.threshold));
                    w.w32(w.global + 0x78, super.period45);
                    add(entry, w, {{4, value}, {5, compensation}}, std::string(entry == 0x10755b60 ? "super round " : "super 45 round ") + hex(value) + " " + hex(compensation) + " period " + hex(std::uint32_t(super.period)) + " phase " + hex(vec(super.phase)));
                }

    // ---- The move functions ----
    const std::array<std::uint32_t, 8> deltas = {0, 1, 0xffffffff, 0x40, 0xffffffc0, 0x7fffffff, 0x80000000, 0x1234};
    for (const unsigned point : {0u, 3u, 15u})
        for (const auto delta : deltas)
            for (const unsigned zone : {0u, 1u}) {
                auto w = make();
                add(0x10755e00, w, {{4, w.zone_at(zone)}, {5, point}, {6, delta}}, "move x zone " + std::to_string(zone) + " point " + std::to_string(point) + " delta " + hex(delta));
                auto v = make();
                add(0x10755e30, v, {{4, v.zone_at(zone)}, {5, point}, {6, delta}}, "move y zone " + std::to_string(zone) + " point " + std::to_string(point) + " delta " + hex(delta));
            }
    struct Free { std::int32_t x, y, dot; };
    const std::array<Free, 12> frees = {{{0x4000, 0, 0x4000}, {0, 0x4000, 0x4000}, {0x2d41, 0x2d41, 0x2d41}, {-0x2d41, 0x2d41, -0x2d41}, {0x4000, 0, 0x2d41}, {0, 0x4000, 0x400},
                                         {0x7fff, 0x7fff, 0x4000}, {-0x8000, 0x4000, 0x1}, {0, 0, 0x4000}, {0, 0, 0x2000}, {0x1234, -0x3210, 0x4000}, {0x3000, 0x2000, -0x3fff}}};
    for (const auto& free : frees)
        for (const auto delta : deltas) {
            auto w = make();
            w.w16(w.context + 0x18, vec(free.x)); w.w16(w.context + 0x1a, vec(free.y)); w.w16(w.context + 0x40, vec(free.dot));
            add(0x10755cc0, w, {{4, w.zone_at(1)}, {5, 7}, {6, delta}}, "move general free " + hex(vec(free.x)) + "," + hex(vec(free.y)) + " dot " + hex(vec(free.dot)) + " delta " + hex(delta));
        }

    // ---- The project functions ----
    struct Axes { std::int32_t px, py; };
    const std::array<Axes, 9> axes = {{{0x4000, 0}, {0, 0x4000}, {0x2d41, 0x2d41}, {-0x2d41, 0x2d41}, {0x7fff, -0x8000}, {0, 0}, {1, -1}, {0x1234, 0x3210}, {-0x4000, 0}}};
    const std::array<std::uint32_t, 9> coords = {0, 1, 0xffffffff, 0x40, 0xffffffc0, 0x7fff, 0x7fffffff, 0x80000000, 0x12345678};
    for (const auto& axis : axes)
        for (const auto x : coords)
            for (const auto y : {0u, 0x40u, 0xffffffffu, 0x7fffffffu, 0x80000000u}) {
                auto w = make();
                w.w16(w.context + 0x14, vec(axis.px)); w.w16(w.context + 0x16, vec(axis.py)); w.w16(w.context + 0x1c, vec(axis.py)); w.w16(w.context + 0x1e, vec(axis.px));
                add(0x10755e60, w, {{4, x}, {5, y}}, "project " + hex(vec(axis.px)) + "," + hex(vec(axis.py)) + " at " + hex(x) + "," + hex(y));
                auto v = make();
                v.w16(v.context + 0x14, vec(axis.py)); v.w16(v.context + 0x16, vec(axis.px)); v.w16(v.context + 0x1c, vec(axis.px)); v.w16(v.context + 0x1e, vec(axis.py));
                add(0x10755ec0, v, {{4, x}, {5, y}}, "dual project " + hex(vec(axis.px)) + "," + hex(vec(axis.py)) + " at " + hex(x) + "," + hex(y));
            }
    for (const auto x : coords) {
        bare(0x10755f20, {{3, context_at}, {4, x}, {5, 0x55555555}}, "project x " + hex(x));
        bare(0x10755f30, {{3, context_at}, {4, 0x55555555}, {5, x}}, "project y " + hex(x));
    }

    // ---- The cvt and the single width ----
    for (const unsigned index : {0u, 1u, 7u, 63u}) {
        auto w = make();
        add(0x10755f40, w, {{4, index}}, "read cvt " + std::to_string(index));
    }
    struct Ratio { std::uint32_t down, up; };
    const std::array<Ratio, 5> ratios = {{{0x10000, 0x10000}, {0x8000, 0x20000}, {0x12345, 0x6789a}, {0, 0}, {0xffff0000, 0x7fffffff}}};
    for (const auto& axis : axes)
        for (const auto& ratio : ratios) {
            for (const unsigned index : {0u, 5u, 63u}) {
                auto w = make();
                w.w16(w.context + 0x14, vec(axis.px)); w.w16(w.context + 0x16, vec(axis.py)); w.w32(w.global + 0xb8, ratio.down); w.w32(w.global + 0xbc, ratio.up);
                add(0x10755f60, w, {{4, index}}, "scaled cvt " + std::to_string(index) + " vector " + hex(vec(axis.px)) + "," + hex(vec(axis.py)) + " ratios " + hex(ratio.down) + "," + hex(ratio.up));
            }
            auto v = make();
            v.w16(v.context + 0x14, vec(axis.px)); v.w16(v.context + 0x16, vec(axis.py)); v.w32(v.global + 0xb8, ratio.down); v.w32(v.global + 0xbc, ratio.up);
            v.w32(v.global + 0x60, 0x2345);
            add(0x10756030, v, {}, "scaled single width vector " + hex(vec(axis.px)) + "," + hex(vec(axis.py)) + " ratios " + hex(ratio.down) + "," + hex(ratio.up));
        }
    for (const auto value : {0u, 0x1234u, 0xffffffffu, 0x80000000u}) {
        auto w = make();
        w.w32(w.global + 0x60, value);
        add(0x10756020, w, {}, "single width " + hex(value));
    }
    for (const unsigned index : {0u, 3u, 63u})
        for (const auto delta : deltas) {
            auto w = make();
            add(0x107560e0, w, {{4, 0x99999999}, {5, index}, {6, delta}}, "shift cvt " + std::to_string(index) + " by " + hex(delta));
        }

    // ---- The vector opcodes ----
    for (const auto entry : {0x10756520u, 0x10756560u}) {
        auto w = make();
        add(entry, w, {}, "set axes " + hex(entry));
        auto v = make();
        v.w16(v.context + 0x14, 0x1234); v.w16(v.context + 0x18, 0x4321); v.w16(v.context + 0x40, 0x7777);
        add(entry, v, {{4, 0x11111111}, {5, 0x22222222}, {6, 0x33333333}}, "set axes over other vectors " + hex(entry));
    }
    const std::array<std::int32_t, 9> clamps = {0, 1, 0x3ff, 0x400, -0x3ff, -0x400, 0x4000, -0x4000, 0x7fff};
    for (const auto entry : {0x107565a0u, 0x10756630u})
        for (const unsigned opcode : {2u, 3u, 4u, 5u, 6u, 7u})
            for (const auto first : clamps)
                for (const auto second : {0, 0x2000, -0x3fff}) {
                    auto w = make();
                    w.w32(w.context + 4, opcode);
                    w.w16(w.context + 0x14, vec(first)); w.w16(w.context + 0x16, vec(second)); w.w16(w.context + 0x18, vec(first)); w.w16(w.context + 0x1a, vec(second));
                    add(entry, w, {}, "axis vector opcode " + hex(opcode) + " " + hex(vec(first)) + "," + hex(vec(second)));
                }
    // The line opcodes: points on the axes and diagonals (the first eight), every pair, the odd opcode turning the vector a quarter.
    const auto line_world = [&](std::uint32_t opcode, unsigned p1, unsigned p2, bool swapped_zones) {
        World w(scenario, hint.at);
        w.set_zone(0, line_points(random), {0, 5, 10}, {4, 9, 15});
        w.set_zone(1, line_points(random), {0, 5, 10}, {4, 9, 15});
        w.w32(w.context + 4, opcode);
        w.set_stack({p1, p2});
        if (swapped_zones) { w.w32(w.context + 0x0c, w.zone_at(0)); w.w32(w.context + 0x10, w.zone_at(1)); }
        else { w.w32(w.context + 0x0c, w.zone_at(1)); w.w32(w.context + 0x10, w.zone_at(1)); }
        w.w16(w.context + 0x18, 0x2d41); w.w16(w.context + 0x1a, 0x2d41);
        return w;
    };
    for (const auto entry : {0x107566b0u, 0x10756a00u, 0x10756820u})
        for (const unsigned odd : {0u, 1u}) {
            const std::uint32_t opcode = entry == 0x10756820u ? 0x86 + odd : entry == 0x107566b0u ? 6 + odd : 8 + odd;
            for (unsigned p1 = 0; p1 < 10; ++p1)
                for (unsigned p2 = 0; p2 < 10; ++p2) {
                    if (p1 == p2 && p1 > 1) continue;
                    auto w = line_world(opcode, p1, p2, p1 % 2 == 0 && p2 % 3 == 0);
                    add(entry, w, {}, "line vector " + hex(entry) + " opcode " + hex(opcode) + " points " + std::to_string(p1) + "," + std::to_string(p2));
                }
            for (unsigned n = 0; n < 6; ++n) {
                auto w = line_world(opcode, 10 + n, 10 + (n * 3 + 1) % 6, n % 2);
                add(entry, w, {}, "line vector random " + hex(entry) + " opcode " + hex(opcode) + " " + std::to_string(n));
            }
        }
    const std::array<std::uint32_t, 10> halves = {0, 1, 0x4000, 0xffffc000, 0x7fff, 0xffff8000, 0x1234, 0xffff, 0x10000, 0x12345678};
    for (const auto entry : {0x10756b60u, 0x10756c00u})
        for (const auto x : halves)
            for (const auto y : halves) {
                auto w = make();
                w.set_stack({x, y});
                w.w16(w.context + 0x14, 0x3000); w.w16(w.context + 0x16, 0x1000); w.w16(w.context + 0x18, 0x2d41); w.w16(w.context + 0x1a, 0x2d41);
                add(entry, w, {}, "set vector from stack " + hex(entry) + " " + hex(x) + "," + hex(y));
            }
    for (const auto entry : {0x10756c90u, 0x10756cc0u, 0x10756cf0u})
        for (const auto& axis : axes) {
            auto w = make();
            for (const auto base : {0x14u, 0x18u}) { w.w16(w.context + base, vec(axis.px)); w.w16(w.context + base + 2, vec(axis.py)); }
            if (entry == 0x10756cc0) { w.w16(w.context + 0x18, vec(axis.py)); w.w16(w.context + 0x1a, vec(axis.px)); }
            w.set_stack({1, 2, 3});
            add(entry, w, {{4, 0x44444444}, {5, 0x55555555}}, "vector copy " + hex(entry) + " " + hex(vec(axis.px)) + "," + hex(vec(axis.py)));
        }

    // ---- ISECT: five stack values (point, then the first line's two points, then the second's), lines of every direction ----
    const auto isect_world = [&](const std::vector<Point>& first, const std::vector<Point>& second, std::vector<std::uint32_t> stack) {
        World w(scenario, hint.at);
        w.set_zone(0, first, {0, 5, 10}, {4, 9, 15});
        w.set_zone(1, second, {0, 5, 10}, {4, 9, 15});
        w.w32(w.context + 0x08, w.zone_at(0)); w.w32(w.context + 0x0c, w.zone_at(1)); w.w32(w.context + 0x10, w.zone_at(1));
        w.set_stack(std::move(stack));
        return w;
    };
    {
        // Lines: a (points 0-1 of the first zone) against b (points 0-1 of the second): the stack is [point, a0, a1, b0, b1] with the points 2 and 3 as in/out.
        const std::array<std::array<std::pair<int, int>, 4>, 16> lines = {{
            {{{0, 0}, {100, 0}, {50, -50}, {50, 50}}},          // horizontal x vertical
            {{{0, 0}, {0, 100}, {-50, 50}, {50, 50}}},          // vertical x horizontal
            {{{0, 0}, {100, 100}, {0, 100}, {100, 0}}},         // diagonals
            {{{0, 0}, {100, 50}, {0, 40}, {100, 90}}},          // parallel
            {{{0, 0}, {100, 0}, {0, 10}, {100, 10}}},           // horizontal parallel
            {{{0, 0}, {0, 100}, {10, 0}, {10, 100}}},           // vertical parallel
            {{{0, 0}, {100, 0}, {20, 7}, {70, -13}}},           // horizontal x slanted
            {{{0, 0}, {0, 100}, {20, 7}, {70, -13}}},           // vertical x slanted
            {{{0, 0}, {100, 3}, {50, 50}, {53, -50}}},          // flat x steep
            {{{0, 0}, {3, 100}, {-50, 50}, {50, 53}}},          // steep x flat
            {{{5, 5}, {5, 5}, {0, 0}, {10, 10}}},               // degenerate first line
            {{{0, 0}, {10, 10}, {7, 7}, {7, 7}}},               // degenerate second line
            {{{-3000, -3000}, {3000, 3100}, {-3000, 3000}, {3000, -3200}}},
            {{{0, 0}, {0x7fff, 3}, {1, 0x7fff}, {2, -0x7fff}}},
            {{{0, 0}, {64, 64}, {64, 0}, {128, 64}}},           // parallel diagonals offset
            {{{0, 0}, {64, 128}, {0, 0}, {128, 64}}},           // lines meeting at their first points
        }};
        for (std::size_t n = 0; n < lines.size(); ++n)
            for (const unsigned swap : {0u, 1u}) {
                auto first = random_points(random, points_per_zone), second = random_points(random, points_per_zone);
                first[0].x = lines[n][0].first; first[0].y = lines[n][0].second; first[1].x = lines[n][1].first; first[1].y = lines[n][1].second;
                second[0].x = lines[n][2].first; second[0].y = lines[n][2].second; second[1].x = lines[n][3].first; second[1].y = lines[n][3].second;
                auto w = isect_world(first, second, {swap ? 5u : 4u, 0, 1, 0, 1});
                add(0x10756d20, w, {}, "isect line pair " + std::to_string(n) + (swap ? " (other point)" : ""));
                auto v = isect_world(first, second, {2, 1, 0, 1, 0});
                add(0x10756d20, v, {}, "isect line pair " + std::to_string(n) + " reversed");
            }
        {
            // A denominator of -2^31 (its magnitude stays negative) with a zero numerator: the second line's first point is the answer.
            auto first = random_points(random, points_per_zone), second = random_points(random, points_per_zone);
            first[0].x = 5; first[0].y = 0; first[1].x = 5; first[1].y = 100;
            second[0].x = 5; second[0].y = 7; second[1].x = 5 + std::int32_t(0x80000000u); second[1].y = 9;
            auto w = isect_world(first, second, {4, 0, 1, 0, 1});
            add(0x10756d20, w, {}, "isect denominator -2^31, numerator zero");
        }
        for (unsigned n = 0; n < 40; ++n) {
            auto w = isect_world(random_points(random, points_per_zone), random_points(random, points_per_zone),
                                 {unsigned(random() % 16), unsigned(random() % 16), unsigned(random() % 16), unsigned(random() % 16), unsigned(random() % 16)});
            add(0x10756d20, w, {}, "isect random " + std::to_string(n));
        }
    }

    // ---- The graphic state words ----
    for (const auto value : {0u, 1u, 0x40u, 0xffffffffu, 0x80000000u, 0x12345678u})
        for (const auto entry : {0x10756fc0u, 0x10756fe0u, 0x10757000u}) {
            auto w = make();
            w.set_stack({0x77777777, value});
            add(entry, w, {{4, 0x13131313}}, "state word " + hex(entry) + " " + hex(value));
        }
    for (const auto value : {0u, 1u, 0x40u, 0x7fffu, 0x8000u, 0xffffffffu, 0x12345678u}) {
        auto w = make();
        w.set_stack({0x77777777, value});
        w.w32(w.global + 0x98, scenario.toc(-0x3c24)); w.w32(w.global + 0xa8, 0x10); w.w32(w.global + 0xb0, 0x20); w.w16(w.global + 0xb4, 4);
        add(0x10757020, w, {}, "single width value " + hex(value));
        auto v = make();
        v.set_stack({value});
        v.w32(v.global + 0x98, scenario.toc(-0x3c28)); v.w32(v.global + 0xa8, 0x10); v.w32(v.global + 0xac, 0x7); v.w32(v.global + 0xb0, 0x3);
        add(0x10757020, v, {}, "single width value, divide callback " + hex(value));
    }
    for (const unsigned opcode : {0x10u, 0x11u, 0x12u, 0x13u, 0x16u, 0x17u, 0x18u, 0x20u, 0x21u, 0x22u, 0x0fu, 0x30u, 0xffu})
        for (const auto value : {0u, 1u, 7u, 0xffffffffu, 0x80000000u}) {
            auto w = make();
            w.w32(w.context + 4, opcode);
            w.set_stack({0x88888888, value});
            add(0x10757080, w, {}, "reference or loop opcode " + hex(opcode) + " value " + hex(value));
        }
    for (const unsigned opcode : {0x12u, 0x13u, 0x14u, 0x15u, 0x16u, 0x17u, 0x00u})
        for (const auto zone : {0u, 1u, 2u, 0xffffffffu, 0x1000000u}) {
            auto w = make();
            w.w32(w.context + 4, opcode);
            w.set_stack({0x99999999, zone});
            add(0x107570e0, w, {}, "zone pointer opcode " + hex(opcode) + " zone " + hex(zone));
        }
    for (const unsigned flag : {1u, 0u}) {
        for (const unsigned period : {0u, 0x40u, 0x80u, 0xc0u})
            for (const unsigned phase : {0u, 0x10u, 0x20u, 0x30u})
                for (const unsigned threshold : {0u, 1u, 4u, 5u, 8u, 0xfu}) {
                    auto w = make();
                    w.w16(w.global + 0x7c, 0x3333);  // a period the flag-clear path replaces
                    add(0x10757290, w, {{4, period | phase | threshold}, {5, flag}}, "round state flag " + std::to_string(flag) + " operand " + hex(period | phase | threshold));
                }
        for (const unsigned operand : {0u, 0xffu, 0x80u, 0x51u, 0x1234u, 0xffffffffu}) {
            auto w = make();
            w.set_stack({0x1111, operand});
            add(flag ? 0x10757150 : 0x107571a0, w, {{4, 0x66666666}, {5, 0x77777777}}, std::string(flag ? "SROUND " : "S45ROUND ") + hex(operand));
        }
    }
    for (const unsigned opcode : {0x18u, 0x19u, 0x3du, 0x7au, 0x7bu, 0x7cu, 0x7du, 0x7eu, 0x00u, 0xffu, 0x17u}) {
        auto w = make();
        w.w32(w.context + 4, opcode);
        add(0x107571f0, w, {}, "round function opcode " + hex(opcode));
    }

    // ---- The stack ----
    {
        auto w = make();
        for (const auto points : {0u, 1u, 3u, 15u}) {
            auto v = make();
            v.w16(v.zone_at(1) + 0x1c, 1);
            v.w16(v.array(1, 8), points);
            add(0x10757420, v, {{4, 0x12121212}}, "advance width, last point " + std::to_string(points));
        }
        // A zone of a different contour count (the last end read from the end array).
        for (const unsigned contours : {1u, 2u, 3u}) {
            auto v = make();
            v.w16(v.zone_at(1) + 0x1c, contours);
            add(0x10757420, v, {}, "advance width, contours " + std::to_string(contours));
        }
    }
    for (const auto value : {0u, 1u, 0xffffffffu, 0x80000000u, 0x12345678u}) {
        auto w = make();
        w.set_stack({0x31, value});
        add(0x10757470, w, {}, "DUP " + hex(value));
        auto v = make();
        v.set_stack({0x31, 0x32, value});
        add(0x107574a0, v, {}, "SWAP " + hex(value));
        auto u = make();
        u.set_stack({0x31, 0x32, 0x33, value});
        add(0x10757560, u, {}, "ROLL " + hex(value));
    }
    {
        auto w = make();
        w.set_stack({});
        add(0x10757490, w, {}, "CLEAR on an empty stack");
        auto v = make();
        v.set_stack({1, 2, 3, 4, 5});
        add(0x10757490, v, {}, "CLEAR");
        for (const unsigned depth : {0u, 1u, 2u, 7u, 8u, 9u, 40u}) {
            std::vector<std::uint32_t> values;
            for (unsigned n = 0; n < depth; ++n) values.push_back(0x100 + n);
            auto u = make();
            u.set_stack(values);
            add(0x107574c0, u, {}, "DEPTH " + std::to_string(depth));
        }
        // An odd stack pointer: the depth rounds toward zero (the pointer is a multiple of four in the game; this is the arithmetic).
        auto odd = make();
        odd.set_stack({1, 2, 3});
        odd.w32(odd.context + 0x20, stack_at + 10);
        add(0x107574c0, odd, {}, "DEPTH, stack pointer not a multiple of four");
        auto below = make();
        below.set_stack({1, 2, 3});
        below.w32(below.context + 0x20, stack_at - 6);
        add(0x107574c0, below, {}, "DEPTH, stack pointer below the base");
    }
    for (const unsigned k : {1u, 2u, 3u, 5u, 8u}) {
        std::vector<std::uint32_t> values;
        for (unsigned n = 0; n < 10; ++n) values.push_back(0x200 + n);
        values.push_back(k);
        auto w = make();
        w.set_stack(values);
        add(0x107574f0, w, {}, "CINDEX " + std::to_string(k));
        auto v = make();
        v.set_stack(values);
        add(0x10757510, v, {}, "MINDEX " + std::to_string(k));
    }
    {
        auto w = make();
        w.set_stack({0x10, 0x20, 0});
        add(0x10757510, w, {}, "MINDEX 0");
        auto v = make();
        v.set_stack({0x10, 0x20, 0});
        add(0x107574f0, v, {}, "CINDEX 0");
    }

    // ---- MDAP and MIAP: the round functions, the vector configurations, the two zones ----
    struct Config { const char* name; std::int32_t px, py, fx, fy, dot; std::int32_t move, project, dual; };
    const std::array<Config, 4> configs = {{{"x axis", 0x4000, 0, 0x4000, 0, 0x4000, toc_move_x, toc_project_x, toc_project_x},
                                            {"y axis", 0, 0x4000, 0, 0x4000, 0x4000, toc_move_y, toc_project_y, toc_project_y},
                                            {"diagonal", 0x2d41, 0x2d41, 0x2d41, 0x2d41, 0x3fff, toc_move_any, toc_project_any, toc_dual_any},
                                            {"free x, project y", 0, 0x4000, 0x4000, 0, 0x400, toc_move_any, toc_project_any, toc_dual_any}}};
    const std::array<int, 7> round_slots = {round_to_grid_slot, round_half_slot, round_double_slot, round_up_slot, round_down_slot, round_off_slot, round_super_slot};
    const auto configure = [&](World& w, const Config& config, int round_slot) {
        w.w16(w.context + 0x14, vec(config.px)); w.w16(w.context + 0x16, vec(config.py)); w.w16(w.context + 0x18, vec(config.fx)); w.w16(w.context + 0x1a, vec(config.fy));
        w.w16(w.context + 0x1c, vec(config.px)); w.w16(w.context + 0x1e, vec(config.py)); w.w16(w.context + 0x40, vec(config.dot));
        w.w32(w.context + 0x44, scenario.toc(config.move)); w.w32(w.context + 0x48, scenario.toc(config.project)); w.w32(w.context + 0x4c, scenario.toc(config.dual));
        w.w32(w.global + 0x70, scenario.toc(round_slot));
        w.w32(w.global + 0x74, 0xffffffc0); w.w16(w.global + 0x7c, 0x40); w.w16(w.global + 0x7e, 0x20); w.w16(w.global + 0x80, 0x3f);
    };
    for (const auto& config : configs)
        for (const int round_slot : round_slots)
            for (const unsigned opcode : {0x2eu, 0x2fu})
                for (const unsigned point : {0u, 3u, 15u}) {
                    auto w = make();
                    configure(w, config, round_slot);
                    w.w32(w.context + 4, opcode);
                    w.set_stack({0x41, point});
                    add(0x10757580, w, {}, std::string("MDAP ") + hex(opcode) + " " + config.name + " round " + hex(vec(round_slot)) + " point " + std::to_string(point));
                }
    for (const auto& config : configs)
        for (const int round_slot : {round_to_grid_slot, round_super_slot, round_off_slot})
            for (const unsigned opcode : {0x3eu, 0x3fu})
                for (const unsigned index : {0u, 5u, 63u})
                    for (const unsigned twilight : {0u, 1u})
                        for (const std::uint32_t cut_in : {0x44u, 0xfffu, 0u, 0x80000000u}) {
                            auto w = make();
                            configure(w, config, round_slot);
                            w.w32(w.context + 4, opcode);
                            w.w32(w.global + 0x58, cut_in);
                            if (twilight) w.w32(w.context + 0x08, w.zone_at(0));
                            w.set_stack({0x42, 6, index});
                            // The point 6 of the zone: the cvt entry puts it at its distance in the twilight zone, else only moves it.
                            w.w32(w.context + 0x34, 0);
                            add(0x10757650, w, {}, std::string("MIAP ") + hex(opcode) + " " + config.name + " round " + hex(vec(round_slot)) + " cvt " + std::to_string(index) +
                                                  (twilight ? " twilight" : " glyph zone") + " cut-in " + hex(cut_in));
                        }
    {
        // A cvt entry far from the point (the cut-in test both ways) with the diagonal vector.
        for (const std::uint32_t entry_value : {0u, 0x40u, 0xffffffc0u, 0x7fffffffu, 0x1234u}) {
            auto w = make();
            configure(w, configs[2], round_to_grid_slot);
            w.w32(w.context + 4, 0x3f);
            w.w32(w.global + 0x58, 0x100);
            w.w32(cvt_at + 4 * 9, entry_value);
            w.set_stack({0, 9});
            add(0x10757650, w, {}, "MIAP cvt value " + hex(entry_value));
        }
    }

    // ---- IUP ----
    {
        const auto iup_world = [&](std::uint32_t opcode, const std::vector<Point>& points, std::vector<std::int16_t> starts, std::vector<std::int16_t> ends) {
            World w(scenario, hint.at);
            w.set_zone(0, random_points(random, points_per_zone), {0, 5, 10}, {4, 9, 15});
            w.set_zone(1, points, starts, ends);
            w.w32(w.context + 4, opcode);
            w.w32(w.context + 0x10, w.zone_at(1));
            return w;
        };
        const auto shaped = [&](unsigned touched_mask, std::int32_t range_scale, bool equal_unscaled) {
            std::vector<Point> points;
            for (unsigned n = 0; n < points_per_zone; ++n) {
                Point p{};
                p.original_x = std::int32_t(n * 64) * range_scale; p.original_y = std::int32_t(n * 48) * range_scale;
                p.unscaled_x = std::int16_t(equal_unscaled ? 100 : std::int32_t(n * 37) - 200); p.unscaled_y = std::int16_t(equal_unscaled ? -5 : std::int32_t(n * 29) - 100);
                p.x = p.original_x + (n % 3 == 0 ? 17 : n % 3 == 1 ? -9 : 0); p.y = p.original_y + (n % 4 == 0 ? 11 : -3);
                p.touched = std::uint8_t((touched_mask >> (n % 16)) & 1 ? 3 : 0);
                points.push_back(p);
            }
            return points;
        };
        for (const unsigned opcode : {0x30u, 0x31u}) {
            for (const unsigned mask : {0x0000u, 0xffffu, 0x0001u, 0x0011u, 0x0421u, 0x8421u, 0x1111u, 0x0f0fu, 0x5555u, 0xaaaau, 0x8001u, 0x0210u, 0x4000u}) {
                for (const auto scale : {1, 40, 600}) {
                    auto w = iup_world(opcode, shaped(mask, scale, false), {0, 5, 10}, {4, 9, 15});
                    add(0x10757790, w, {{4, 0x11111111}, {5, 0x22222222}}, "IUP " + hex(opcode) + " touched " + hex(mask) + " scale " + std::to_string(scale));
                }
                auto v = iup_world(opcode, shaped(mask, 1, true), {0, 5, 10}, {4, 9, 15});
                add(0x10757790, v, {}, "IUP " + hex(opcode) + " touched " + hex(mask) + " equal unscaled coordinates");
            }
            for (unsigned n = 0; n < 40; ++n) {
                auto points = random_points(random, points_per_zone);
                for (auto& p : points) p.touched = std::uint8_t(random() % 4 == 0 ? 3 : random() % 4 == 0 ? 1 : random() % 3 == 0 ? 2 : 0);
                auto w = iup_world(opcode, points, {0, 5, 10}, {4, 9, 15});
                add(0x10757790, w, {}, "IUP " + hex(opcode) + " random " + std::to_string(n));
            }
            // Other contour layouts: one contour of all the points, single-point contours, no contours, a count past the arrays' data.
            auto points = shaped(0x2492, 3, false);
            for (const auto& layout : std::vector<std::pair<std::vector<std::int16_t>, std::vector<std::int16_t>>>{{{0}, {15}}, {{0, 1, 2}, {0, 1, 2}}, {{}, {}}, {{0, 3, 4, 8}, {2, 3, 7, 15}}, {{12}, {15}}}) {
                auto w = iup_world(opcode, points, layout.first, layout.second);
                add(0x10757790, w, {}, "IUP " + hex(opcode) + " contours " + std::to_string(layout.first.size()) + " first " + (layout.first.empty() ? "-" : std::to_string(layout.first[0])));
            }
        }
    }

    // ---- SHP, SHC, SHZ and the reference displacement helper ----
    {
        struct Direction { const char* name; std::int32_t px, py, fx, fy, dot; std::int32_t move, project; };
        const std::array<Direction, 4> directions = {{{"x", 0x4000, 0, 0x4000, 0, 0x4000, toc_move_x, toc_project_x}, {"y", 0, 0x4000, 0, 0x4000, 0x4000, toc_move_y, toc_project_y},
                                                       {"diagonal", 0x2d41, 0x2d41, 0x2d41, 0x2d41, 0x3fff, toc_move_any, toc_project_any}, {"free x", 0x2d41, 0x2d41, 0x4000, 0, 0x2d41, toc_move_any, toc_project_any}}};
        const auto shift_world = [&](const Direction& d, std::uint32_t opcode) {
            World w(scenario, hint.at);
            w.set_zone(0, random_points(random, points_per_zone), {0, 5, 10}, {4, 9, 15});
            auto points = random_points(random, points_per_zone);
            for (std::size_t n = 0; n < points.size(); ++n) { points[n].original_x = points[n].x - std::int32_t(n * 3) + 5; points[n].original_y = points[n].y + std::int32_t(n) - 2; }
            w.set_zone(1, points, {0, 5, 10}, {4, 9, 15});
            w.w16(w.context + 0x14, vec(d.px)); w.w16(w.context + 0x16, vec(d.py)); w.w16(w.context + 0x18, vec(d.fx)); w.w16(w.context + 0x1a, vec(d.fy));
            w.w16(w.context + 0x40, vec(d.dot));
            w.w32(w.context + 0x44, scenario.toc(d.move)); w.w32(w.context + 0x48, scenario.toc(d.project));
            w.w32(w.context + 4, opcode);
            w.w32(w.context + 0x34, 2); w.w32(w.context + 0x38, 5);
            return w;
        };
        for (const auto& d : directions) {
            for (const unsigned opcode : {0x32u, 0x33u})
                for (const unsigned loop : {0u, 1u, 3u, 0xffffu})
                    for (const unsigned same_zone : {0u, 1u}) {
                        auto w = shift_world(d, opcode);
                        w.w16(w.context + 0x3e, loop);
                        w.set_stack({7, 8, 9, 10});
                        if (same_zone) { w.w32(w.context + 0x08, w.zone_at(1)); w.w32(w.context + 0x0c, w.zone_at(1)); }
                        add(0x10757bd0, w, {{4, 0x12121212}, {5, 0x13131313}}, std::string("SHP ") + hex(opcode) + " " + d.name + " loop " + hex(loop) + (same_zone ? " zp0=zp1" : ""));
                    }
            for (const unsigned opcode : {0x34u, 0x35u})
                for (const unsigned contour : {0u, 1u, 2u, 3u, 0xffffffffu})
                    for (const unsigned zones_equal : {0u, 1u}) {
                        auto w = shift_world(d, opcode);
                        w.set_stack({4, contour});
                        w.w32(w.context + 0x10, w.zone_at(1));
                        if (zones_equal) { w.w32(w.context + 0x08, w.zone_at(1)); w.w32(w.context + 0x0c, w.zone_at(1)); w.w32(w.context + 0x34, 6); w.w32(w.context + 0x38, 6); }
                        // Otherwise the reference point is in the other zone: a point of the popped contour that is the reference point is still shifted.
                        else { w.w32(w.context + 0x08, w.zone_at(0)); w.w32(w.context + 0x0c, w.zone_at(0)); w.w32(w.context + 0x34, 2); w.w32(w.context + 0x38, 6); }
                        add(0x10757ca0, w, {}, std::string("SHC ") + hex(opcode) + " " + d.name + " contour " + hex(contour) + (zones_equal ? " reference in the zone" : ""));
                    }
            for (const unsigned opcode : {0x36u, 0x37u})
                for (const unsigned zone : {0u, 1u})
                    for (const unsigned reference : {0u, 7u, 15u}) {
                        auto w = shift_world(d, opcode);
                        w.set_stack({3, zone});
                        w.w32(w.context + 0x34, reference); w.w32(w.context + 0x38, reference);
                        add(0x10757dc0, w, {}, std::string("SHZ ") + hex(opcode) + " " + d.name + " zone " + std::to_string(zone) + " reference " + std::to_string(reference));
                    }
            for (const unsigned opcode : {0x32u, 0x33u}) {
                auto w = shift_world(d, opcode);
                add(0x10757ab0, w, {{4, slots_at}, {5, slots_at + 4}, {6, slots_at + 8}}, std::string("reference shift ") + hex(opcode) + " " + d.name);
            }
        }
        // The reference point equal to the point shifted (a zero displacement), and the far reference points.
        for (const unsigned reference : {0u, 15u}) {
            auto w = shift_world(directions[2], 0x32);
            w.w32(w.context + 0x38, reference);
            w.w32(w.context + 0x3e, 0);
            w.set_stack({reference});
            add(0x10757bd0, w, {}, "SHP reference " + std::to_string(reference));
        }
    }

    // ---- The loops and the entry: programs of opcodes run through the real opcode table (the other thirds of FNT run as the original) ----
    {
        const auto write_program = [&](World& w, const std::vector<std::uint8_t>& bytes) {
            for (std::size_t n = 0; n < bytes.size() + 4; ++n) w.w8(program_at + std::uint32_t(n), n < bytes.size() ? bytes[n] : 0x7b);
            return program_at + std::uint32_t(bytes.size());
        };
        const std::vector<std::vector<std::uint8_t>> programs = {
            {0x00, 0x01, 0x02, 0x05, 0x18, 0x19, 0x7a, 0x7d, 0x7c, 0x3d},
            {0xb4, 3, 1, 4, 1, 5, 0x20, 0x23, 0x24, 0x21, 0x22},
            {0xb4, 6, 0, 1, 0, 1, 0x0f},
            {0xb3, 1, 0, 3, 0, 0x06, 0x0c, 0x0d},
            {0xb1, 7, 9, 0x0a, 0x0b, 0x0e, 0x18},
            {0xb2, 4, 3, 1, 0x32, 0x30},
            {0xb1, 5, 0, 0x2f, 0x2e, 0x3e},
            {0xb2, 1, 2, 0x16, 0x13, 0x14, 0x15, 0x10, 0x11, 0x12, 0x17, 0x1a},
            {0x2d},                                    // ENDF outside a definition: the illegal opcode's longjmp
            {0x00, 0x7b, 0x01},                        // an undefined opcode
            {},                                        // nothing to run
        };
        const auto run_world = [&](const std::vector<std::uint8_t>& bytes) {
            auto w = make();
            const auto end = write_program(w, bytes);
            return std::pair{std::move(w), end};
        };
        for (std::size_t n = 0; n < programs.size(); ++n) {
            // The illegal opcode longjmps to the entry's setjmp: only the interpreter's own cases (below) have a jump buffer to land in.
            if (n != 8 && n != 9) {
                auto [w, end] = run_world(programs[n]);
                add(0x10756280, w, {{4, program_at}, {5, end}}, "run program " + std::to_string(n));
                auto [v, end_v] = run_world(programs[n]);
                v.w32(v.context + 0x2c, scenario.toc(toc_project_x));
                add(0x10756100, v, {{4, program_at}, {5, end_v}}, "run traced program " + std::to_string(n));
            }
            for (const unsigned traced : {0u, 1u}) {
                auto [x, end_x] = run_world(programs[n]);
                x.w16(x.global + 0xc, 16);
                x.w8(x.global + 0xc9, 1);
                // The trace word is the callback's descriptor (a harmless project function here: it only returns).
                add(0x10756310, x, {{3, x.zones}, {4, program_at}, {5, end_x}, {6, x.global}, {7, traced ? scenario.toc(toc_project_x) : 0u}}, "interpret program " + std::to_string(n) + (traced ? " traced" : ""));
            }
        }
        // No trace callback: the traced loop returns at once with the instruction pointer left at the start.
        {
            auto [w, end] = run_world(programs[0]);
            w.w32(w.context + 0x2c, 0);
            add(0x10756100, w, {{4, program_at}, {5, end}}, "run traced without a callback");
        }
        // The trace callback clears `trace` after the first opcode (the loop ends and puts the instruction pointer back); the callback's round index.
        for (const int round : {round_to_grid_slot, round_half_slot, round_double_slot, round_up_slot, round_down_slot, round_off_slot, round_super_slot, round_45_slot, 0x12345678}) {
            auto [w, end] = run_world(programs[0]);
            w.w32(w.context + 0x2c, scenario.toc(toc_project_x));
            w.w32(w.global + 0x70, round == 0x12345678 ? 0x12345678u : scenario.toc(round));
            // The callback is a descriptor of the scratch area for the (answered) reporter: a poke on it can clear the trace word (a routine of this
            // block would be under check and take no poke); the loop then ends after the first opcode and puts the instruction pointer back.
            w.w32(slots_at, 0x107755c0); w.w32(slots_at + 4, game_toc); w.w32(w.context + 0x2c, slots_at);
            auto& item = add(0x10756100, w, {{4, program_at}, {5, end}}, "run traced, round function " + hex(std::uint32_t(round)));
            item.answers = {{0x107755c0, 0}};
            if (round != round_to_grid_slot) item.pokes = {{0x107755c0, {w32(context_at + 0x2c, 0)}}};
        }
        // The entry's own paths: the current program flag, the size checks, the table check, the scale callback.
        const auto entry_world = [&](unsigned program, unsigned ppem, int mode, unsigned c8, unsigned scale_input, std::uint32_t ratio_down, std::uint32_t ratio_up) {
            auto [w, end] = run_world(programs[0]);
            w.w8(w.global + 0xc9, program);
            w.w16(w.global + 0xc, ppem);
            w.w8(w.global + 0xb6, std::uint32_t(mode));
            w.w8(w.global + 0xc8, c8);
            w.w16(w.global + 0x88, scale_input);
            w.w32(w.global + 0xb8, ratio_down); w.w32(w.global + 0xbc, ratio_up);
            w.w32(w.global + 0x98, scenario.toc(-0x3c24)); w.w32(w.global + 0xa8, 0x10); w.w32(w.global + 0xb0, 0x20); w.w16(w.global + 0xb4, 4);
            return std::pair{std::move(w), end};
        };
        for (const unsigned ppem : {16u, 2u, 1u, 0u})
            for (const int mode : {1, 0, -1})
                for (const unsigned c8 : {0u, 1u})
                    for (const std::uint32_t ratio : {0x10000u, 0x20000u, 0x8000u})
                        for (const unsigned scale_input : {12u, 0u}) {
                            auto [w, end] = entry_world(0, ppem, mode, c8, scale_input, ratio, 0x10000);
                            add(0x10756310, w, {{3, w.zones}, {4, program_at}, {5, end}, {6, w.global}, {7, 0}},
                                "interpret prep: ppem " + std::to_string(ppem) + " mode " + std::to_string(mode) + " c8 " + std::to_string(c8) + " down ratio " + hex(ratio) + " scale input " + std::to_string(scale_input));
                        }
        for (const std::uint32_t second : {0x10000u, 0x20000u, 0x1u}) {
            auto [w, end] = entry_world(0, 16, 0, 0, 0, 0x30000, second);
            add(0x10756310, w, {{3, w.zones}, {4, program_at}, {5, end}, {6, w.global}, {7, 0}}, "interpret prep: up ratio " + hex(second));
        }
        // The table check: the first descriptor's low 24 bits against the TOC's (the table of the snapshot is the installer's), and the constant at +0x40c.
        {
            const auto table = scenario.r32(scenario.r32(hint.at + 0x18 + 0x8c) ? hint.at + 0x18 + 0x8c : hint.at);
            for (const unsigned corruption : {0u, 1u, 2u}) {
                auto [w, end] = run_world(programs[0]);
                w.w8(w.global + 0xc9, 1);
                w.w32(w.global + 0x8c, slots_at);
                for (std::uint32_t at = 0; at < 0x4c8; at += 4) w.w32(slots_at + at, scenario.r32(table + at));
                if (corruption == 1) w.w32(slots_at + 0x40c, 0x2d413cce);
                if (corruption == 2) w.w32(slots_at + 0x40c, 0);
                add(0x10756310, w, {{3, w.zones}, {4, program_at}, {5, end}, {6, w.global}, {7, 0}}, "interpret with the table's constant " + std::to_string(corruption));
            }
            for (const unsigned c8 : {1u}) {
                auto [w, end] = entry_world(0, 16, 1, c8, 0, 0x10000, 0x10000);
                w.w32(w.global + 0x8c, slots_at);
                for (std::uint32_t at = 0; at < 0x4c8; at += 4) w.w32(slots_at + at, scenario.r32(table + at));
                w.w32(slots_at, 0x10000000);
                add(0x10756310, w, {{3, w.zones}, {4, program_at}, {5, end}, {6, w.global}, {7, 0}}, "interpret with the first table entry wrong, c8 " + std::to_string(c8));
            }
        }
        // Random programs over the opcodes of this block: eight pushed values, then ten opcodes.
        const std::array<std::uint8_t, 46> ops = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x0c, 0x0d, 0x0e, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x3d, 0x7a, 0x7c, 0x7d, 0x10, 0x11, 0x12,
                                                   0x1a, 0x1d, 0x1e, 0x30, 0x31, 0x2e, 0x2f, 0x3e, 0x3f, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x0a, 0x0b, 0x06, 0x07, 0x08, 0x09, 0x2d, 0x7b};
        for (unsigned n = 0; n < 30; ++n) {
            std::vector<std::uint8_t> bytes = {0xb7};
            for (unsigned k = 0; k < 8; ++k) bytes.push_back(std::uint8_t(random() % 12));
            for (unsigned k = 0; k < 10; ++k) bytes.push_back(ops[random() % ops.size()]);
            auto [w, end] = run_world(bytes);
            w.w8(w.global + 0xc9, 1);
            add(0x10756310, w, {{3, w.zones}, {4, program_at}, {5, end}, {6, w.global}, {7, 0}}, "interpret random program " + std::to_string(n) + [&] { std::string text = " ops"; for (const auto byte : bytes) text += " " + hex(byte); return text; }());
        }
    }
    // The call trace pairs the first cases of an entry only: the interpreter's rarer call sites (the scale callback, the table check's longjmp, the traced
    // loop, the answered illegal opcode, the single width callback) go first.
    const auto rank = [](const Case& item) {
        const auto has = [&](const char* text) { return item.label.find(text) != std::string::npos; };
        if (has("table")) return 0;
        if (has("illegal") || has("single width value")) return 1;
        if (has("interpret prep: ppem 16")) return 2;
        if (has("interpret prep")) return 3;
        if (has("traced")) return 4;
        return 5;
    };
    std::stable_sort(cases.begin() + std::ptrdiff_t(first_case), cases.end(), [&](const Case& a, const Case& b) { return rank(a) < rank(b); });
}
}
