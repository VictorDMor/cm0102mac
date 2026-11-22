// The font engine, block `fnt_b` (0x10757fb0..0x10759250): cases for the graphic-state and point-movement opcode handlers. The game runs none of
// them (symbol.ttf's fpgm only defines functions), so every outcome here comes from these cases: each handler with the boundary operands its
// code has (loop counts 0, 1 and many, points in the twilight zone and the glyph zone, the x and y and generic projections, zero distances, the
// 26.6 values at their signs, the stack operands at their limits) on the shared fixture (font_engine_checks_fnt.hpp: a real hint font's graphic
// state with the execution context, zones, stack, programs and definitions built around it). The handlers reach the real project, move and scale
// functions of FNT's first third through the glue, on both sides. Labels start with "fnt_b:" (CM_FONT_ENGINE_BLOCK=fnt_b).
#include "font_engine_checks_fnt.hpp"
#include <algorithm>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
using namespace fnt;
constexpr std::uint32_t storage_at = area + 0x1c00, cvt_at = area + 0x1800;
}

FONT_ENGINE_CASES("fnt_b") {
    Fixture fixture(scenario);
    if (!fixture.hint) return;
    const auto add = [&](std::uint32_t entry, const std::string& label, std::vector<Write> memory, std::uint32_t opcode, std::vector<std::uint32_t> stack,
                         std::vector<std::pair<unsigned, std::uint32_t>> registers = {}) -> Case& {
        auto writes = concat(fixture.base, memory);
        writes.push_back(w32(state_at + 4, storage_at));
        writes.push_back(w32(state_at + 8, cvt_at));
        for (unsigned n = 0; n < 16; ++n) { writes.push_back(w32(storage_at + 4 * n, 0x1000 + n)); writes.push_back(w32(cvt_at + 4 * n, 0x40 * n + 7)); }
        append(writes, stack_of(stack));
        writes.push_back(w32(ctx_at + 4, opcode));
        registers.insert(registers.begin(), {3, ctx_at});
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(writes), .label = "fnt_b: " + label});
        return cases.back();
    };
    const std::uint32_t zone0 = zones_at, zone1 = zones_at + 0x2c;
    struct Vectors { const char* name; std::uint32_t px, py, fx, fy; std::int32_t project, dual; };
    const std::array<Vectors, 4> vectors = {{
        {"x", 0x4000, 0, 0x4000, 0, -0x3c50, -0x3c50},
        {"y", 0, 0x4000, 0, 0x4000, -0x3c74, -0x3c74},
        {"diagonal", 0x2d41, 0x2d41, 0x2d41, 0x2d41, -0x3c7c, -0x3c80},
        {"mixed", 0x3000, 0x2a54, 0x4000, 0, -0x3c7c, -0x3c80},
    }};
    const auto vector_writes = [&](const Vectors& v) {
        return std::vector<Write>{w16(ctx_at + 0x14, v.px), w16(ctx_at + 0x16, v.py), w16(ctx_at + 0x18, v.fx), w16(ctx_at + 0x1a, v.fy), w16(ctx_at + 0x1c, v.px), w16(ctx_at + 0x1e, v.py),
                                  w32(ctx_at + 0x48, fixture.toc(v.project)), w32(ctx_at + 0x4c, fixture.toc(v.dual))};
    };
    struct Zones { const char* name; std::uint32_t zp0, zp1, zp2; };
    const std::array<Zones, 4> zone_sets = {{{"glyph", zone1, zone1, zone1}, {"twilight", zone0, zone0, zone0}, {"mixed 1", zone1, zone0, zone1}, {"mixed 2", zone0, zone1, zone0}}};
    const auto zone_writes = [&](const Zones& z) { return std::vector<Write>{w32(ctx_at + 8, z.zp0), w32(ctx_at + 0xc, z.zp1), w32(ctx_at + 0x10, z.zp2)}; };
    const std::array<std::uint32_t, 8> amounts = {0, 1, 0x40, 0xffffffc0, 0x7fffffff, 0x80000000, 0x123, 0xfffffffe};

    // ---- SHPIX ----
    for (const auto& v : vectors)
        for (const auto& z : zone_sets)
            for (const std::uint32_t loop : {0u, 1u, 3u})
                for (const auto amount : amounts) {
                    auto memory = concat(vector_writes(v), zone_writes(z));
                    memory.push_back(w16(ctx_at + 0x3e, loop));
                    std::vector<std::uint32_t> stack;
                    for (std::uint32_t k = 0; k <= loop; ++k) stack.push_back(2 + 3 * k);
                    stack.push_back(amount);
                    add(0x10757fb0, "shpix " + std::string(v.name) + " " + z.name + " loop " + std::to_string(loop) + " " + hex(amount), memory, 0x38, stack);
                }
    for (const std::uint32_t loop : {0xffffu, 0x8000u, 2u}) add(0x10757fb0, "shpix loop " + hex(loop), {w16(ctx_at + 0x3e, loop)}, 0x38, {1, 2, 3, 0x40});
    // ---- IP, MSIRP, ALIGNRP, ALIGNPTS, SCFS, GC, MD ----
    for (const auto& v : vectors)
        for (const auto& z : zone_sets) {
            for (const std::uint32_t loop : {0u, 2u})
                for (const auto [rp1, rp2] : {std::pair{1u, 5u}, std::pair{4u, 4u}, std::pair{2u, 9u}}) {
                    auto memory = concat(vector_writes(v), zone_writes(z));
                    append(memory, {w16(ctx_at + 0x3e, loop), w32(ctx_at + 0x34, rp1), w32(ctx_at + 0x38, rp2)});
                    std::vector<std::uint32_t> stack;
                    for (std::uint32_t k = 0; k <= loop; ++k) stack.push_back(3 + 4 * k);
                    add(0x107580d0, "ip " + std::string(v.name) + " " + z.name + " loop " + std::to_string(loop) + " rp " + std::to_string(rp1) + "," + std::to_string(rp2), memory, 0x39, stack);
                }
            for (const std::uint32_t op : {0x3au, 0x3bu})
                for (const auto distance : amounts) {
                    auto memory = concat(vector_writes(v), zone_writes(z));
                    memory.push_back(w32(ctx_at + 0x30, 3));
                    add(0x10758270, "msirp op " + hex(op) + " " + std::string(v.name) + " " + z.name + " " + hex(distance), memory, op, {7, distance});
                }
            for (const std::uint32_t loop : {0u, 1u, 3u}) {
                auto memory = concat(vector_writes(v), zone_writes(z));
                append(memory, {w16(ctx_at + 0x3e, loop), w32(ctx_at + 0x30, 2)});
                std::vector<std::uint32_t> stack;
                for (std::uint32_t k = 0; k <= loop; ++k) stack.push_back(5 + 2 * k);
                add(0x107583b0, "alignrp " + std::string(v.name) + " " + z.name + " loop " + std::to_string(loop), memory, 0x3c, stack);
            }
            for (const auto [p1, p2] : {std::pair{3u, 8u}, std::pair{5u, 5u}, std::pair{0u, 15u}})
                add(0x10758480, "alignpts " + std::string(v.name) + " " + z.name + " " + std::to_string(p1) + "," + std::to_string(p2), concat(vector_writes(v), zone_writes(z)), 0x27, {p1, p2});
            for (const auto value : amounts)
                add(0x10758ee0, "scfs " + std::string(v.name) + " " + z.name + " " + hex(value), concat(vector_writes(v), zone_writes(z)), 0x48, {6, value});
            for (const std::uint32_t op : {0x46u, 0x47u})
                add(0x10758e40, "gc op " + hex(op) + " " + std::string(v.name) + " " + z.name, concat(vector_writes(v), zone_writes(z)), op, {9});
            for (const std::uint32_t op : {0x49u, 0x4au})
                add(0x10758fa0, "md op " + hex(op) + " " + std::string(v.name) + " " + z.name, concat(vector_writes(v), zone_writes(z)), op, {9, 4});
        }
    // ---- the angle adjustment (0x7f): points at every octant of rp0 and the weights ----
    for (const auto& z : {zone_sets[0], zone_sets[2]})
        for (std::uint32_t rp0 : {1u, 6u})
            for (std::uint32_t point = 0; point < 16; point += 3) {
                auto memory = zone_writes(z);
                append(memory, {w32(ctx_at + 0x30, rp0), w16(ctx_at + 0x18, 0x4000), w16(ctx_at + 0x1a, 0)});
                for (const std::uint32_t weight : {3u, 0u, 0x7fffu})
                    add(0x10758790, "aa rp0 " + std::to_string(rp0) + " point " + std::to_string(point) + " weight " + hex(weight) + " " + z.name,
                        concat(memory, {w16(state_at + 0x86, weight)}), 0x7f, {point});
            }
    {   // Crafted directions: a point on rp0 (a zero distance divides by zero), straight and diagonal lines both ways.
        const std::array<std::pair<int, int>, 12> directions = {{{0, 0}, {100, 0}, {-100, 0}, {0, 100}, {0, -100}, {100, 100}, {-100, 100}, {100, -100}, {-100, -100}, {300, 20}, {20, 300}, {-250, -9}}};
        for (std::size_t d = 0; d < directions.size(); ++d)
            for (const std::uint32_t weight : {3u, 0x4000u}) {
                std::vector<Write> memory = zone_writes(zone_sets[0]);
                append(memory, {w32(ctx_at + 0x30, 1), w16(state_at + 0x86, weight)});
                const std::uint32_t xs = zone_data + zone_stride, ys = xs + 0x40;
                memory.push_back(w32(xs + 4, 1000)); memory.push_back(w32(ys + 4, 2000));
                memory.push_back(w32(xs + 4 * 7, std::uint32_t(1000 + directions[d].first))); memory.push_back(w32(ys + 4 * 7, std::uint32_t(2000 + directions[d].second)));
                add(0x10758790, "aa direction " + std::to_string(d) + " weight " + hex(weight), memory, 0x7f, {7});
            }
    }
    {   // The state's own trig table holds words whose low halves are mostly zero, so the vector chosen is degenerate; a table of real 2.14 components
        // (angles ascending) shows the swap, the signs and the move distance.
        constexpr std::uint32_t vectors_at = area + 0x1e00, angles_at = area + 0x1f00;
        const std::array<std::pair<int, int>, 8> table = {{{0, 0x4000}, {0x0400, 0x3fff}, {0x1000, 0x3d3f}, {0x2d41, 0x2d41}, {0x3d3f, 0x1000}, {0x3fff, 0x0400}, {0x4000, 0}, {0x3fff, -0x0400}}};
        std::vector<Write> custom = {w32(state_at + 0xc0, vectors_at), w32(state_at + 0xc4, angles_at)};
        for (std::size_t k = 0; k < 20; ++k) {
            const auto [x, y] = table[k % table.size()];
            custom.push_back(w32(vectors_at + 8 * std::uint32_t(k), std::uint32_t(x))); custom.push_back(w32(vectors_at + 8 * std::uint32_t(k) + 4, std::uint32_t(y)));
            custom.push_back(w16(angles_at + 2 * std::uint32_t(k), 0x10 + 0x18 * std::uint32_t(k)));
        }
        const std::array<std::pair<int, int>, 14> directions = {{{100, 0}, {-100, 0}, {0, 100}, {0, -100}, {100, 100}, {-100, 100}, {100, -100}, {-100, -100}, {300, 20}, {20, 300}, {-250, -9}, {-9, -250}, {64, 3}, {5, 64}}};
        for (std::size_t d = 0; d < directions.size(); ++d)
            for (const std::uint32_t weight : {3u, 40u, 0x2000u}) {
                auto memory = concat(zone_writes(zone_sets[0]), custom);
                append(memory, {w32(ctx_at + 0x30, 1), w16(state_at + 0x86, weight), w16(ctx_at + 0x18, 0x4000), w16(ctx_at + 0x1a, 0)});
                const std::uint32_t xs = zone_data + zone_stride, ys = xs + 0x40;
                memory.push_back(w32(xs + 4, 1000)); memory.push_back(w32(ys + 4, 2000));
                memory.push_back(w32(xs + 4 * 7, std::uint32_t(1000 + directions[d].first))); memory.push_back(w32(ys + 4 * 7, std::uint32_t(2000 + directions[d].second)));
                add(0x10758790, "aa table direction " + std::to_string(d) + " weight " + hex(weight), memory, 0x7f, {7});
            }
    }
    // ---- SANGW, SCANCTRL, SCANTYPE, INSTCTRL, FLIPON, FLIPOFF, DEBUG ----
    for (const auto value : amounts) {
        add(0x10758550, "sangw " + hex(value), {}, 0x7e, {value});
        add(0x10758670, "scanctrl " + hex(value), {w32(state_at + 0x64, 0x12345678)}, 0x85, {value});
        add(0x10758670, "scanctrl zero word " + hex(value), {w32(state_at + 0x64, 0)}, 0x85, {value});
    }
    for (const std::uint32_t value : {0u, 1u, 2u, 3u, 0xffffffffu, 0x10000u})
        add(0x107586a0, "scantype " + hex(value), {w32(state_at + 0x64, 0x12345678)}, 0x8d, {value});
    for (const std::uint32_t selector : {0u, 1u, 2u, 3u, 0x101u})
        for (const std::uint32_t value : {0u, 1u, 2u, 3u, 0xffffffffu})
            for (const std::uint32_t program : {0u, 1u})
                add(0x10758710, "instctrl " + hex(selector) + " " + hex(value) + " program " + std::to_string(program), {w8(state_at + 0xc8, program), w32(state_at + 0x68, 0x5a5a5a5a)}, 0x8e, {value, selector});
    add(0x10759220, "flipon", {w8(state_at + 0x8a, 0)}, 0x4d, {});
    add(0x10759230, "flipoff", {w8(state_at + 0x8a, 1)}, 0x4e, {});
    add(0x10759240, "debug", {}, 0x4f, {1, 2, 3});
    add(0x10759240, "debug empty", {}, 0x4f, {});
    // ---- FLIPPT, FLIPRGON, FLIPRGOFF ----
    for (const auto& z : {zone_sets[0], zone_sets[1]}) {
        for (const std::uint32_t loop : {0u, 1u, 4u, 0xffffu}) {
            std::vector<std::uint32_t> stack;
            for (std::uint32_t k = 0; k < 5; ++k) stack.push_back(2 * k + 1);
            add(0x10758570, "flippt " + std::string(z.name) + " loop " + std::to_string(loop), concat(zone_writes(z), {w16(ctx_at + 0x3e, loop)}), 0x80, stack);
        }
        for (const auto [first, last] : {std::pair{2u, 6u}, std::pair{4u, 4u}, std::pair{6u, 2u}, std::pair{0u, 15u}, std::pair{3u, 0x10003u}}) {
            add(0x107585c0, "fliprgon " + std::string(z.name) + " " + std::to_string(first) + ".." + std::to_string(last), zone_writes(z), 0x81, {first, last});
            add(0x10758610, "fliprgoff " + std::string(z.name) + " " + std::to_string(first) + ".." + std::to_string(last), zone_writes(z), 0x82, {first, last});
        }
    }
    // ---- the pushes ----
    const std::vector<std::uint8_t> stream = {0x03, 0x80, 0x7f, 0xff, 0x00, 0x01, 0xfe, 0x12, 0x34, 0xab, 0xcd, 0x00, 0x00, 0x7f, 0xff, 0x80, 0x00};
    for (std::uint32_t op = 0xb0; op <= 0xb7; ++op) add(0x10758b60, "pushb " + hex(op), bytes_at(fpgm_at, stream), op, {5}, {}).memory.push_back(w32(ctx_at, fpgm_at));
    for (std::uint32_t op = 0xb8; op <= 0xbf; ++op) add(0x10758bd0, "pushw " + hex(op), bytes_at(fpgm_at, stream), op, {5}, {}).memory.push_back(w32(ctx_at, fpgm_at));
    for (const std::uint8_t count : {0, 1, 2, 5, 8}) {
        auto bytes = stream;
        bytes[0] = count;
        add(0x10758b90, "npushb " + std::to_string(count), bytes_at(fpgm_at, bytes), 0x40, {5}).memory.push_back(w32(ctx_at, fpgm_at));
        add(0x10758c00, "npushw " + std::to_string(count), bytes_at(fpgm_at, bytes), 0x41, {5}).memory.push_back(w32(ctx_at, fpgm_at));
    }
    for (const std::uint32_t count : {0u, 1u, 3u, 0xffffu, 0x10002u})
        for (const std::uint32_t bytes : {0u, 1u, 0x1000u})
            add(0x10758ae0, "push loop count " + hex(count) + " bytes " + hex(bytes), concat(bytes_at(fpgm_at, stream), {w32(ctx_at, fpgm_at)}), 0, {1}, {{4, count}, {5, bytes}});
    // ---- storage and cvt ----
    for (const std::uint32_t index : {0u, 3u, 15u}) {
        add(0x10758c40, "ws " + hex(index), {}, 0x42, {index, 0xdeadbeef});
        add(0x10758c80, "rs " + hex(index), {}, 0x43, {index});
        add(0x10758de0, "rcvt " + hex(index), {}, 0x45, {index});
        add(0x10758de0, "rcvt scaled " + hex(index), {w32(ctx_at + 0x54, fixture.toc(-0x3c5c)), w16(ctx_at + 0x14, 0x4000), w16(ctx_at + 0x16, 0)}, 0x45, {index});
        for (const std::uint32_t value : {0u, 0x40u, 0x7u, 0xffffffc0u, 0x80000000u, 0x1234u})
            for (const auto scale : {std::pair{"raw", -0x3c54}, std::pair{"scaled", -0x3c5c}}) {
                std::vector<Write> memory = {w32(ctx_at + 0x54, fixture.toc(scale.second)), w32(state_at + 0xb8, 0x18000), w32(state_at + 0xbc, 0x12345)};
                add(0x10758cc0, std::string("wcvtp ") + scale.first + " " + hex(index) + " " + hex(value), memory, 0x44, {index, value});
            }
        for (const std::uint32_t value : {0u, 0x40u, 0xffffffc0u, 0x7fffffffu, 0x1234u})
            add(0x10758d70, "wcvtf " + hex(index) + " " + hex(value), {}, 0x70, {index, value});
    }
    // ---- MPPEM, MPS, GETINFO ----
    for (const auto& v : vectors)
        for (const std::uint32_t mode : {0u, 1u})
            for (const std::uint32_t ppem : {9u, 12u, 0xffffu})
                add(0x107590b0, "mppem " + std::string(v.name) + " mode " + std::to_string(mode) + " ppem " + hex(ppem),
                    concat(vector_writes(v), {w8(state_at + 0xb6, mode), w16(state_at + 0xc, ppem), w32(state_at + 0xb8, 0x13000), w32(state_at + 0xbc, 0x11000)}), 0x4b, {});
    add(0x10759190, "mps", {w16(state_at + 0xe, 17)}, 0x4c, {});
    for (const std::uint32_t selector : {0u, 1u, 2u, 3u, 4u, 6u, 7u, 0xffu})
        for (const std::uint32_t flags : {0u, 1u, 2u, 3u})
            add(0x107591b0, "getinfo " + hex(selector) + " flags " + hex(flags), {w8(state_at + 0xb7, flags)}, 0x88, {selector});
}
}
