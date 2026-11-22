// The font engine, block `t2ksc` (0x10761270..0x10762f40): cases for the scan converter's nine routines. The game runs them for every glyph (the
// end-to-end run drives 60,480 requests through 0x107620d0, 0x107623c0 and 0x10762e30 and, below them, the rest); these cases add what a run does
// not reach: the thresholds of the arithmetic (extents at 0xb504 on both sides, spans of 0x3f/0x40, row counts at 8, 9, 0x20 and 0x21), every
// start/end combination of on-curve and off-curve points, depth limits, dropout control in both directions, grey and one-bit bitmaps, lists the
// assertions catch, chunk tables at their growth, and the converter's release in every branch.
//
// States. The converter object is a block the original's constructor builds: a case that needs one has a `prepare` that runs the ORIGINAL
// constructor (0x107620d0, from the outline the case wrote into the scratch area) inside each compared run, on both sides, and then pokes the
// fields it wants changed; the routine under check runs from that state. A prepared case is compared and traced, not mutated; the constructor's own
// cases (no prepare) are mutated. Labels start with "t2ksc:" (CM_FONT_ENGINE_BLOCK=t2ksc).
#include "font_engine_adapter.hpp"
#include "font_engine_checks.hpp"
#include <algorithm>
#include <array>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
[[maybe_unused]] constexpr std::uint32_t constructor = 0x107620d0, render = 0x107623c0, destructor = 0x10762e30, contours_entry = 0x10761ec0, curve_entry = 0x10761d50,
                        line_entry = 0x107617b0, chunk_entry = 0x10761270, bounds_entry = 0x107614c0, extents_entry = 0x10761730;
[[maybe_unused]] constexpr std::uint32_t game_malloc = 0x10775130, game_free = 0x10775170, game_realloc = 0x107751b0, reporter = 0x107755c0;
// The outline arrays and the scalars of the constructor's stack arguments live in the scratch area.
[[maybe_unused]] constexpr std::uint32_t area = scratch_base + 0x40000, starts_at = area, ends_at = area + 0x2000, xs_at = area + 0x4000, ys_at = area + 0x8000, on_at = area + 0xc000,
                        words_at = area + 0x10000;
// The converter's fields the cases poke.
[[maybe_unused]] constexpr std::uint32_t f_bitmap = 0x1c, f_bounds_known = 0x20, f_large = 0x44, f_cursor = 0x168, f_limit = 0x16c, f_chunk_table = 0x21f0, f_capacity = 0x21f4,
                        f_count = 0x21f8, f_depth = 0x21fc, f_columns = 0x2203, f_mem = 0x2224, f_row_heads = 0xc8, f_first_row = 0xd0, f_last_row = 0xd4,
                        f_column_heads = 0x158, f_first_column = 0x160, f_last_column = 0x164;

struct Point { int x, y; bool on = true; };
using Contour = std::vector<Point>;
using Outline = std::vector<Contour>;
// The constructor's modes: the grey bitmap, the two dropout controls, `skip` = 3 leaves the contours unwalked, the curve depth limit.
struct Mode {
    bool grey = false, rows = false, columns = false;
    int skip = 0, depth = 4;
};

std::uint32_t memory_object(const Scenario& scenario) {
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x17c <= std::uint64_t(low) + size; at += 4)
            if (scenario.r32(std::uint32_t(at)) == 0xab1500ff && scenario.r32(std::uint32_t(at) + 0x178) == 0xa5a55a5a && scenario.r32(std::uint32_t(at) + 8) == 0x1f5)
                return std::uint32_t(at);
    }
    return 0;
}

// The outline as writes: halfword start and end indices, word coordinates, on-curve bytes; the count of contours.
struct Laid {
    std::vector<Write> memory;
    std::uint32_t count = 0;
};
Laid lay(const Outline& outline) {
    Laid laid;
    std::uint32_t index = 0;
    for (const auto& contour : outline) {
        if (contour.empty()) continue;
        laid.memory.push_back(w16(starts_at + 2 * laid.count, index));
        laid.memory.push_back(w16(ends_at + 2 * laid.count, index + std::uint32_t(contour.size()) - 1));
        for (const auto& point : contour) {
            laid.memory.push_back(w32(xs_at + 4 * index, std::uint32_t(point.x)));
            laid.memory.push_back(w32(ys_at + 4 * index, std::uint32_t(point.y)));
            laid.memory.push_back(w8(on_at + index, point.on ? 1 : 0));
            ++index;
        }
        ++laid.count;
    }
    return laid;
}
// The constructor's stack arguments: the caller's words at sp + 0x38 (low bytes read), 0x3c, 0x40 and the depth word at 0x44.
std::vector<Write> stack_arguments(const Scenario& scenario, const Mode& mode) {
    return {w32(scenario.sp() + 0x38, std::uint32_t(mode.skip)), w32(scenario.sp() + 0x3c, mode.rows), w32(scenario.sp() + 0x40, mode.columns), w32(scenario.sp() + 0x44, std::uint32_t(mode.depth))};
}
std::vector<Write> join(std::vector<Write> a, const std::vector<Write>& b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

// Runs the original constructor on the run's own state (the outline is in memory, the stack words written here) and returns the converter; the
// case's registers are put back (the caller sets what the routine under check reads).
std::uint32_t build(State& s, std::uint32_t mem, std::uint32_t count, const Mode& mode) {
    guest::Cpu c(s);
    const auto saved = s.registers;
    const auto sp = c.sp();
    c.put32(sp + 0x38, std::uint32_t(mode.skip));
    c.put32(sp + 0x3c, mode.rows);
    c.put32(sp + 0x40, mode.columns);
    c.put32(sp + 0x44, std::uint32_t(mode.depth));
    c.set({{3, mem}, {4, count}, {5, starts_at}, {6, ends_at}, {7, xs_at}, {8, ys_at}, {9, on_at}, {10, mode.grey ? 1u : 0u}});
    font_engine_run_original(s, constructor);
    const auto sc = c.r(3);
    s.registers = saved;
    return sc;
}

// Heap and scratch memory dirtied before a call (the zero fills show only against dirty memory): a block of `size` bytes from the original's allocator,
// every byte 0xaa, given back (the next allocation of that size gets it) or, with `slot`, made the memory object's scratch buffer of that slot, free.
void dirty_block(guest::Cpu& c, std::uint32_t mem, std::uint32_t size, int slot) {
    c.set({{3, mem}, {4, size}});
    font_engine_run_original(c.s, 0x107678a0);
    const auto block = c.r(3);
    for (std::uint32_t n = 0; n < size; n += 4) c.put32(block + n, 0xaaaaaaaau);
    if (slot >= 0) {
        c.put32(mem + 0x128 + 4 * std::uint32_t(slot), block);
        c.put32(mem + 0x140 + 4 * std::uint32_t(slot), size);
        c.put32(mem + 0x158 + 4 * std::uint32_t(slot), 1);
    } else {
        c.set({{3, mem}, {4, block}});
        font_engine_run_original(c.s, 0x10767bb0);
    }
}

// ---- Shapes (26.6 coordinates, y up) ----
Contour rectangle(int x0, int y0, int x1, int y1, bool clockwise = false) {
    Contour contour{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
    if (clockwise) std::reverse(contour.begin(), contour.end());
    return contour;
}
// An ellipse as quadratic arcs: four on-curve extremes and the four corner control points; `all_off` makes every point off the curve (the midpoints are implied).
Contour ellipse(int cx, int cy, int rx, int ry, bool all_off = false, unsigned rotate = 0, bool start_on_curve_last = false) {
    Contour contour;
    const int ex = rx, ey = ry;
    const std::array<Point, 8> ring = {{{cx + ex, cy, true}, {cx + ex, cy + ey, false}, {cx, cy + ey, true}, {cx - ex, cy + ey, false}, {cx - ex, cy, true},
                                         {cx - ex, cy - ey, false}, {cx, cy - ey, true}, {cx + ex, cy - ey, false}}};
    for (const auto& point : ring) {
        auto p = point;
        if (all_off) p.on = false;
        contour.push_back(p);
    }
    std::rotate(contour.begin(), contour.begin() + rotate % contour.size(), contour.end());
    if (start_on_curve_last) std::reverse(contour.begin(), contour.end());
    return contour;
}
Outline random_outline(std::mt19937& random, int range, unsigned max_points, bool curvy) {
    Outline outline(1 + random() % 3);
    for (auto& contour : outline) {
        contour.resize(1 + random() % max_points);
        for (auto& point : contour) {
            point.x = int(random() % (2 * range + 1)) - range;
            point.y = int(random() % (2 * range + 1)) - range;
            point.on = !curvy || random() % 3 != 0;
        }
    }
    return outline;
}
}

FONT_ENGINE_CASES("t2ksc") {
    const auto mem = memory_object(scenario);
    if (!mem) return;
    std::mt19937 random(0x72c5c);
    const auto first_case = cases.size();
    const auto note = [](const std::string& text) { return "t2ksc: " + text; };
    const std::vector<std::uint32_t> owners = {game_malloc, game_free, game_realloc};
    // 0x107620d0: the converter for an outline, in every mode.
    const auto construct = [&](const Outline& outline, Mode mode, const std::string& label) {
        const auto laid = lay(outline);
        cases.push_back({.entry = constructor,
                         .registers = {{3, mem}, {4, laid.count}, {5, starts_at}, {6, ends_at}, {7, xs_at}, {8, ys_at}, {9, on_at}, {10, mode.grey ? 1u : 0u}},
                         .memory = join(laid.memory, stack_arguments(scenario, mode)),
                         .label = note("construct " + label)});
        cases.back().original_owners = owners;
    };
    // The constructor's case with the heap dirtied for the list tables it will allocate (one block of each size given, freed again first).
    const auto lists_of = [](const Outline& outline) {
        int ymin = 0x7fffffff, ymax = -0x7fffffff - 1, xmin = 0x7fffffff, xmax = -0x7fffffff - 1;
        for (const auto& contour : outline) for (const auto& point : contour) { ymin = std::min(ymin, point.y); ymax = std::max(ymax, point.y); xmin = std::min(xmin, point.x); xmax = std::max(xmax, point.x); }
        const int rows = ((ymax + 0x40) >> 6) - ((ymin - 0x40) >> 6) + 1, columns = ((xmax + 0x40) >> 6) - ((xmin - 0x40) >> 6) + 1;
        return std::pair<int, int>{rows, columns};
    };
    const auto construct_dirty = [&](const Outline& outline, Mode mode, const std::string& label) {
        const auto laid = lay(outline);
        const auto [rows, columns] = lists_of(outline);
        Case item{.entry = constructor,
                  .registers = {{3, mem}, {4, laid.count}, {5, starts_at}, {6, ends_at}, {7, xs_at}, {8, ys_at}, {9, on_at}, {10, mode.grey ? 1u : 0u}},
                  .memory = join(laid.memory, stack_arguments(scenario, mode)),
                  .label = note("construct dirty heap " + label)};
        item.prepare = [=](State& s) {
            guest::Cpu c(s);
            const auto saved = s.registers;
            for (const int count : {rows, columns}) if (count > 0x20 && count < 0x2000) dirty_block(c, mem, std::uint32_t(count) * 4, -1);
            s.registers = saved;
        };
        item.original_owners = owners;
        cases.push_back(std::move(item));
    };
    // A routine under check from a state the original constructor built (see the head of the file); `setup` sets the registers and pokes the fields.
    using Setup = std::function<void(guest::Cpu&, std::uint32_t)>;
    const auto prepared = [&](std::uint32_t entry, const Outline& outline, Mode mode, Setup setup, const std::string& label, std::vector<Write> extra = {}) -> Case& {
        const auto laid = lay(outline);
        Case item{.entry = entry, .memory = join(laid.memory, extra), .label = note(label)};
        item.prepare = [=](State& s) {
            try {
                const auto sc = build(s, mem, laid.count, mode);
                guest::Cpu c(s);
                c.set({{9, 0x99990000u}, {10, 0xaaaa0000u}});
                setup(c, sc);
            } catch (const std::exception& error) {
                std::fprintf(stderr, "t2ksc: the preparation failed: %s\n", error.what());
                throw;
            }
        };
        item.answers = {{reporter, 0}};
        item.original_owners = owners;
        cases.push_back(std::move(item));
        return cases.back();
    };
    // The constructor's case and the bitmap of the same outline (the converter built by the original, 0x107623c0 run from there).
    const auto both = [&](const Outline& outline, Mode mode, const std::string& label) {
        construct(outline, mode, label);
        prepared(render, outline, mode, [](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {5, 0}}); }, "render " + label);
        prepared(render, outline, mode, [mem](guest::Cpu& c, std::uint32_t sc) { dirty_block(c, mem, 0x8000, 5); c.set({{3, sc}, {5, 0}, {9, 0x99990000u}, {10, 0xaaaa0000u}}); }, "render into a dirty buffer " + label);
    };
    const auto name_of = [](Mode mode) {
        return std::string(mode.grey ? "grey " : "mono ") + (mode.rows ? "rows " : "") + (mode.columns ? "columns " : "") + (mode.skip ? "skip " : "") + "depth " + std::to_string(mode.depth);
    };

    const Outline square{rectangle(0, 0, 640, 640)};
    const Outline circle{ellipse(320, 320, 300, 300)};
    std::vector<Mode> modes;
    for (const bool grey : {false, true}) for (const bool rows : {false, true}) for (const bool columns : {false, true}) modes.push_back({grey, rows, columns, 0, 4});
    for (const auto& mode : modes) {
        construct(square, mode, "square " + name_of(mode));
        construct(circle, mode, "circle " + name_of(mode));
        auto skipped = mode;
        skipped.skip = 3;
        construct(square, skipped, "square " + name_of(skipped));
    }
    for (const int depth : {-5, 0, 1, 3, 4, 5, 6, 8, 15, 16, 17, 100, 0x7fffffff, int(0x80000000)})
        construct(circle, {false, false, false, 0, depth}, "circle depth " + std::to_string(depth));

    // Row and column counts around the unrolled zeroing (8, 9), the inline slots (0x20, 0x21) and a few more: a rectangle from the origin whose far
    // edge is k pixels (and k pixels less one unit, plus one unit) away; the column lists exist for the grey bitmap.
    for (const int k : {0, 1, 2, 5, 6, 7, 8, 9, 10, 13, 14, 29, 30, 31, 40}) for (const int unit : {0, -1, 1}) {
        const auto edge = 64 * k + unit;
        both({rectangle(0, 0, 640, edge)}, {false, false, false, 0, 4}, "rows k " + std::to_string(k) + (unit ? (unit < 0 ? " -1" : " +1") : ""));
        construct_dirty({rectangle(0, 0, 640, edge)}, {false, false, false, 0, 4}, "rows k " + std::to_string(k) + (unit ? (unit < 0 ? " -1" : " +1") : ""));
        construct_dirty({rectangle(0, 0, edge, 640)}, {true, false, false, 0, 4}, "columns k " + std::to_string(k) + (unit ? (unit < 0 ? " -1" : " +1") : ""));
        both({rectangle(0, 0, edge, 640)}, {true, false, false, 0, 4}, "columns k " + std::to_string(k) + (unit ? (unit < 0 ? " -1" : " +1") : ""));
        both({rectangle(0, 0, edge, 640)}, {false, false, true, 0, 4}, "dropout columns k " + std::to_string(k) + (unit ? (unit < 0 ? " -1" : " +1") : ""));
    }
    // Rectangles at fractional positions, every combination of the centre line's neighbourhood (a crossing exactly at, just before and after a centre).
    for (const int x : {0, 1, 0x1f, 0x20, 0x21, 0x3f, 0x40, 0x41, -1, -0x20, -0x21, -0x40}) for (const int w : {1, 0x1f, 0x20, 0x21, 0x3f, 0x40, 0x41, 0x7f, 0x80, 0x81, 200})
        for (const bool grey : {false, true}) {
            both({rectangle(x, 0, x + w, 640)}, {grey, true, !grey, 0, 4}, std::string(grey ? "grey " : "mono ") + "thin x " + std::to_string(x) + " w " + std::to_string(w));
            both({rectangle(0, x, 640, x + w, true)}, {grey, true, true, 0, 4}, std::string(grey ? "grey " : "mono ") + "thin y " + std::to_string(x) + " h " + std::to_string(w));
        }
    // Thin slanted features (the crossings take every parity: the odd end points of the straight edges never do): parallelograms a few units wide.
    for (const int w : {1, 3, 9, 20, 40, 63}) for (const int slope : {1, 3, 7, 30, -5, -29, 100}) for (const int x : {0, 59, 64 * 3 - 5, 64 * 3 - 4}) {
        const std::string name = "w " + std::to_string(w) + " slope " + std::to_string(slope) + " x " + std::to_string(x);
        both({{{x + 100, 0}, {x + 100 + w, 0}, {x + 100 + w + slope, 640}, {x + 100 + slope, 640}}}, {false, true, true, 0, 4}, "thin slanted " + name);
        both({{{0, x + 100}, {0, x + 100 + w}, {640, x + 100 + w + slope}, {640, x + 100 + slope}}}, {false, true, true, 0, 4}, "thin slanted y " + name);
    }
    // A feature a few units wide whose crossings walk across a pixel boundary one unit a row (the dropout pixel is the middle of the pair: the rounding of
    // its sum decides which side of the boundary it falls).
    for (const int base : {118, 120, 122, 124, 125, 126, 127}) for (const int slope : {8, 10, 12, -10}) for (const int w : {1, 2, 3, 4, 5, 6, 7}) for (const int wedge : {0, 1, -1, 3}) {
        const std::string name = "base " + std::to_string(base) + " slope " + std::to_string(slope) + " w " + std::to_string(w) + " wedge " + std::to_string(wedge);
        both({{{base, 0}, {base + w, 0}, {base + w + slope + wedge, 640}, {base + slope, 640}}}, {false, true, true, 0, 4}, "boundary walk " + name);
        both({{{0, base}, {0, base + w}, {640, base + w + slope + wedge}, {640, base + slope}}}, {false, true, true, 0, 4}, "boundary walk y " + name);
    }
    // Curves: every rotation of the eight points (start on or off the curve, last point on or off), all off the curve, either direction, radii.
    for (unsigned rotate = 0; rotate < 8; ++rotate) for (const bool all_off : {false, true}) for (const bool reversed : {false, true})
        for (const auto [rx, ry] : std::array<std::pair<int, int>, 4>{{{300, 300}, {20, 500}, {3, 3}, {1500, 40}}}) {
            const Outline outline{ellipse(400, 400, rx, ry, all_off, rotate, reversed)};
            for (const bool grey : {false, true}) both(outline, {grey, true, true, 0, 4}, "ellipse r" + std::to_string(rotate) + (all_off ? " off" : "") + (reversed ? " rev" : "") + " " + std::to_string(rx) + "x" +
                                                                                             std::to_string(ry) + (grey ? " grey" : ""));
        }
    // Contours of few points (a one-point, a two-point, a three-point contour of every on/off pattern) and several contours.
    for (unsigned pattern = 0; pattern < 8; ++pattern) for (const unsigned points : {1u, 2u, 3u}) {
        Contour contour;
        for (unsigned n = 0; n < points; ++n) contour.push_back({100 + 70 * int(n * n), 90 + 130 * int(n), ((pattern >> n) & 1) != 0});
        both({contour}, {true, true, true, 0, 4}, "points " + std::to_string(points) + " pattern " + std::to_string(pattern));
    }
    both({rectangle(0, 0, 640, 640), rectangle(100, 100, 300, 300, true), rectangle(400, 100, 500, 500, true), ellipse(450, 450, 30, 30)}, {true, true, true, 0, 4}, "four contours");
    both({}, {false, false, false, 0, 4}, "no contours");
    // Large extents: the fixed-point interpolation (a span at or past 0xb504) and the plain one, on either side of the limit, both slopes.
    for (const int size : {0xb503, 0xb504, 0xb505, 0xc000, 0x12000}) for (const bool grey : {false, true}) {
        both({rectangle(0, 0, size, 640)}, {grey, true, true, 0, 4}, "wide " + hex(unsigned(size)) + (grey ? " grey" : ""));
        both({rectangle(0, 0, 640, size)}, {grey, true, true, 0, 4}, "tall " + hex(unsigned(size)) + (grey ? " grey" : ""));
        both({{{0, 0}, {size, 100}, {size / 2, size / 3}}}, {grey, true, true, 0, 4}, "triangle " + hex(unsigned(size)) + (grey ? " grey" : ""));
        both({{{0, size}, {size, 0}, {100, 200}}}, {grey, false, true, 0, 4}, "falling " + hex(unsigned(size)) + (grey ? " grey" : ""));
        both({{{0, 0}, {-size, 100}, {size / 2, -size / 3}}}, {grey, true, true, 0, 4}, "negative " + hex(unsigned(size)) + (grey ? " grey" : ""));
    }
    both({ellipse(0x6000, 0x6000, 0x5000, 0x5000)}, {true, true, true, 0, 4}, "large ellipse");
    both({ellipse(0x6000, 0x6000, 0x5000, 0x5000)}, {false, true, true, 0, 16}, "large ellipse depth 16");
    // Random outlines (straight and curved), small and glyph-sized, mostly with every dropout control on.
    for (unsigned n = 0; n < 200; ++n) {
        const int range = random() % 3 == 0 ? 100 : random() % 2 ? 1200 : 9000;
        const Mode mode{random() % 2 != 0, random() % 2 != 0, random() % 2 != 0, 0, int(random() % 3 == 0 ? random() % 20 : 4)};
        both(random_outline(random, range, 12, random() % 2 != 0), mode, "random " + std::to_string(n) + " " + name_of(mode));
    }

    // ---- Prepared cases: the converter built by the original, the routine under check run from there ----
    const std::vector<std::pair<std::string, Outline>> shapes = [&] {
        std::vector<std::pair<std::string, Outline>> list;
        list.push_back({"square", square});
        list.push_back({"circle", circle});
        list.push_back({"ring", {rectangle(0, 0, 640, 640), rectangle(160, 160, 480, 480, true)}});
        list.push_back({"thin", {rectangle(100, 0, 130, 700), rectangle(0, 300, 700, 340, true)}});
        for (unsigned rotate = 0; rotate < 8; rotate += 3) list.push_back({"ellipse " + std::to_string(rotate), {ellipse(400, 400, 350, 120, false, rotate)}});
        list.push_back({"ellipse off", {ellipse(400, 400, 350, 350, true, 3)}});
        list.push_back({"two off", {{{100, 100, false}, {400, 100, false}}}});
        list.push_back({"one off", {{{100, 100, false}}}});
        list.push_back({"triangle off start", {{{100, 100, false}, {500, 100, true}, {300, 500, true}}}});
        list.push_back({"triangle off end", {{{100, 100, true}, {500, 100, true}, {300, 500, false}}}});
        for (unsigned n = 0; n < 6; ++n) list.push_back({"random " + std::to_string(n), random_outline(random, 1500, 10, n % 2 != 0)});
        return list;
    }();

    // 0x10761ec0: the contours of the outline walked (the converter built with the contours skipped).
    for (const auto& [name, outline] : shapes) for (const bool grey : {false, true}) {
        const Mode mode{grey, true, true, 3, 4};
        prepared(contours_entry, outline, mode, [](guest::Cpu& c, std::uint32_t sc) { c.set(3, sc); }, "contours " + name + (grey ? " grey" : " mono"));
        const Mode deep{grey, false, false, 3, 12};
        prepared(contours_entry, outline, deep, [](guest::Cpu& c, std::uint32_t sc) { c.set(3, sc); }, "contours " + name + " depth 12" + (grey ? " grey" : " mono"));
    }
    // A contour that starts after it ends, a contour count past the arrays, an index at the edges.
    for (const int count : {0, 1, 2, 3}) {
        prepared(contours_entry, {rectangle(0, 0, 640, 640), ellipse(300, 300, 50, 50)}, {true, true, true, 3, 4},
                 [count](guest::Cpu& c, std::uint32_t sc) { c.put16(sc + 0x2214, std::uint32_t(count)); c.set(3, sc); }, "contours count field " + std::to_string(count));
    }
    prepared(contours_entry, {rectangle(0, 0, 640, 640)}, {true, true, true, 3, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.put16(c.u32(sc + 0x220c), 3); c.put16(c.u32(sc + 0x2210), 1); c.set(3, sc); }, "contours start after end");
    prepared(contours_entry, {rectangle(0, 0, 640, 640)}, {true, true, true, 3, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.put16(c.u32(sc + 0x220c), 0xffff); c.set(3, sc); }, "contours start -1");
    prepared(contours_entry, {rectangle(0, 0, 640, 640)}, {true, true, true, 3, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.put16(c.u32(sc + 0x2210), 0xffff); c.set(3, sc); }, "contours end -1");

    // 0x10761d50: a curve (26.6) into the lists of a converter built for a box: random controls, degenerate curves, depth limits and large boxes.
    for (const int box : {640, 6400, 0xc000}) for (const int depth : {4, 6, 16}) {
        const Outline outline{rectangle(0, 0, box, box)};
        for (unsigned n = 0; n < (box == 640 ? 12u : 8u); ++n) {
            std::array<std::uint32_t, 6> v{};
            for (auto& value : v) value = std::uint32_t(random() % (box + 1));
            const Mode mode{n % 2 != 0, true, n % 3 == 0, 3, depth};
            prepared(curve_entry, outline, mode, [v](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {4, v[0]}, {5, v[1]}, {6, v[2]}, {7, v[3]}, {8, v[4]}, {9, v[5]}}); },
                     "curve box " + std::to_string(box) + " depth " + std::to_string(depth) + " random " + std::to_string(n));
        }
    }
    for (const int depth : {4, 5, 16}) {
        const Mode mode{true, true, true, 3, depth};
        const std::vector<std::array<std::uint32_t, 6>> special = {{300, 300, 300, 300, 300, 300}, {0, 0, 320, 320, 640, 640}, {0, 0, 640, 0, 640, 640}, {100, 500, 100, 100, 100, 500},
                                                                  {0, 640, 0, 0, 0, 640}, {640, 0, 0, 640, 640, 640}, {1, 1, 1, 639, 639, 639}, {0, 0, 640, 640, 0, 0}};
        for (std::size_t n = 0; n < special.size(); ++n)
            prepared(curve_entry, {rectangle(0, 0, 640, 640)}, mode, [v = special[n]](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {4, v[0]}, {5, v[1]}, {6, v[2]}, {7, v[3]}, {8, v[4]}, {9, v[5]}}); },
                     "curve special " + std::to_string(n) + " depth " + std::to_string(depth));
    }

    // 0x107617b0: a line into the lists: every direction, the parity of the end points (made odd), vertical and horizontal lines, large extents.
    for (const int box : {640, 6400, 0xc000, 0x16000}) for (const bool grey : {false, true}) {
        const Outline outline{rectangle(0, 0, box, box)};
        const Mode mode{grey, true, !grey, 3, 4};
        for (unsigned n = 0; n < (box == 640 ? 16u : 8u); ++n) {
            std::array<std::uint32_t, 4> v{};
            for (auto& value : v) value = std::uint32_t(random() % (box + 1));
            if (n % 4 == 1) v[2] = v[0];
            if (n % 4 == 2) v[3] = v[1];
            if (n % 8 == 3) v = {v[0] & ~1u, v[1] | 1u, v[2] & ~1u, v[3] | 1u};
            prepared(line_entry, outline, mode, [v](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {4, v[0]}, {5, v[1]}, {6, v[2]}, {7, v[3]}}); },
                     "line box " + std::to_string(box) + (grey ? " grey " : " mono ") + std::to_string(n));
        }
    }
    {   // The rows and columns of the first and the last pixel, and a line that runs the whole box in each direction (large and small).
        const std::vector<std::array<std::uint32_t, 4>> special = {{0, 0, 640, 640}, {640, 640, 0, 0}, {0, 640, 640, 0}, {640, 0, 0, 640}, {0, 0, 0, 640}, {0, 0, 640, 0}, {320, 0, 320, 640},
                                                                  {0, 320, 640, 320}, {31, 31, 33, 33}, {32, 32, 32, 32}, {1, 1, 1, 1}, {0x1f, 0x20, 0x21, 0x20}, {0x20, 0x1f, 0x20, 0x21}, {639, 639, 640, 640}};
        for (const bool grey : {false, true}) for (std::size_t n = 0; n < special.size(); ++n)
            prepared(line_entry, {rectangle(0, 0, 640, 640)}, {grey, false, !grey, 3, 4},
                     [v = special[n]](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {4, v[0]}, {5, v[1]}, {6, v[2]}, {7, v[3]}}); }, std::string("line special ") + (grey ? "grey " : "mono ") + std::to_string(n));
    }
    // A line near the top of the 32-bit range: its first centre wraps (the converter's lists are indexed far outside: both sides fault alike).
    for (const std::uint32_t x : {0x7fffffffu, 0x7ffffff0u, 0x7fffffe0u, 0x80000000u})
        prepared(line_entry, {rectangle(0, 0, 640, 640)}, {true, false, false, 3, 4}, [x](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {4, x}, {5, 100}, {6, x}, {7, 200}}); }, "line near the top of the range " + hex(x));

    // 0x10761270: the next chunk: the first one, the following ones, the assertion (the cursor short of the limit), the table's growth into the allocator and by
    // reallocation, and the extents (the chunk's size) at the edges.
    {
        const Outline outline{rectangle(0, 0, 640, 640)};
        const Mode mode{true, true, true, 3, 4};
        const auto at = [](guest::Cpu& c, std::uint32_t sc) { c.set(3, sc); };
        prepared(chunk_entry, outline, mode, [](guest::Cpu& c, std::uint32_t sc) { c.put32(sc + f_capacity, 0); c.set(3, sc); }, "chunk first");
        prepared(chunk_entry, outline, mode, at, "chunk next with the cursor short of the limit (assertion)");
        prepared(chunk_entry, outline, mode, [](guest::Cpu& c, std::uint32_t sc) { c.put32(sc + f_cursor, c.u32(sc + f_limit)); c.set(3, sc); }, "chunk next");
        for (const std::uint32_t count : {1u, 5u, 0x1eu, 0x1fu, 0x20u, 0x21u}) {
            prepared(chunk_entry, outline, mode, [count](guest::Cpu& c, std::uint32_t sc) { c.put32(sc + f_cursor, c.u32(sc + f_limit)); c.put32(sc + f_count, count); c.set(3, sc); },
                     "chunk count " + std::to_string(count) + " inline table");
            prepared(chunk_entry, outline, mode, [count, mem](guest::Cpu& c, std::uint32_t sc) {
                // The table already in the allocator: a block of the right size from the allocator (the original's), the old entries copied as far as they go.
                c.set({{3, mem}, {4, 0x100}});
                font_engine_run_original(c.s, 0x107678a0);
                const auto table = c.r(3);
                for (std::uint32_t n = 0; n < 0x20; ++n) c.put32(table + 4 * n, c.u32(sc + 0x2170 + 4 * n));
                c.put32(sc + f_chunk_table, table);
                c.put32(sc + f_cursor, c.u32(sc + f_limit));
                c.put32(sc + f_count, count);
                c.set({{3, sc}, {9, 0x99990000u}, {10, 0xaaaa0000u}});
            }, "chunk count " + std::to_string(count) + " allocated table");
        }
        for (const std::uint32_t extent : {0u, 0x3fu, 0x40u, 0xffffu, 0x10000u, 0x100000u, 0xfffff000u, 0x80000000u}) {
            prepared(chunk_entry, outline, mode, [extent](guest::Cpu& c, std::uint32_t sc) {
                c.put32(sc + 0x38, extent);
                c.put32(sc + f_cursor, c.u32(sc + f_limit));
                c.set(3, sc);
            }, "chunk extent " + hex(extent));
        }
    }
    // 0x107614c0: the extents from the lists (the first pass reads the rows, the second the columns when they exist): real outlines, and fields poked: empty row
    // ranges, ranges that cross, a missing column table, lists cut to one node.
    for (const auto& [name, outline] : shapes) for (const bool grey : {false, true}) {
        const Mode full{grey, true, true, 0, 4};
        prepared(bounds_entry, outline, full, [](guest::Cpu& c, std::uint32_t sc) { c.set(3, sc); }, "bounds " + name + (grey ? " grey" : " mono"));
        const Mode plain{grey, false, false, 0, 4};
        prepared(bounds_entry, outline, plain, [](guest::Cpu& c, std::uint32_t sc) { c.set(3, sc); }, "bounds plain " + name + (grey ? " grey" : " mono"));
    }
    for (const auto& [name, outline] : std::vector<std::pair<std::string, Outline>>{{"circle", circle}, {"ring", shapes[2].second}}) {
        const Mode full{true, true, true, 0, 4};
        for (const int shrink : {0, 1, 2, 3, 4, 5, 6, 7}) {
            prepared(bounds_entry, outline, full, [shrink](guest::Cpu& c, std::uint32_t sc) {
                // Narrow the row and column ranges or cross them, and empty the lists at their ends.
                const auto first = c.u32(sc + f_first_row), last = c.u32(sc + f_last_row), cfirst = c.u32(sc + f_first_column), clast = c.u32(sc + f_last_column);
                if (shrink == 1) { c.put32(sc + f_first_row, last + 1); }
                if (shrink == 2) { c.put32(sc + f_last_row, first - 1); }
                if (shrink == 3) { c.put32(sc + f_first_column, clast + 1); }
                if (shrink == 4) { c.put32(sc + f_last_column, cfirst - 1); c.put32(sc + f_first_column, cfirst); }
                if (shrink == 5) { c.put32(c.u32(sc + f_row_heads) + 4 * first, 0); c.put32(c.u32(sc + f_row_heads) + 4 * last, 0); }
                if (shrink == 6) { c.put32(c.u32(sc + f_column_heads) + 4 * cfirst, 0); c.put32(c.u32(sc + f_column_heads) + 4 * clast, 0); }
                if (shrink == 7) { c.put8(sc + f_columns, 0); }
                c.set(3, sc);
            }, std::string("bounds ") + name + " poke " + std::to_string(shrink));
        }
    }
    prepared(bounds_entry, circle, {true, true, true, 0, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.put32(sc + 0x34, 0x7fffffff); c.put32(sc + 0x38, 0x80000000); c.put32(sc + 0x3c, 5); c.put32(sc + 0x40, 5); c.set(3, sc); }, "bounds point extents at the limits");

    // 0x10761730: the extents of the points (a leaf; no converter needed: it writes six words), the last index at its edges.
    for (const std::int32_t last : {-5, -1, 0, 1, 2, 3, 7, 8, 9, 10, 17, 40}) for (unsigned pattern = 0; pattern < 4; ++pattern) {
        std::vector<Write> memory;
        const std::uint32_t xs = words_at, ys = words_at + 0x400, sc = words_at + 0x800;
        for (unsigned n = 0; n < 48; ++n) {
            const auto x = pattern == 0 ? std::uint32_t(random()) : pattern == 1 ? n * 3 : pattern == 2 ? 100 - n : std::uint32_t(std::int32_t(random() % 2001) - 1000);
            const auto y = pattern == 0 ? std::uint32_t(random()) : pattern == 1 ? 7u : pattern == 2 ? n : std::uint32_t(std::int32_t(random() % 2001) - 1000);
            memory.push_back(w32(xs + 4 * n, x));
            memory.push_back(w32(ys + 4 * n, y));
        }
        cases.push_back({.entry = extents_entry, .registers = {{3, sc}, {4, xs}, {5, ys}, {6, std::uint32_t(last)}, {9, 0x99990000u}, {10, 0xaaaa0000u}}, .memory = memory,
                         .label = note("extents last " + std::to_string(last) + " pattern " + std::to_string(pattern))});
    }

    // 0x107623c0: the bitmap, from every shape and mode the converter takes; the skip byte; and lists poked into the shapes the assertions catch.
    for (const auto& [name, outline] : shapes) for (const auto& mode : modes) {
        for (const unsigned skip : {0u, 1u}) {
            if (skip && (name != "circle" && name != "thin")) continue;
            prepared(render, outline, mode, [skip](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {5, skip}}); }, "render " + name + " " + name_of(mode) + (skip ? " skip" : ""));
        }
    }
    prepared(render, circle, {true, true, true, 0, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.put32(sc + f_bounds_known, 1); c.set(3, sc); }, "render with known bounds (stale)");
    prepared(render, circle, {true, true, true, 0, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.put32(sc + f_last_row, 0x7000); c.set(3, sc); }, "render grey, row range past the bitmap (assertion)");
    prepared(render, circle, {false, true, true, 0, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.put32(sc + f_last_row, 0x7000); c.set(3, sc); }, "render mono, row range past the bitmap (assertion)");
    // The pixel box from poked extents: a negative width (the row size rounds towards zero), a zero height, wider and narrower than the shape.
    for (const int shrink : {0x40, 0x100, 0x240, 0x300, 0x1000}) for (const bool grey : {false, true}) {
        prepared(render, circle, {grey, true, true, 0, 4}, [shrink](guest::Cpu& c, std::uint32_t sc) {
            c.put32(sc + 0x20, 1);
            c.put32(sc + 0x24, 0x400 + std::uint32_t(shrink));
            c.put32(sc + 0x28, 0x400);
            c.put32(sc + 0x2c, 0);
            c.put32(sc + 0x30, 0xffffffc0u);  // the box's top row is the first row: no rows
            c.put32(sc + f_first_row, 5);
            c.put32(sc + f_last_row, 4);
            c.put32(sc + f_first_column, 5);
            c.put32(sc + f_last_column, 4);
            c.set({{3, sc}, {5, 0}});
        }, std::string("render negative width ") + hex(unsigned(shrink)) + (grey ? " grey" : " mono"));
    }
    // Lists with a single node (the assertion), with an odd number of nodes, with windings that do not close.
    for (const int kind : {0, 1, 2, 3, 4}) for (const auto& mode : std::vector<Mode>{{true, true, true, 0, 4}, {false, true, true, 0, 4}}) {
        prepared(render, circle, mode, [kind](guest::Cpu& c, std::uint32_t sc) {
            const auto rows = c.u32(sc + f_row_heads), columns = c.u32(sc + f_column_heads);
            const auto first_row = c.u32(sc + f_first_row), first_column = c.u32(sc + f_first_column);
            const auto row = c.u32(rows + 4 * (first_row + 10)), column = c.u32(columns + 4 * (first_column + 10));
            if (kind == 1 && row) c.put32(row + 4, 0);                         // a row with one node
            if (kind == 2 && column) c.put32(column + 4, 0);                   // a column with one node
            if (kind == 3 && row && c.u32(row + 4) && c.u32(c.u32(row + 4) + 4)) c.put32(c.u32(c.u32(row + 4) + 4) + 4, 0);   // a row cut to three nodes
            if (kind == 4 && column && c.u32(column + 4) && c.u32(c.u32(column + 4) + 4)) c.put32(c.u32(c.u32(column + 4) + 4) + 4, 0);
            c.set({{3, sc}, {5, 0}});
        }, std::string("render lists poked ") + std::to_string(kind) + (mode.grey ? " grey" : " mono"));
    }
    // Dropout flags on the thin shapes (the found words).
    for (const bool rows : {false, true}) for (const bool columns : {false, true}) {
        const Mode mode{false, rows, columns, 0, 4};
        for (const int w : {1, 20, 63, 64, 65, 100}) {
            prepared(render, {rectangle(10, 0, 10 + w, 700), rectangle(0, 300, 700, 300 + w, true)}, mode, [](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {5, 0}}); },
                     std::string("render thin ") + std::to_string(w) + (rows ? " rows" : "") + (columns ? " columns" : ""));
        }
    }

    // 0x10762e30: the converter given back: the null converter, the bitmap in its scratch slot or a block, chunks, tables and lists in the allocator, the converter
    // not in its slot.
    cases.push_back({.entry = destructor, .registers = {{3, 0}, {9, 0x99990000u}, {10, 0xaaaa0000u}}, .label = note("delete null")});
    for (const auto& [name, outline] : shapes) for (const bool grey : {false, true}) for (const bool rendered : {false, true}) {
        const Mode mode{grey, true, true, 0, 4};
        prepared(destructor, outline, mode, [rendered](guest::Cpu& c, std::uint32_t sc) {
            if (rendered) { c.set({{3, sc}, {5, 0}}); font_engine_run_original(c.s, render); }
            c.set({{3, sc}, {9, 0x99990000u}, {10, 0xaaaa0000u}});
        }, "delete " + name + (grey ? " grey" : " mono") + (rendered ? " rendered" : ""));
    }
    for (const int size : {0x2000, 0x5000, 0xb504, 0x12000}) {
        // Large shapes: the lists come from the allocator and the converter needs more chunks.
        prepared(destructor, {rectangle(0, 0, size, size / 4), ellipse(size / 2, size / 8, size / 4, size / 10)}, {true, true, true, 0, 4}, [](guest::Cpu& c, std::uint32_t sc) { c.set({{3, sc}, {9, 0x99990000u}, {10, 0xaaaa0000u}}); },
                 "delete large " + hex(unsigned(size)));
    }
    for (const int variant : {0, 1, 2, 3, 4, 5, 6}) {
        prepared(destructor, circle, {true, true, true, 0, 4}, [variant, mem](guest::Cpu& c, std::uint32_t sc) {
            const auto allocate = [&](std::uint32_t size) {
                c.set({{3, mem}, {4, size}});
                font_engine_run_original(c.s, 0x107678a0);
                return c.r(3);
            };
            if (variant == 1) { c.put32(sc + f_bitmap, allocate(64)); }                                   // a bitmap that is a block of its own
            if (variant == 2) { c.put32(mem + 0x138, 0); }                                                 // the converter is not in its slot: released as a block
            if (variant == 3) { c.put32(mem + 0x13c, 0); c.put32(sc + f_bitmap, allocate(64)); }           // a bitmap block, slot 5 elsewhere
            if (variant == 4 || variant == 5) {
                // chunks 1..3 (and a table in the allocator for 5)
                const auto table = variant == 5 ? allocate(0x100) : c.u32(sc + f_chunk_table);
                if (variant == 5) { for (std::uint32_t n = 0; n < 0x20; ++n) c.put32(table + 4 * n, c.u32(sc + 0x2170 + 4 * n)); c.put32(sc + f_chunk_table, table); c.put32(sc + f_capacity, 0x40); }
                for (std::uint32_t n = 1; n <= 3; ++n) c.put32(table + 4 * n, allocate(0x40 * n));
                c.put32(sc + f_count, 3);
            }
            if (variant == 6) { c.put32(sc + f_count, 0x7fffffff); c.put32(sc + f_count, 0); c.put32(sc + f_bitmap, 0); }
            c.set({{3, sc}, {9, 0x99990000u}, {10, 0xaaaa0000u}});
        }, "delete variant " + std::to_string(variant));
    }

    // The call trace pairs the first cases of an entry: the ones that reach the rarer call sites (assertions, tables in the allocator, releases) go first.
    std::stable_partition(cases.begin() + std::ptrdiff_t(first_case), cases.end(), [](const Case& item) {
        for (const char* marker : {"assertion", "poked", "chunk", "variant", "near the top", "known bounds", "negative", "contours count", "start after", "start -1", "end -1"})
            if (item.label.find(marker) != std::string::npos) return true;
        return false;
    });
}
}
