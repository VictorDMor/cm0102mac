// The font engine, block `fnt_c` (0x10759250..0x1075ac40): cases for the interpreter's flow control, definitions, jump-table dispatchers,
// MDRP, MIRP, the delta opcodes, the opcode table's filler and the state tables' builder. The game runs the FDEF handler (symbol.ttf's fpgm, at
// load) and the table builders only; every other handler is dormant for it, so each outcome here comes from these cases (and from the
// interpreter end-to-end mode, font_engine_checks_fnt.cpp).
//
// The state is a real one: the hint font of the font in the snapshot that has one (symbol.ttf), found through the scaler's stamps. Its graphic
// state (the opcode table, the vector and angle tables, the scale callback, the round function) is copied into the scratch area, whose function
// and instruction definition arrays, programs, zones and stack are built per case around it: the handlers' stores land there, the dispatch
// through the glue reaches the real fnt_a handlers and the real run loop (0x10756280), which executes through the real opcode table.
// Labels start with "fnt_c:" (CM_FONT_ENGINE_BLOCK=fnt_c).
#include "font_engine_checks_fnt.hpp"
#include <algorithm>
#include <cstdlib>

namespace cm::aot::font_engine_checks {
namespace {
using namespace fnt;
const std::array<std::uint32_t, 14> edge_values = {0, 1, 2, 3, 0x3f, 0x40, 0x41, 0x7fffffff, 0x80000000, 0x80000001, 0xffffffff, 0xfffffffe, 0xffffffc0, 100};
}

FONT_ENGINE_CASES("fnt_c") {
    Fixture fixture(scenario);
    if (!fixture.hint) { std::fprintf(stderr, "fnt_c: no hint font in the snapshot\n"); return; }
    if (std::getenv("FNT_C_DEBUG")) {
        std::fprintf(stderr, "hint %x\n", fixture.hint);
        for (std::uint32_t at = 0; at < 0xd0; at += 4) std::fprintf(stderr, "state+%x = %08x\n", at, scenario.r32(fixture.hint + 0x18 + at));
    }
    const auto add = [&](std::uint32_t entry, const std::string& label, std::vector<Write> memory, std::uint32_t opcode, std::vector<std::uint32_t> stack,
                         std::vector<std::pair<unsigned, std::uint32_t>> registers = {}) -> Case& {
        auto writes = concat(fixture.base, memory);
        append(writes, stack_of(stack));
        writes.push_back(w32(ctx_at + 4, opcode));
        registers.insert(registers.begin(), {3, ctx_at});
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(writes), .label = "fnt_c: " + label});
        return cases.back();
    };
    constexpr std::uint32_t arithmetic = 0x10759250, unary = 0x10759430, skip = 0x10759520, if_entry = 0x107595a0, else_entry = 0x10759660, eif = 0x107596f0,
                            jmpr = 0x10759700, jrot = 0x10759730, jrof = 0x10759770, round_entry = 0x107597b0, nround = 0x10759810, cut_in = 0x10759840;

    // ---- 0x10759250: every opcode with a case with every pair of the edge values; the rest of the range (and outside it) with a few ----
    const std::array<std::uint32_t, 11> pair_values = {0, 1, 3, 0x40, 0x7fffffff, 0x80000000, 0x80000001, 0xffffffff, 0xffffffc0, 100, 0xfffffffe};
    for (std::uint32_t op = 0x4f; op <= 0x8d; ++op) {
        const bool defined = (op >= 0x50 && op <= 0x55) || op == 0x5a || op == 0x5b || (op >= 0x60 && op <= 0x63) || op == 0x8b || op == 0x8c;
        if (defined) {
            for (const auto a : pair_values)
                for (const auto b : pair_values) add(arithmetic, "arith op " + hex(op) + " " + hex(a) + " " + hex(b), {}, op, {a, b});
        } else {
            for (const auto [a, b] : {std::pair{1u, 2u}, std::pair{0x80000000u, 0xffffffffu}})
                add(arithmetic, "arith op " + hex(op) + " " + hex(a) + " " + hex(b), {}, op, {a, b});
        }
    }
    // ---- 0x10759430 ----
    for (std::uint32_t op = 0x55; op <= 0x68; ++op)
        for (const auto a : edge_values)
            add(unary, "unary op " + hex(op) + " " + hex(a), {}, op, {a});
    for (std::uint32_t op = 0x56; op <= 0x57; ++op)
        for (const std::uint32_t a : {0x20u, 0x1fu, 0x60u, 0xa0u, 0xffffffe0u, 0xffffff60u, 0x1000u})
            add(unary, "unary rounding op " + hex(op) + " " + hex(a), {}, op, {a});
    // ---- 0x10759520: the operands of every opcode ----
    for (std::uint32_t op = 0; op < 256; ++op)
        for (const std::uint32_t count : {0u, 1u, 3u, 9u, 255u}) {
            std::vector<std::uint8_t> bytes = {std::uint8_t(count)};
            for (unsigned n = 0; n < 8; ++n) bytes.push_back(std::uint8_t(0x30 + n));
            add(skip, "skip op " + hex(op) + " count " + hex(count), bytes_at(fpgm_at, bytes), op, {});
        }
    // ---- IF, ELSE, EIF, JMPR, JROT, JROF ----
    const std::vector<std::vector<std::uint8_t>> flows = {
        {0x59},                                              // an immediate EIF
        {0x1b, 0x59},                                        // ELSE, EIF
        {0x58, 0x59, 0x59},                                  // a nested IF with no else
        {0x58, 0x1b, 0x59, 0x1b, 0x59},                      // nested IF with its ELSE, then ours
        {0x58, 0x58, 0x58, 0x59, 0x1b, 0x59, 0x59, 0x1b, 0x59},
        {0xb0, 0x59, 0x59},                                  // PUSHB 0x59 inside: the byte is an operand
        {0xb1, 0x58, 0x1b, 0x59},                            // PUSHB x2 with IF and ELSE bytes
        {0xb8, 0x00, 0x59, 0x59},                            // PUSHW
        {0x40, 0x02, 0x58, 0x59, 0x59},                      // NPUSHB 2
        {0x41, 0x01, 0x59, 0x59, 0x59},                      // NPUSHW 1
        {0x40, 0x00, 0x59},                                  // NPUSHB 0
        {0x1b, 0x1b, 0x59},                                  // ELSE inside the scan
        {0x2c, 0x59},                                        // an ordinary opcode
        {0xc0, 0x59},                                        // an opcode above 0xb8 and 0xc0
        {0x5a, 0x59},                                        // 0x5a: the first above EIF
        {0x57, 0x59},                                        // 0x57: below IF
    };
    for (std::size_t f = 0; f < flows.size(); ++f) {
        auto program = flows[f];
        program.insert(program.end(), {0x59, 0x59, 0x59, 0x59, 0x59, 0x59});  // a terminator for a nest that runs on
        for (const std::uint32_t condition : {0u, 1u, 0x80000000u})
            add(if_entry, "if flow " + std::to_string(f) + " condition " + hex(condition), bytes_at(fpgm_at, program), 0x58, {condition});
        add(else_entry, "else flow " + std::to_string(f), bytes_at(fpgm_at, program), 0x1b, {});
    }
    add(if_entry, "if with an empty stack", bytes_at(fpgm_at, {0x59, 0x59}), 0x58, {});
    add(eif, "eif", {}, 0x59, {5, 6});
    for (const std::uint32_t offset : {0u, 1u, 2u, 5u, 0xffffffffu, 0xfffffff0u, 0x7fffffffu, 0x80000000u})
        add(jmpr, "jmpr " + hex(offset), {}, 0x1c, {offset});
    for (const std::uint32_t offset : {3u, 0u, 0xffffffffu, 0x100u})
        for (const std::uint32_t condition : {0u, 1u, 0xffffffffu, 0x80000000u}) {
            add(jrot, "jrot " + hex(offset) + " cond " + hex(condition), {}, 0x78, {offset, condition});
            add(jrof, "jrof " + hex(offset) + " cond " + hex(condition), {}, 0x79, {offset, condition});
        }
    // ---- ROUND, NROUND, the single width cut-in ----
    for (std::uint32_t op = 0x68; op <= 0x6f; ++op)
        for (const std::uint32_t value : {0u, 1u, 0x1fu, 0x20u, 0x21u, 0x3fu, 0x40u, 0xffffffe0u, 0xffffffdfu, 0x7fffffc0u, 0x80000000u, 0x12345u})
            add(op < 0x6c ? round_entry : nround, "round op " + hex(op) + " " + hex(value), {}, op, {value});
    for (const std::uint32_t distance : {0u, 0x40u, 0x3fu, 0x41u, 0x30u, 0x50u, 0xffffffc0u, 0xffffffd0u, 0xffffff90u, 0x80000000u, 0x7fffffffu, 0x20u, 0xffffffe0u})
        for (const std::uint32_t cutin : {0u, 0x10u, 0x8u, 0x100u, 0x7fffffffu, 0x80000000u})
            add(cut_in, "single width cut-in " + hex(distance) + " cut " + hex(cutin),
                {w32(state_at + 0x5c, cutin), w32(state_at + 0x60, 0x40)}, 0, {}, {{4, distance}});
    // ---- the vector and zone setups shared by MDRP and MIRP ----
    struct Vectors { const char* name; std::uint32_t px, py; std::uint32_t project, dual; };  // halfwords (2.14) and the descriptors' TOC slots
    const std::array<Vectors, 5> vectors = {{
        {"x", 0x4000, 0, std::uint32_t(-0x3c50), std::uint32_t(-0x3c50)},
        {"y", 0, 0x4000, std::uint32_t(-0x3c74), std::uint32_t(-0x3c74)},
        {"diagonal", 0x2d41, 0x2d41, std::uint32_t(-0x3c7c), std::uint32_t(-0x3c80)},
        {"steep", 0x3000, 0x2a54, std::uint32_t(-0x3c7c), std::uint32_t(-0x3c80)},
        {"negative", 0xd2bf, 0x2d41, std::uint32_t(-0x3c7c), std::uint32_t(-0x3c80)},
    }};
    const auto vector_writes = [&](const Vectors& v) {
        return std::vector<Write>{w16(ctx_at + 0x14, v.px), w16(ctx_at + 0x16, v.py), w32(ctx_at + 0x48, fixture.toc(int(v.project))), w32(ctx_at + 0x4c, fixture.toc(int(v.dual))),
                                  w16(ctx_at + 0x1c, v.px), w16(ctx_at + 0x1e, v.py)};
    };
    const std::uint32_t zone0 = zones_at, zone1 = zones_at + 0x2c;
    struct Zones { const char* name; std::uint32_t zp0, zp1; };
    const std::array<Zones, 4> zone_pairs = {{{"glyph/glyph", zone1, zone1}, {"glyph/twilight", zone1, zone0}, {"twilight/glyph", zone0, zone1}, {"twilight/twilight", zone0, zone0}}};
    const auto zone_writes = [&](const Zones& z) { return std::vector<Write>{w32(ctx_at + 8, z.zp0), w32(ctx_at + 0xc, z.zp1)}; };
    // ---- MDRP (0xc0..0xdf) ----
    for (std::uint32_t op = 0xc0; op <= 0xdf; ++op)
        for (const auto& z : zone_pairs)
            for (std::size_t vi = 0; vi < vectors.size(); ++vi) {
                if (vi >= 2 && (op & 3) && z.zp0 == z.zp1 && z.zp0 == zone0) continue;  // keep the count down
                for (const std::uint32_t cutin : {0u, 0x20u}) {
                    const std::uint32_t point = 3 + std::uint32_t((op * 7 + vi) % 12), rp0 = 1 + std::uint32_t((op + vi) % 4);
                    auto memory = concat(vector_writes(vectors[vi]), zone_writes(z));
                    append(memory, {w32(ctx_at + 0x30, rp0), w32(state_at + 0x5c, cutin), w32(state_at + 0x60, 0x40 + 3 * op % 5), w32(state_at + 0x6c, (op & 1) ? 0 : 0x38)});
                    add(0x107598e0, "mdrp op " + hex(op) + " " + z.name + " vectors " + vectors[vi].name + " cutin " + hex(cutin), memory, op, {point});
                }
            }
    // ---- MIRP (0xe0..0xff) ----
    const std::uint32_t cvt_at = area + 0x1800;
    for (std::uint32_t op = 0xe0; op <= 0xff; ++op)
        for (const auto& z : zone_pairs) {
            if (z.zp0 == z.zp1 && z.zp0 == zone0 && (op & 1)) continue;
            for (std::size_t vi = 0; vi < vectors.size(); ++vi)
                for (const std::uint32_t flip : {0u, 1u})
                    for (const std::uint32_t index : {0u, 1u, 2u, 3u}) {
                        if (vi >= 2 && index >= 2 && flip) continue;
                        auto memory = concat(vector_writes(vectors[vi]), zone_writes(z));
                        const std::array<std::uint32_t, 4> cvt = {0x40, 0x39, 0xffffffc0, 0x1234};
                        for (unsigned n = 0; n < 4; ++n) memory.push_back(w32(cvt_at + 4 * n, cvt[n]));
                        append(memory, {w32(state_at + 8, cvt_at), w8(state_at + 0x8a, flip), w32(ctx_at + 0x30, 2 + op % 5), w32(state_at + 0x5c, (op % 3) ? 0u : 0x30u),
                                        w32(state_at + 0x60, 0x44), w32(state_at + 0x6c, (op & 2) ? 0x50 : 0x10), w32(state_at + 0x58, (op & 4) ? 0x10 : 0x300)});
                        add(0x10759ae0, "mirp op " + hex(op) + " " + z.name + " vectors " + vectors[vi].name + " flip " + std::to_string(flip) + " cvt " + std::to_string(index), memory, op,
                            {4 + op % 9, index});
                    }
        }
    // ---- UTP, SDB, SDS ----
    for (const auto& [fx, fy] : {std::pair{0x4000u, 0u}, std::pair{0u, 0x4000u}, std::pair{0x2d41u, 0x2d41u}, std::pair{0u, 0u}})
        for (const std::uint32_t point : {0u, 5u, 15u}) {
            add(0x1075a150, "utp freedom " + hex(fx) + "," + hex(fy) + " point " + hex(point), {w16(ctx_at + 0x18, fx), w16(ctx_at + 0x1a, fy)}, 0x29, {point});
            add(0x1075a150, "utp twilight freedom " + hex(fx) + "," + hex(fy) + " point " + hex(point), {w16(ctx_at + 0x18, fx), w16(ctx_at + 0x1a, fy), w32(ctx_at + 8, zone0)}, 0x29, {point});
        }
    for (const std::uint32_t value : {0u, 9u, 0xffffffffu, 0x12345678u, 0x8000u}) {
        add(0x1075a1a0, "sdb " + hex(value), {}, 0x5e, {value});
        add(0x1075a1c0, "sds " + hex(value), {}, 0x5f, {value});
    }
    // ---- FDEF, IDEF, CALL, LOOPCALL, the undefined opcode ----
    const std::vector<std::vector<std::uint8_t>> bodies = {
        {0x2d},                                    // an empty function
        {0xb0, 0x07, 0x2d},                        // PUSHB 7
        {0x40, 0x03, 1, 2, 3, 0x2d},               // NPUSHB 3
        {0x41, 0x02, 0, 1, 0, 2, 0x2d},            // NPUSHW 2
        {0xb8, 0x12, 0x34, 0x2d},                  // PUSHW
        {0xb1, 0x2d, 0x2d, 0x2d},                  // PUSHB 2 holding ENDF bytes
        {0x58, 0x1b, 0x59, 0x2d},
        {0x60, 0x61, 0x2d},
    };
    for (std::size_t b = 0; b < bodies.size(); ++b)
        for (const std::uint32_t program : {0u, 1u})
            for (const std::uint32_t function : {0u, 3u, 7u}) {
                auto program_bytes = bodies[b];
                program_bytes.insert(program_bytes.end(), {0x2d, 0x2d, 0x2d, 0x2d});
                // the instruction pointer starts 3 bytes in, as after the opcode and the operands before it
                add(0x10759e30, "fdef body " + std::to_string(b) + " program " + std::to_string(program) + " function " + std::to_string(function),
                    concat(bytes_at(fpgm_at + 8, program_bytes), {w8(state_at + 0xc9, program), w32(ctx_at, fpgm_at + 8)}), 0x2c, {function});
            }
    // Function definitions of the fixture: function n starts at fpgm_at + 0x40 + 0x10 n, 4 bytes long (PUSHB 1 value, ADD...), in program 1 or 0.
    const std::array<std::vector<std::uint8_t>, 4> function_bodies = {{{0xb0, 0x02, 0x60}, {0xb1, 0x03, 0x04, 0x60, 0x60}, {}, {0x40, 0x01, 0x05, 0x61}}};
    std::vector<Write> definitions;
    for (std::uint32_t n = 0; n < 4; ++n) {
        const auto start = 0x40 + 0x10 * n;
        append(definitions, bytes_at(fpgm_at + start, function_bodies[n]));
        append(definitions, bytes_at(prep_at + start, function_bodies[n]));
        const auto program = n % 2;  // 1: fpgm, 0: prep
        definitions.push_back(w32(fdefs_at + 8 * n, start)); definitions.push_back(w16(fdefs_at + 8 * n + 4, std::uint32_t(function_bodies[n].size()))); definitions.push_back(w16(fdefs_at + 8 * n + 6, program));
    }
    for (const std::uint32_t function : {0u, 1u, 2u, 3u}) {
        add(0x10759dc0, "call " + hex(function), definitions, 0x2b, {1, 2, function});
        for (const std::uint32_t count : {0u, 1u, 2u, 3u, 0xffffffffu, 0x10001u, 0x10000u})
            add(0x10759ef0, "loopcall " + hex(function) + " count " + hex(count), definitions, 0x2a, {7, 8, count, function});
    }
    // An undefined opcode: instruction definitions for 0x91 and 0x93 (entries 8 bytes: start, length, program, opcode).
    std::vector<Write> idefs = definitions;
    append(idefs, {w32(idefs_at, 0x40), w16(idefs_at + 4, 3), w8(idefs_at + 6, 1), w8(idefs_at + 7, 0x91),
                   w32(idefs_at + 8, 0x70), w16(idefs_at + 12, 4), w8(idefs_at + 14, 0), w8(idefs_at + 15, 0x93), w16(state_at + 0xca, 2)});
    for (const std::uint32_t op : {0x91u, 0x93u, 0x92u, 0x83u, 0xafu, 0xffu, 0x00u, 0x191u})
        for (const std::uint32_t count : {2u, 1u, 0u, 0xffffu})
            add(0x10759fa0, "undefined op " + hex(op) + " with " + hex(count) + " idefs", concat(idefs, {w16(state_at + 0xca, count)}), op, {}, {})
                .answers = {{longjmp_entry, 0}};
    for (const std::uint32_t opcode_number : {0x91u, 0x93u, 0x94u, 0x191u, 0u})
        for (const std::uint32_t count : {2u, 0u, 1u})
            for (const std::uint32_t program : {0u, 1u})
                for (const auto& body : {bodies[0], bodies[3], bodies[5]}) {
                    auto program_bytes = body;
                    program_bytes.insert(program_bytes.end(), {0x2d, 0x2d, 0x2d, 0x2d});
                    add(0x1075a040, "idef " + hex(opcode_number) + " with " + hex(count) + " idefs program " + std::to_string(program) + " body " + std::to_string(body.size()),
                        concat(concat(idefs, bytes_at(fpgm_at + 0x100, program_bytes)), {w16(state_at + 0xca, count), w8(state_at + 0xc9, program), w32(ctx_at, fpgm_at + 0x100)}), 0x89, {opcode_number});
                }
    // ---- DELTAP and DELTAC ----
    std::mt19937 random(0xde17a);
    const std::array<std::pair<std::uint32_t, std::uint32_t>, 6> delta_entries = {{{0x1075a3a0, 0}, {0x1075a3d0, 16}, {0x1075a410, 32}, {0x1075a450, 0}, {0x1075a480, 16}, {0x1075a4c0, 32}}};
    for (unsigned n = 0; n < 600; ++n) {
        const auto& [entry, range] = delta_entries[n % 6];
        const std::uint32_t base = 9, shift = random() % 8, ppem_choice = random() % 6;
        const std::uint32_t ppem = ppem_choice == 5 ? 5 + random() % 80 : base + range + random() % 16;
        const auto count = std::uint32_t(random() % 7);
        std::vector<std::uint32_t> stack;
        std::uint32_t last = 0;
        for (std::uint32_t k = 0; k < count; ++k) {
            const bool match = random() % 2;
            const std::uint32_t high = match ? ppem - base - range : random() % 16;
            last = std::min<std::uint32_t>(15, std::max(last, high));
            const std::uint32_t arg = ((random() % 4 == 0 ? random() % 16 : last) << 4 | random() % 16) + (random() % 8 == 0 ? 0xfffff000u : 0u);
            stack.push_back(arg);
            stack.push_back(random() % 12);
        }
        stack.push_back(count);
        const auto& v = vectors[random() % vectors.size()];
        auto memory = vector_writes(v);
        append(memory, {w16(state_at + 0x82, base), w16(state_at + 0x84, shift), w16(state_at + 0xc, ppem), w8(state_at + 0xb6, random() % 3 == 0 ? 1 : 0),
                        w32(state_at + 0xb8, 0x10000 + 0x1111 * (random() % 5)), w32(state_at + 0xbc, 0x10000 + 0x777 * (random() % 5)), w32(state_at + 8, cvt_at)});
        add(entry, "delta entry " + hex(entry) + " case " + std::to_string(n) + " ppem " + std::to_string(ppem) + " vectors " + v.name + " count " + std::to_string(count), memory, 0x5d, stack);
    }
    // The generic delta entry (the callers' registers): 0x1075a1e0 with each function, base and shift.
    for (const std::uint32_t function : {fixture.toc(-0x3c4c), fixture.toc(-0x3c90), fixture.toc(-0x3c78)})
        for (const std::uint32_t base : {9u, 0xfffffff7u, 0x20009u})
            add(0x1075a1e0, "delta direct function " + hex(function) + " base " + hex(base), {w16(state_at + 0xc, 12), w8(state_at + 0xb6, 0), w16(ctx_at + 0x14, 0x4000), w16(ctx_at + 0x16, 0)}, 0x5d,
                {0x10, 3, 0x21, 4, 0x31, 5, 3}, {{4, function}, {5, base}, {6, 2}});
    // ---- the table builders ----
    std::vector<Write> junk;
    for (unsigned k = 0; k < 0x130; ++k) junk.push_back(w32(table_at + 4 * k, 0xdead0000u + k));
    cases.push_back({.entry = 0x1075a500, .registers = {{3, table_at}}, .memory = junk, .label = "fnt_c: opcode table filler"});
    auto state_junk = junk;
    append(state_junk, {w32(state_at + 0x8c, table_at), w32(state_at + 0xc0, 0x11111111), w32(state_at + 0xc4, 0x22222222)});
    cases.push_back({.entry = 0x1075a970, .registers = {{3, state_at}}, .memory = state_junk, .label = "fnt_c: state tables builder"});
}
}
