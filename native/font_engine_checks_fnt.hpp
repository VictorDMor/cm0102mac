#pragma once
// The fixture the interpreter blocks' cases (fnt_b, fnt_c) share: a real hint font's graphic state copied into the scratch area with the
// execution context, the zones, the operand stack, the programs and the definition arrays built around it (see font_engine_checks_fnt_c.cpp).
#include "font_engine_checks.hpp"
#include <array>
#include <random>

namespace cm::aot::font_engine_checks {
namespace fnt {
inline constexpr std::uint32_t longjmp_entry = 0x10773f30;
constexpr std::uint32_t area = scratch_base + 0xa0000;
constexpr std::uint32_t ctx_at = area, state_at = area + 0x100, zones_at = area + 0x200, stack_at = area + 0x400, fdefs_at = area + 0x800,
                        idefs_at = area + 0x900, prep_at = area + 0xa00, fpgm_at = area + 0xc00, zone_data = area + 0x1000, table_at = area + 0x4000;
constexpr std::uint32_t zone_stride = 0x400, points = 16;

// The first hint font of the snapshot (a font's sfnt +4), through the scaler's stamps.
inline std::uint32_t hint_font_of(const Scenario& scenario) {
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4) {
            const auto scaler = std::uint32_t(at);
            if (scenario.r32(scaler) != 0x5a1234a5 || scenario.r32(scaler + 0x5f4) != 0xa5fedc5a) continue;
            const auto sfnt = scenario.r32(scaler + 0x144);
            if (!sfnt) continue;
            const auto hint = scenario.r32(sfnt + 4);
            if (hint) return hint;
        }
    }
    return 0;
}

inline std::vector<Write> concat(std::vector<Write> a, const std::vector<Write>& b) { append(a, b); return a; }
inline std::vector<Write> bytes_at(std::uint32_t at, const std::vector<std::uint8_t>& bytes) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < bytes.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), bytes[n]));
    return memory;
}
// The operand stack: the values from the bottom, and the stack-top word of the context.
inline std::vector<Write> stack_of(const std::vector<std::uint32_t>& values) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < values.size(); ++n) memory.push_back(w32(stack_at + 4 * std::uint32_t(n), values[n]));
    memory.push_back(w32(ctx_at + 0x20, stack_at + 4 * std::uint32_t(values.size())));
    return memory;
}

struct Fixture {
    const Scenario& scenario;
    std::uint32_t hint = 0;
    std::vector<Write> base;
    std::uint32_t toc(int slot) const { return scenario.toc(slot); }

    explicit Fixture(const Scenario& sc) : scenario(sc), hint(hint_font_of(sc)) {
        if (!hint) return;
        auto& m = base;
        const auto zone1 = zones_at + 0x2c;
        // The context (interpreter::...).
        m.push_back(w32(ctx_at + 0x00, fpgm_at));
        m.push_back(w32(ctx_at + 0x04, 0));
        for (const auto offset : {0x08u, 0x0cu, 0x10u}) m.push_back(w32(ctx_at + offset, zone1));
        m.push_back(w16(ctx_at + 0x14, 0x4000)); m.push_back(w16(ctx_at + 0x16, 0));
        m.push_back(w16(ctx_at + 0x18, 0x4000)); m.push_back(w16(ctx_at + 0x1a, 0));
        m.push_back(w16(ctx_at + 0x1c, 0x4000)); m.push_back(w16(ctx_at + 0x1e, 0));
        m.push_back(w32(ctx_at + 0x20, stack_at));
        m.push_back(w32(ctx_at + 0x24, zones_at));
        m.push_back(w32(ctx_at + 0x28, state_at));
        m.push_back(w32(ctx_at + 0x2c, 0));
        m.push_back(w32(ctx_at + 0x30, 1)); m.push_back(w32(ctx_at + 0x34, 2)); m.push_back(w32(ctx_at + 0x38, 3));
        m.push_back(w16(ctx_at + 0x3c, 0)); m.push_back(w16(ctx_at + 0x3e, 0)); m.push_back(w16(ctx_at + 0x40, 0x4000));
        m.push_back(w32(ctx_at + 0x44, toc(-0x3c4c)));  // move along x
        m.push_back(w32(ctx_at + 0x48, toc(-0x3c50)));  // project on x
        m.push_back(w32(ctx_at + 0x4c, toc(-0x3c50)));
        m.push_back(w32(ctx_at + 0x50, toc(-0x3c6c)));  // the run loop
        m.push_back(w32(ctx_at + 0x54, toc(-0x3c54)));  // the cvt read
        m.push_back(w32(ctx_at + 0x58, toc(-0x3c58)));  // the single width value
        // The graphic state: the snapshot's own, with the tables and programs of the fixture.
        for (std::uint32_t at = 0; at < 0xd0; at += 4) m.push_back(w32(state_at + at, scenario.r32(hint + 0x18 + at)));
        m.push_back(w32(state_at + 0x90, fdefs_at)); m.push_back(w32(state_at + 0x94, idefs_at));
        m.push_back(w32(state_at + 0x9c, prep_at)); m.push_back(w32(state_at + 0xa0, fpgm_at));
        m.push_back(w32(state_at + 0x58, 0x30)); m.push_back(w32(state_at + 0x5c, 0)); m.push_back(w32(state_at + 0x60, 0x40));
        m.push_back(w32(state_at + 0x6c, 0x40));
        m.push_back(w16(state_at + 0x82, 9)); m.push_back(w16(state_at + 0x84, 3)); m.push_back(w16(state_at + 0x88, 0));
        m.push_back(w8(state_at + 0x8a, 0)); m.push_back(w8(state_at + 0xb6, 0)); m.push_back(w8(state_at + 0xc8, 0)); m.push_back(w8(state_at + 0xc9, 1));
        m.push_back(w16(state_at + 0xca, 0));
        // The words the game's hint font leaves unset until a prep runs (this snapshot's has no programs): the rounding state, the scale callback and its
        // factors, the ppem and the ratios, as a font program that had set them would leave them.
        m.push_back(w32(state_at + 0x70, toc(-0x5b3c))); m.push_back(w32(state_at + 0x74, 0xffffffc0)); m.push_back(w32(state_at + 0x78, 0x2d413ccd));
        m.push_back(w16(state_at + 0x7c, 0x40)); m.push_back(w16(state_at + 0x7e, 0)); m.push_back(w16(state_at + 0x80, 0x20)); m.push_back(w16(state_at + 0x86, 3));
        m.push_back(w32(state_at + 0x98, toc(-0x3c24))); m.push_back(w32(state_at + 0xa4, 0x40)); m.push_back(w32(state_at + 0xa8, 0x1000));
        m.push_back(w32(state_at + 0xac, 0x800)); m.push_back(w32(state_at + 0xb0, 0)); m.push_back(w16(state_at + 0xb4, 12)); m.push_back(w8(state_at + 0xb7, 3));
        m.push_back(w32(state_at + 0xb8, 0x10000)); m.push_back(w32(state_at + 0xbc, 0x10000));
        m.push_back(w16(state_at + 0xc, 12)); m.push_back(w16(state_at + 0xe, 14)); m.push_back(w32(state_at + 0x10, 12 << 16));
        // Two zones: the twilight zone and a glyph zone of 16 points with coordinates that differ across the arrays.
        std::mt19937 random(0xf27c);
        for (unsigned z = 0; z < 2; ++z) {
            const auto zone = zones_at + 0x2c * z, data = zone_data + zone_stride * z;
            const std::array<std::uint32_t, 11> fields = {data, data + 0x40, data + 0x80, data + 0xc0, data + 0x100, data + 0x120, data + 0x140, 0, data + 0x160, data + 0x170, data + 0x150};
            for (unsigned k = 0; k < 11; ++k) if (k != 7) m.push_back(w32(zone + 4 * k, fields[k]));
            m.push_back(w16(zone + 0x1c, 2));
            for (unsigned p = 0; p < points; ++p) {
                const auto x = std::uint32_t(int(random() % 2000) - 400), y = std::uint32_t(int(random() % 2000) - 400);
                m.push_back(w32(data + 4 * p, x + 3 + (p * p) % 13)); m.push_back(w32(data + 0x40 + 4 * p, y + 5 + (p * 7) % 9));
                m.push_back(w32(data + 0x80 + 4 * p, x)); m.push_back(w32(data + 0xc0 + 4 * p, y));
                m.push_back(w16(data + 0x100 + 2 * p, x / 2)); m.push_back(w16(data + 0x120 + 2 * p, y / 2));
                m.push_back(w8(data + 0x140 + p, p % 3 == 0 ? 1 : 0)); m.push_back(w8(data + 0x150 + p, random() % 4));
            }
            m.push_back(w16(data + 0x160, 0)); m.push_back(w16(data + 0x162, 8)); m.push_back(w16(data + 0x170, 7)); m.push_back(w16(data + 0x172, 15));
        }
    }
};
}
}
