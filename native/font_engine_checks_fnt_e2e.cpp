// The font engine, end to end, interpreter mode (blocks fnt_a, fnt_b, fnt_c): synthetic TrueType programs run through the interpreter's real
// entry (0x10756310: the zones, the program's bounds, the graphic state, the trace flag) on the hint font the end-to-end run has just made of a
// shipped font (symbol.ttf: its fpgm has run, so its function definitions are real), with the zones, the operand stack and the programs built in
// the scratch area. The original and the production routing run the same programs from the same state (the glyph sweep calls this once a font's
// glyphs are cached; font_engine_checks_e2e.cpp); what the interpreter returns is logged and every byte the programs write (the operand stack, the
// zones' coordinates and flags, the storage and cvt, the definition arrays, the graphic state) is compared with the sweep's dirty pages. The
// programs cover each opcode group (arithmetic, flow, definitions and calls, vectors and rounding, point movement, storage and cvt, deltas).
#include "font_engine_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t interpreter_entry = 0x10756310;
constexpr std::uint32_t area = scratch_base + 0xb0000, zones_at = area, stack_at = area + 0x1000, program_at = area + 0x2000, zone_data = area + 0x4000;
constexpr std::uint32_t points = 24, zone_stride = 0x800;

using Program = std::vector<std::uint8_t>;
// A push of byte values (NPUSHB) and of signed halfwords (NPUSHW).
Program pushb(std::initializer_list<unsigned> values) {
    Program p{0x40, std::uint8_t(values.size())};
    for (const auto v : values) p.push_back(std::uint8_t(v));
    return p;
}
Program pushw(std::initializer_list<int> values) {
    Program p{0x41, std::uint8_t(values.size())};
    for (const auto v : values) { p.push_back(std::uint8_t(v >> 8)); p.push_back(std::uint8_t(v)); }
    return p;
}
Program join(std::initializer_list<Program> parts) {
    Program all;
    for (const auto& part : parts) all.insert(all.end(), part.begin(), part.end());
    return all;
}
}

std::vector<std::uint32_t> run_interpreter_programs(State& s, std::uint32_t hint) {
    guest::Cpu c(s);
    std::vector<std::uint32_t> log;
    const auto put32 = [&](std::uint32_t at, std::uint32_t v) { s.memory.write<4>(at, v); };
    const auto put16 = [&](std::uint32_t at, std::uint32_t v) { s.memory.write<2>(at, v & 0xffff); };
    const auto state = hint + 0x18;
    // Zones: the twilight zone and a glyph zone with the arrays the opcodes use, coordinates from a fixed generator.
    std::mt19937 random(0x1e2e);
    for (unsigned z = 0; z < 2; ++z) {
        const auto zone = zones_at + 0x2c * z, data = zone_data + zone_stride * z;
        const std::array<std::uint32_t, 11> fields = {data, data + 0x80, data + 0x100, data + 0x180, data + 0x200, data + 0x240, data + 0x280, 0, data + 0x2c0, data + 0x2e0, data + 0x300};
        for (unsigned k = 0; k < 11; ++k) if (k != 7) put32(zone + 4 * k, fields[k]);
        put16(zone + 0x1c, 3);
        for (unsigned p = 0; p < points; ++p) {
            const auto x = std::uint32_t(int(random() % 1500) - 200), y = std::uint32_t(int(random() % 1500) - 200);
            put32(data + 4 * p, x + (p * p) % 11); put32(data + 0x80 + 4 * p, y + (p * 5) % 7);
            put32(data + 0x100 + 4 * p, x); put32(data + 0x180 + 4 * p, y);
            put16(data + 0x200 + 2 * p, x / 3); put16(data + 0x240 + 2 * p, y / 3);
            s.memory.write<1>(data + 0x280 + p, p % 3 == 0);
            s.memory.write<1>(data + 0x300 + p, 0);
        }
        for (unsigned k = 0; k < 3; ++k) { put16(data + 0x2c0 + 2 * k, 8 * k); put16(data + 0x2e0 + 2 * k, 8 * k + 7); }
    }
    // The programs. Function 1 adds its two arguments; 2 doubles the top of the stack.
    const std::vector<Program> programs = {
        join({pushb({5, 3, 8, 4, 12, 7}), {0x60, 0x61, 0x63, 0x62, 0x8b, 0x8c, 0x64, 0x65, 0x66, 0x67, 0x56, 0x57, 0x5c}}),                        // arithmetic
        join({pushw({-100, 250, 0x7fff, -0x8000}), {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x5a, 0x5b}, pushb({1, 0, 9}), {0x20, 0x21, 0x23, 0x24, 0x25}}),  // comparisons, stack ops (fnt_a)
        join({pushb({1}), {0x58}, pushb({7}), {0x1b}, pushb({0x59, 0x1b, 0x58}), {0x59}, pushb({0x11, 0x22}), pushb({0}), {0x58, 0x58}, pushb({4}), {0x1b}, pushb({6}), {0x59, 0x59, 0x1b, 0x59}, pushb({0}), {0x58}, pushw({0x5959, 0x581b}), {0x1b}, pushb({3}), {0x59}}),  // IF / ELSE / EIF
        join({pushb({3}), {0x1c}, pushb({0, 0, 0}), pushb({2, 1}), {0x78}, pushb({0, 5, 0}), {0x79}, pushb({44, 44, 44, 44})}),                   // jumps over operands
        join({pushb({1}), {0x2c}, pushb({0, 0, 0}), {0x60, 0x2d}, pushb({2}), {0x2c, 0x20, 0x60, 0x2d}, pushb({30, 12, 1}), {0x2b, 0x21}, pushb({21, 2}), {0x2b}}),  // FDEF, CALL
        join({pushb({3, 1}), {0x2c, 0x21, 0xb0, 0x07, 0x2d}, pushb({3, 4, 1}), {0x2a}}),                                                          // FDEF and LOOPCALL
        join({{0x01}, pushb({2, 20}), {0x42}, pushb({2}), {0x43}, pushb({0, 100}), {0x44}, pushb({0}), {0x45}, pushb({0, 90}), {0x70}, {0x4b, 0x4c, 0x88}}),  // storage, cvt, info
        join({{0x01, 0x18}, pushb({3, 5}), {0x10, 0x11}, pushb({6}), {0x2e}, pushb({7, 2}), {0xc4}, pushb({9, 3}), {0xe4}, {0x00}, pushb({8, 1}), {0xd5}}),    // MDRP, MIRP on both axes
        join({{0x00, 0x1a}, pushb({3, 6}), {0x12, 0x3d}, pushb({4, 5}), {0x3a}, pushb({7}), {0x3c}, pushb({0, 4}), {0x17}, pushb({10}), {0x39}}),          // MSIRP, ALIGNRP, IP
        join({{0x01}, pushw({128}), pushb({8, 9}), {0x38}, pushb({2, 6}), {0x27}, pushb({3, 20}), {0x48}, pushb({5}), {0x46}, pushb({7, 8}), {0x49}}),     // SHPIX, ALIGNPTS, SCFS, GC, MD
        join({pushb({3, 4, 5}), {0x80}, pushb({2, 7}), {0x81}, pushb({3, 5}), {0x82}, pushb({7}), {0x29}, pushb({9}), {0x7f}, pushb({2, 0}), {0x7e}}),   // FLIPPT, FLIPRGON/OFF, UTP, AA
        join({pushb({0x41}), {0x5e}, pushb({3}), {0x5f}, pushb({2, 3, 0x12, 4, 0x20, 5, 3}), {0x5d}, pushb({1, 0x11, 4}), {0x73}, pushb({0x31, 6, 1}), {0x72}}),  // SDB, SDS, DELTAP, DELTAC
        join({pushb({1, 9, 0x20, 0x31}), {0x68, 0x6c, 0x76, 0x77, 0x18, 0x19, 0x7a, 0x7c, 0x7d, 0x68, 0x85}, pushb({0}), {0x85, 0x8d, 0x4d, 0x4e}}),  // rounding, scan control (the mix of fnt_a and fnt_c)
        join({pushb({1, 0x80, 4, 5}), {0xb1, 0x03, 0x04, 0xb8, 0x00, 0x08, 0x40, 0x00, 0x41, 0x00, 0x60}}),                                         // PUSHB, PUSHW, empty NPUSH
        {0x83, 0x01},                                                                                                                              // an undefined opcode (no instruction definitions): the error 0x1101
    };
    const auto program_bytes = [&](const Program& p) { for (std::size_t n = 0; n < p.size(); ++n) s.memory.write<1>(program_at + std::uint32_t(n), p[n]); };
    // The state words a prep would have set (the shipped font's hint font has run its fpgm only): the rounding state, the scale callback and its
    // factors, the ppem and ratios, the cut-ins; the font's own words are put back afterwards.
    std::vector<std::uint32_t> saved_state;
    for (std::uint32_t at = 0x58; at < 0xd0; at += 4) saved_state.push_back(std::uint32_t(s.memory.read<4>(state + at)));
    const auto toc_word = [&](int slot) { return std::uint32_t(s.memory.read<4>(0x108bba90 + std::uint32_t(slot))); };
    put32(state + 0x58, 0x44); put32(state + 0x5c, 0); put32(state + 0x60, 0x40); put32(state + 0x64, 0); put32(state + 0x68, 0); put32(state + 0x6c, 0x40);
    put32(state + 0x70, toc_word(-0x5b3c)); put32(state + 0x74, 0xffffffc0); put32(state + 0x78, 0x2d413ccd);
    put16(state + 0x7c, 0x40); put16(state + 0x7e, 0); put16(state + 0x80, 0x20); put16(state + 0x82, 9); put16(state + 0x84, 3); put16(state + 0x86, 3); put16(state + 0x88, 0);
    s.memory.write<1>(state + 0x8a, 0);
    put32(state + 0x98, toc_word(-0x3c24)); put32(state + 0xa4, 0x40); put32(state + 0xa8, 0x1000); put32(state + 0xac, 0x800); put32(state + 0xb0, 0); put16(state + 0xb4, 12);
    s.memory.write<1>(state + 0xb6, 0); s.memory.write<1>(state + 0xb7, 3); put32(state + 0xb8, 0x10000); put32(state + 0xbc, 0x10000);
    s.memory.write<1>(state + 0xc8, 0); put16(state + 0xca, 0);
    put16(state + 0xc, 14); put16(state + 0xe, 14); put32(state + 0x10, 14 << 16);
    const auto original_stack = s.memory.read<4>(state + 0), original_c9 = s.memory.read<1>(state + 0xc9);
    // The cvt and storage arrays: the font's own may be missing (symbol.ttf has no cvt), so the programs get arrays of the scratch area.
    constexpr std::uint32_t cvt_at = area + 0x3000, storage_at = area + 0x3200;
    for (unsigned n = 0; n < 64; ++n) { put32(cvt_at + 4 * n, 0x40 * n + 7); put32(storage_at + 4 * n, 0x1000 + n); }
    const auto original_cvt = s.memory.read<4>(state + 8), original_storage = s.memory.read<4>(state + 4);
    s.memory.write<4>(state + 8, cvt_at);
    s.memory.write<4>(state + 4, storage_at);
    s.memory.write<4>(state + 0, stack_at);
    for (std::size_t index = 0; index < programs.size(); ++index) {
        program_bytes(programs[index]);
        s.memory.write<1>(state + 0xc9, 1);
        c.set({{3, zones_at}, {4, program_at}, {5, program_at + std::uint32_t(programs[index].size())}, {6, state}, {7, 0}});
        if (std::getenv("FNT_E2E_DEBUG")) std::fprintf(stderr, "program %zu hint %x state+0x8c %x stack %x\n", index, hint, unsigned(s.memory.read<4>(state + 0x8c)), unsigned(s.memory.read<4>(state)));
        c.call(interpreter_entry, return_sentinel);
        if (std::getenv("FNT_E2E_DEBUG")) std::fprintf(stderr, "  returned %x stack top %x\n", c.r(3), unsigned(0));
        log.push_back(0x1e000000u | std::uint32_t(index));
        log.push_back(c.r(3));
    }
    for (std::uint32_t at = 0x58, k = 0; at < 0xd0; at += 4, ++k) put32(state + at, saved_state[k]);
    s.memory.write<4>(state + 0, std::uint32_t(original_stack));
    s.memory.write<4>(state + 8, std::uint32_t(original_cvt));
    s.memory.write<4>(state + 4, std::uint32_t(original_storage));
    s.memory.write<1>(state + 0xc9, std::uint8_t(original_c9));
    return log;
}
}
