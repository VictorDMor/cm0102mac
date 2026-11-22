// The font engine, block `t2ktt_head` (0x107630e0..0x1076460c): cases for the stream constructors and their delete, the hint font and
// the cmap lookups. The fonts, memory objects, sfnts, hint fonts and input streams are the snapshot's own (found by the scaler's stamps:
// the font records' fields), so a case allocates and frees real heap blocks on both sides; a record a case needs for an edge (a stream
// of every kind, a cmap, a glyph, a graphic state, a program) is built in the scratch area, or the snapshot's own is changed by the
// case's memory writes (undone after it).
//
// The error paths. The game never runs out of memory, so the unwinding is provoked: the game's malloc answered null (an allocation
// fails, the allocator longjmps to the setjmp of the entry, which takes the transfer itself) and a block with a bad stamp (the release's
// longjmp). Labels start with "t2ktt_head:" (CM_FONT_ENGINE_BLOCK=t2ktt_head).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <cstdlib>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t new_substream = 0x107630f0, stream_remaining = 0x107630e0, new_input_stream = 0x107631c0, rewind = 0x10763280, seek = 0x10763290,
                        delete_stream = 0x107632a0, read_packed = 0x10763350, new_hint_font = 0x10763390, scale_shifted = 0x10763820,
                        scale_divided = 0x10763840, scale_fixed = 0x10763890, set_hint_size = 0x107638c0, load_glyph_program = 0x10763eb0,
                        delete_hint_font = 0x107642e0, scale_ratio = 0x107643d0, cmap4_lookup = 0x107644c0, cmap6_lookup = 0x107645a0;
constexpr std::uint32_t reporter = 0x107755c0, game_malloc = 0x10775130, game_free = 0x10775170, game_realloc = 0x107751b0;
constexpr std::uint32_t area = scratch_base + 0x70000;
constexpr std::uint32_t streams_at = area, data_at = area + 0x1000, error_cell = area + 0x100, record_at = area + 0x200, extra_at = area + 0x400, big_at = area + 0x2000;

struct Block { std::uint32_t user, size, slot; };
struct Font {
    std::uint32_t record, data, size, mem, input, sfnt, hint, maxp, directory;
    std::vector<Block> blocks;
};
std::uint32_t word(const Scenario& scenario, std::uint32_t at) { return scenario.r32(at); }

// The fonts of the snapshot (a distinct file size each): the scaler's stamps, the record that names it and what hangs off them.
std::vector<Font> fonts_of(const Scenario& scenario) {
    auto& s = scenario.s;
    std::vector<std::uint32_t> scalers;
    for (const auto& [low, size] : s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4)
            if (word(scenario, std::uint32_t(at)) == 0x5a1234a5 && word(scenario, std::uint32_t(at) + 0x5f4) == 0xa5fedc5a) scalers.push_back(std::uint32_t(at));
    }
    std::vector<Font> fonts;
    for (const auto& [low, size] : s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 4 <= std::uint64_t(low) + size; at += 4) {
            const auto value = word(scenario, std::uint32_t(at));
            if (at < low + 0x14 || std::ranges::find(scalers, value) == scalers.end()) continue;
            const auto record = std::uint32_t(at) - 0x14;
            if (s.memory.read<1>(record) != 1 || word(scenario, record + 8) != word(scenario, value + 4) || word(scenario, record + 0x10) != word(scenario, value + 0x144)) continue;
            Font font{record, word(scenario, record + 4), word(scenario, word(scenario, record + 0xc) + 8), word(scenario, record + 8), word(scenario, record + 0xc),
                      word(scenario, record + 0x10), 0, 0, 0, {}};
            font.hint = word(scenario, font.sfnt + 4);
            font.maxp = word(scenario, font.sfnt + 0x10);
            font.directory = word(scenario, font.sfnt);
            const auto table = word(scenario, font.mem + 0xc), capacity = word(scenario, font.mem + 8);
            for (std::uint32_t n = 0; n < capacity && n < 0x1f5; ++n) {
                const auto header = word(scenario, table + 4 * n);
                if (header && word(scenario, header) == 0xaa53c5aa && word(scenario, header + 8) == n) font.blocks.push_back({header + 0x10, word(scenario, header + 4), n});
            }
            fonts.push_back(std::move(font));
        }
    }
    std::vector<Font> distinct;
    for (auto& font : fonts)
        if (std::ranges::none_of(distinct, [&](const Font& other) { return other.size == font.size; })) distinct.push_back(std::move(font));
    return distinct;
}

std::vector<Write> concat(std::vector<Write> a, const std::vector<Write>& b) { append(a, b); return a; }
// A stream record built in the scratch area.
std::vector<Write> stream_of(std::uint32_t at, std::uint32_t base, std::uint32_t position, std::uint32_t length, std::uint32_t origin, std::uint32_t kind,
                             std::uint32_t mem) {
    return {w32(at, base), w32(at + 4, position), w32(at + 8, length), w32(at + 0xc, origin), w8(at + 0x10, kind), w32(at + 0x14, mem), w32(at + 0x18, 0x11111111), w32(at + 0x1c, 0x22222222)};
}
std::vector<Write> bytes_at(std::uint32_t at, const std::vector<std::uint8_t>& bytes) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < bytes.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), bytes[n]));
    return memory;
}
std::vector<Write> halves_at(std::uint32_t at, const std::vector<std::uint32_t>& halves) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < halves.size(); ++n) memory.push_back(w16(at + 2 * std::uint32_t(n), halves[n]));
    return memory;
}
void put16(std::vector<std::uint8_t>& bytes, std::size_t at, std::uint32_t value) { bytes[at] = std::uint8_t(value >> 8); bytes[at + 1] = std::uint8_t(value); }

// A format-4 cmap table: segments {start, end, delta, range offset (to the glyph array in bytes from the range offset's own word; 0 none)}
// and the glyph array after them.
std::vector<std::uint8_t> cmap_format_4(const std::vector<std::array<std::uint32_t, 4>>& segments, const std::vector<std::uint32_t>& glyphs) {
    const auto count = segments.size();
    std::vector<std::uint8_t> bytes(16 + 8 * count + 2 * glyphs.size() + 8, 0);
    put16(bytes, 0, 4);
    put16(bytes, 6, std::uint32_t(2 * count));
    for (std::size_t n = 0; n < count; ++n) {
        put16(bytes, 14 + 2 * n, segments[n][1]);
        put16(bytes, 16 + 2 * count + 2 * n, segments[n][0]);
        put16(bytes, 16 + 4 * count + 2 * n, segments[n][2]);
        put16(bytes, 16 + 6 * count + 2 * n, segments[n][3]);
    }
    for (std::size_t n = 0; n < glyphs.size(); ++n) put16(bytes, 16 + 8 * count + 2 * n, glyphs[n]);
    return bytes;
}
std::vector<std::uint8_t> cmap_format_6(std::uint32_t first, const std::vector<std::uint32_t>& glyphs) {
    std::vector<std::uint8_t> bytes(10 + 2 * glyphs.size(), 0);
    put16(bytes, 0, 6);
    put16(bytes, 6, first);
    put16(bytes, 8, std::uint32_t(glyphs.size()));
    for (std::size_t n = 0; n < glyphs.size(); ++n) put16(bytes, 10 + 2 * n, glyphs[n]);
    return bytes;
}
// A cmap record over `table` bytes at data_at: one encoding record (offset 0) or two (the second at +offset), chosen index `chosen`.
std::vector<Write> cmap_of(std::uint32_t at, std::uint32_t chosen, const std::vector<std::uint8_t>& table, std::uint32_t second_offset = 0) {
    std::vector<Write> memory{w32(at, 0), w16(at + 4, 0), w16(at + 6, 2), w32(at + 8, at + 0x40), w32(at + 0xc, data_at), w32(at + 0x10, std::uint32_t(table.size())),
                              w16(at + 0x14, chosen), w16(at + 0x16, table.size() >= 2 ? (std::uint32_t(table[0]) << 8 | table[1]) : 0)};
    memory.push_back(w32(at + 0x40, at + 0x60));
    memory.push_back(w32(at + 0x44, at + 0x70));
    memory.push_back(w16(at + 0x60, 3)); memory.push_back(w16(at + 0x62, 1)); memory.push_back(w32(at + 0x64, 0));
    memory.push_back(w16(at + 0x70, 3)); memory.push_back(w16(at + 0x72, 0)); memory.push_back(w32(at + 0x74, second_offset));
    append(memory, bytes_at(data_at, table));
    return memory;
}
}

FONT_ENGINE_CASES("t2ktt_head") {
    const auto fonts = fonts_of(scenario);
    if (fonts.empty()) return;
    std::mt19937 random(0x7a77);
    // The default lists are the edges; the whole grids (CM_FONT_ENGINE_EXHAUSTIVE=1) are run by hand once (the equivalence test's share stays short).
    const bool full = std::getenv("CM_FONT_ENGINE_EXHAUSTIVE") != nullptr;
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> answered = {{reporter, 0}};
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, const std::string& label) -> Case& {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = "t2ktt_head: " + label});
        cases.back().answers = answered;
        // The game's malloc, free and realloc dispatch on both sides, so each call of the unit's is paired and the answers reach both.
        cases.back().original_owners = {game_malloc, game_free, game_realloc};
        return cases.back();
    };
    // The allocator's failure: the game's malloc answered null (and made to dispatch, so the answer reaches both sides).
    const auto fail_malloc = [&](Case& item) {
        item.answers = {{game_malloc, 0}, {reporter, 0}};
        item.original_owners = {game_malloc, game_free, game_realloc};
    };
    constexpr std::uint32_t edges[] = {0, 1, 2, 5, 0x10, 0x7fffffff, 0x80000000, 0xfffffffe, 0xffffffff};

    // ---- 0x107630e0: what remains of a stream ----
    for (const auto origin : edges)
        for (const auto length : edges)
            add(stream_remaining, {{3, streams_at}}, stream_of(streams_at, data_at, 0, length, origin, 2, 0), "remaining origin " + hex(origin) + " length " + hex(length));

    // ---- 0x10763280, 0x10763290: rewind and seek ----
    for (const auto origin : edges) {
        add(rewind, {{3, streams_at}}, stream_of(streams_at, data_at, 0x55, 0x100, origin, 3, 0), "rewind origin " + hex(origin));
        for (const auto offset : edges)
            add(seek, {{3, streams_at}, {4, offset}}, stream_of(streams_at, data_at, 0x55, 0x100, origin, 3, 0), "seek origin " + hex(origin) + " offset " + hex(offset));
    }

    // ---- 0x10763350: the 7-bit groups ----
    {
        const std::vector<std::vector<std::uint8_t>> sequences = {
            {0x00}, {0x01}, {0x7f}, {0x80, 0x01}, {0xff, 0x7f}, {0x80, 0x80, 0x01}, {0xff, 0xff, 0xff, 0x7f}, {0xff, 0xff, 0xff, 0xff, 0x0f},
            {0xff, 0xff, 0xff, 0xff, 0x7f}, {0x80, 0x80, 0x80, 0x80, 0x80, 0x01}, {0xff, 0xff, 0xff, 0xff, 0xff, 0x7f},
            {0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x0b}, {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00}};
        for (const auto& sequence : sequences) {
            for (const std::uint32_t position : {0u, 3u})
                add(read_packed, {{3, streams_at}}, concat(stream_of(streams_at, data_at, position, 0x100, 0, 3, 0), bytes_at(data_at + position, sequence)),
                    "packed " + std::to_string(sequence.size()) + " bytes from " + std::to_string(position) + " first " + hex(sequence[0]));
        }
        for (unsigned n = 0; n < (full ? 150u : 40u); ++n) {
            std::vector<std::uint8_t> sequence(12);
            for (auto& byte : sequence) byte = std::uint8_t(random());
            sequence[random() % 8 + 1] &= 0x7f;
            for (auto& byte : sequence) if (random() % 3 == 0) byte |= 0x80;
            add(read_packed, {{3, streams_at}}, concat(stream_of(streams_at, data_at, 2, 0x100, 0, 3, 0), bytes_at(data_at + 2, sequence)), "packed random " + std::to_string(n));
        }
    }

    // ---- 0x10763820, 0x10763840, 0x10763890: the scale callbacks ----
    {
        const std::uint32_t values[] = {0, 1, 2, 3, 0xffffffffu, 0xfffffffeu, 0x7fff, 0x8000, 0xffff8000u, 0x12345, 0x7fffffffu, 0x80000000u, 0xdeadbeefu};
        for (const auto mult : full ? std::vector<std::uint32_t>{0u, 1u, 3u, 0x100u, 0xffffffffu, 0x7fffffffu} : std::vector<std::uint32_t>{3u, 0xffffffffu}) {
            for (const auto add_value : full ? std::vector<std::uint32_t>{0u, 0x20u, 0xfffffff0u, 0x4000u} : std::vector<std::uint32_t>{0u, 0x20u}) {
                for (const auto shift : full ? std::vector<std::uint32_t>{0u, 1u, 6u, 14u, 31u, 32u, 0xffffu} : std::vector<std::uint32_t>{0u, 6u, 32u}) {
                    const auto gs = concat({w32(record_at + 0xa8, mult), w32(record_at + 0xb0, add_value), w16(record_at + 0xb4, shift), w32(record_at + 0xac, 7)}, {});
                    for (const auto value : {1u, 0xffffffffu, 0x5555u, 0x80000000u})
                        add(scale_shifted, {{3, record_at}, {4, value}}, gs, "scale shifted mult " + hex(mult) + " add " + hex(add_value) + " shift " + hex(shift) + " value " + hex(value));
                }
                for (const auto divisor : full ? std::vector<std::uint32_t>{1u, 2u, 3u, 64u, 0xfffffffbu, 0xffffffffu, 0u} : std::vector<std::uint32_t>{3u, 0xfffffffbu, 0u}) {
                    const auto gs = std::vector<Write>{w32(record_at + 0xa8, mult), w32(record_at + 0xb0, add_value), w32(record_at + 0xac, divisor)};
                    for (const auto value : values)
                        add(scale_divided, {{3, record_at}, {4, value}}, gs, "scale divided mult " + hex(mult) + " add " + hex(add_value) + " div " + hex(divisor) + " value " + hex(value));
                }
            }
        }
        for (const auto unit : {0u, 0x10000u, 0x8000u, 0xffff0000u, 0x12345678u, 0x7fffffffu, 0x80000000u})
            for (const auto value : values)
                add(scale_fixed, {{3, record_at}, {4, value}, {5, 0x51}, {6, 0x52}, {7, 0x53}, {8, 0x54}}, {w32(record_at + 0xa4, unit)}, "scale fixed unit " + hex(unit) + " value " + hex(value));
    }

    // ---- 0x107643d0: the ratio of two sizes ----
    {
        std::vector<std::uint32_t> sizes = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 17, 20, 22, 24, 32, 48, 64, 72, 100, 127, 128, 255, 256, 511, 512, 1000, 1024, 2048, 4096,
                                            0x7fff, 0x8000, 0xffff, 0x10000, 0x7ffffff, 0x1000000};
        const std::vector<Write> garbage = {w32(record_at, 0xa1a2a3a4), w32(record_at + 4, 0xb1b2b3b4), w32(record_at + 8, 0xc1c2c3c4), w32(record_at + 0xc, 0xd1d2d3d4), w32(record_at + 0x10, 0xe1e2e3e4)};
        if (!full) sizes = {1, 3, 9, 14, 22, 72, 1000, 2048, 0x8000, 0x10000};
        for (const auto a : sizes)
            for (const auto b : sizes) add(scale_ratio, {{3, a}, {4, b}, {5, record_at}}, garbage, "ratio " + std::to_string(a) + ", " + std::to_string(b));
        for (const auto a : {0u, 0xffffffffu, 0x80000000u, 0x7fffffffu, 0x20000000u, 0xfe000000u})
            for (const auto b : {1u, 2u, 0xffffffffu, 0x80000000u, 0x7fffffffu, 0xfffffffeu, 0x40000000u, 3u, 0u}) {
                // A zero numerator (a << 6 wraps to 0) over a zero divisor halves two zeros for ever (as the original does): left out.
                if (std::uint32_t(a << 6) == 0 && b == 0) continue;
                add(scale_ratio, {{3, a}, {4, b}, {5, record_at}}, garbage, "ratio edge " + hex(a) + ", " + hex(b));
            }
    }

    // ---- the cmap lookups: formats 4 and 6 over built tables, and over each font's own cmap table ----
    {
        // Segments: [0x20..0x7e delta -0x1f], [0x80..0x90 range offset into the glyph array], [0xffff..0xffff delta 1].
        const auto table4 = cmap_format_4({{0x20, 0x7e, 0xffe1, 0}, {0x80, 0x90, 0, 4}, {0xffff, 0xffff, 1, 0}}, {0x111, 0x222, 0x333, 0x0, 0x555, 0x666});
        const auto table4_second = cmap_format_4({{0x00, 0x30, 5, 0}, {0xff00, 0xffff, 0x10, 0}}, {});
        for (const std::uint32_t code : {0u, 0x1fu, 0x20u, 0x21u, 0x41u, 0x7eu, 0x7fu, 0x80u, 0x81u, 0x85u, 0x90u, 0x91u, 0xfffeu, 0xffffu, 0x10020u, 0x10041u, 0xffff0020u, 0x30u, 0x39u}) {
            add(cmap4_lookup, {{3, record_at}, {4, code}, {5, 1}, {6, 2}, {7, 3}, {8, 4}}, cmap_of(record_at, 0, table4), "cmap4 first record code " + hex(code));
            add(cmap4_lookup, {{3, record_at}, {4, code}}, cmap_of(record_at, 1, table4_second, 0), "cmap4 second record code " + hex(code));
        }
        for (unsigned n = 0; n < (full ? 200u : 20u); ++n) {
            std::vector<std::array<std::uint32_t, 4>> segments;
            std::vector<std::uint32_t> glyphs;
            const auto count = 1 + random() % 5;
            std::uint32_t start = random() % 50;
            for (unsigned k = 0; k < count; ++k) {
                const auto end = start + random() % 40;
                const bool ranged = random() % 3 == 0;
                segments.push_back({start, end, std::uint32_t(random() & 0xffff), ranged ? std::uint32_t(2 * (count - k) + 2 * glyphs.size()) : 0});
                if (ranged) for (std::uint32_t g = start; g <= end; ++g) glyphs.push_back(random() & 0xffff);
                start = end + 1 + random() % 10;
            }
            segments.back()[1] = 0xffff;
            for (const std::uint32_t code : {start - 1, start / 2, 0u, 0x20u, 0xffffu, std::uint32_t(random() % 100)})
                add(cmap4_lookup, {{3, record_at}, {4, code}}, cmap_of(record_at, 0, cmap_format_4(segments, glyphs)), "cmap4 random " + std::to_string(n) + " code " + hex(code));
        }
        const auto table6 = cmap_format_6(0x30, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
        for (const std::uint32_t code : {0u, 0x2fu, 0x30u, 0x31u, 0x35u, 0x3au, 0x3bu, 0x3cu, 0x10030u, 0xffffffffu, 0x7fffffffu, 0x80000030u, 0x30u + 0x10000u})
            add(cmap6_lookup, {{3, record_at}, {4, code}, {9, 0x99}, {10, 0xaa}}, cmap_of(record_at, 0, table6), "cmap6 code " + hex(code));
        add(cmap6_lookup, {{3, record_at}, {4, 5}}, cmap_of(record_at, 0, cmap_format_6(0, {})), "cmap6 empty table");
        add(cmap6_lookup, {{3, record_at}, {4, 0}}, cmap_of(record_at, 0, cmap_format_6(0xffff, {9, 8})), "cmap6 first 0xffff");
        for (const auto& font : fonts) {
            // The font's own cmap table: its encoding records and the offset of one with format 4.
            const auto u16 = [&](std::uint32_t at) { return std::uint32_t(scenario.u16(font.data + at)); };
            std::uint32_t cmap_offset = 0;
            for (std::uint32_t n = 0, tables = u16(4); n < tables; ++n)
                if (word(scenario, font.data + 12 + 16 * n) == 0x636d6170) cmap_offset = word(scenario, font.data + 20 + 16 * n);
            if (!cmap_offset) continue;
            const auto records = u16(cmap_offset + 2);
            std::vector<Write> memory{w32(record_at, 0), w16(record_at + 6, records), w32(record_at + 8, record_at + 0x40), w32(record_at + 0xc, font.data + cmap_offset),
                                      w32(record_at + 0x10, 0x1000)};
            for (std::uint32_t n = 0; n < records && n < 8; ++n) {
                memory.push_back(w32(record_at + 0x40 + 4 * n, record_at + 0x60 + 8 * n));
                memory.push_back(w16(record_at + 0x60 + 8 * n, u16(cmap_offset + 4 + 8 * n)));
                memory.push_back(w16(record_at + 0x62 + 8 * n, u16(cmap_offset + 6 + 8 * n)));
                memory.push_back(w32(record_at + 0x64 + 8 * n, word(scenario, font.data + cmap_offset + 8 + 8 * n)));
            }
            for (std::uint32_t chosen = 0; chosen < records && chosen < 8; ++chosen) {
                if (u16(cmap_offset + word(scenario, font.data + cmap_offset + 8 + 8 * chosen)) != 4) continue;
                auto chosen_memory = memory;
                chosen_memory.push_back(w16(record_at + 0x14, chosen));
                // The codes of the table's segments, and past them.
                const auto table = cmap_offset + word(scenario, font.data + cmap_offset + 8 + 8 * chosen);
                const auto segments = u16(table + 6) / 2;
                std::vector<std::uint32_t> codes = {0, 0x1f, 0x20, 0x7f, 0x80, 0xff, 0x100, 0x2000, 0xffff, 0x10000};
                for (std::uint32_t seg = 0; seg < segments; ++seg) {
                    const auto end = u16(table + 14 + 2 * seg), start = u16(table + 16 + 2 * segments + 2 * seg);
                    codes.push_back(start); codes.push_back(end); codes.push_back(start + (end - start) / 2); codes.push_back(end + 1); codes.push_back(start - 1);
                }
                for (const auto code : codes)
                    add(cmap4_lookup, {{3, record_at}, {4, code}}, chosen_memory, "cmap4 of the font " + hex(font.size) + " record " + std::to_string(chosen) + " code " + hex(code));
            }
        }
    }

    // ---- 0x107631c0, 0x107630f0: the stream constructors ----
    for (const auto& font : fonts) {
        const std::vector<Write> errors = {w32(error_cell, 0xdeadbeef)};
        for (const std::uint32_t length : {0u, 1u, 0x20u, 0x1234u, 0x7fffffffu, 0xffffffffu}) {
            add(new_input_stream, {{3, font.mem}, {4, font.data}, {5, length}, {6, error_cell}}, errors, "new stream length " + hex(length) + " with the error cell (font " + hex(font.size) + ")");
            add(new_input_stream, {{3, font.mem}, {4, data_at}, {5, length}, {6, 0}}, {}, "new stream length " + hex(length) + " without the error cell");
        }
        {
            auto& item = add(new_input_stream, {{3, font.mem}, {4, font.data}, {5, 0x100}, {6, error_cell}}, errors, "new stream, malloc fails, error cell");
            fail_malloc(item);
        }
        {
            auto& item = add(new_input_stream, {{3, font.mem}, {4, font.data}, {5, 0x100}, {6, 0}}, {}, "new stream, malloc fails, no error cell");
            fail_malloc(item);
        }
        for (const std::uint32_t pooled : {0u, 1u, 0x100u})
            for (const auto& [offset, length] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0, 0}, {0, 0x10}, {0x100, 0x200}, {0xffffffff, 2}, {0x7fffffff, 0x7fffffff}, {0x20, 0xffffffe0}}) {
                add(new_substream, {{3, font.mem}, {4, font.input}, {5, offset}, {6, length}, {7, pooled}, {8, 0}}, {}, "substream " + hex(offset) + "+" + hex(length) + " pooled " + hex(pooled) + " without the error cell");
                add(new_substream, {{3, font.mem}, {4, font.input}, {5, offset}, {6, length}, {7, pooled}, {8, error_cell}}, errors, "substream " + hex(offset) + "+" + hex(length) + " pooled " + hex(pooled) + " with the error cell");
            }
        for (const std::uint32_t pooled : {0u, 1u}) {
            auto& item = add(new_substream, {{3, font.mem}, {4, font.input}, {5, 0x40}, {6, 0x80}, {7, pooled}, {8, error_cell}}, errors, "substream pooled " + hex(pooled) + ", malloc fails, error cell");
            fail_malloc(item);
        }
        {
            auto& item = add(new_substream, {{3, font.mem}, {4, font.input}, {5, 0x40}, {6, 0x80}, {7, 0}, {8, 0}}, {}, "substream, malloc fails, no error cell");
            fail_malloc(item);
        }
    }

    // ---- 0x107632a0: the delete: every kind, the memory object's own stream, the unwinding ----
    for (const auto& font : fonts) {
        if (font.blocks.size() < 4) continue;
        // Blocks of 0x20 bytes or more can hold a stream record; the others are data.
        std::vector<Block> roomy;
        for (const auto& block : font.blocks) if (block.size >= 0x20 && block.user != font.input) roomy.push_back(block);
        if (roomy.size() < 3) continue;
        const std::vector<Write> errors = {w32(error_cell, 0xdeadbeef)};
        add(delete_stream, {{3, 0}, {4, 0}}, {}, "delete null stream");
        add(delete_stream, {{3, 0}, {4, error_cell}}, errors, "delete null stream with the error cell");
        const auto stream = roomy[0].user, data = roomy[1].user;
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 0xffu}) {
            for (const std::uint32_t err : {0u, error_cell}) {
                add(delete_stream, {{3, stream}, {4, err}}, concat(stream_of(stream, data, 0, 0x10, 0, kind, font.mem), errors),
                    "delete kind " + hex(kind) + (err ? " with the error cell" : " without") + " (font " + hex(font.size) + ")");
            }
        }
        // The memory object's own stream: flagged free instead of released.
        for (const std::uint32_t kind : {2u, 3u})
            add(delete_stream, {{3, stream}, {4, 0}}, concat(stream_of(stream, data, 0, 0x10, 0, kind, font.mem), {w32(font.mem + 0x130, stream), w32(font.mem + 0x160, 0x77)}),
                "delete the memory object's own stream, kind " + hex(kind));
        add(delete_stream, {{3, stream}, {4, error_cell}}, concat(stream_of(stream, data, 0, 0x10, 0, 1, font.mem), {w32(font.mem + 0x130, stream), w32(font.mem + 0x160, 0x77), w32(error_cell, 0xdeadbeef)}),
            "delete kind 1 where the object's own stream is the stream");
        // The unwinding: the stream's block has a bad stamp (the release raises 0x2719), the data's, the object's table slot is wrong.
        const auto header = [&](std::uint32_t user) { return user - 0x10; };
        for (const std::uint32_t kind : {1u, 2u}) {
            add(delete_stream, {{3, stream}, {4, error_cell}}, concat(stream_of(stream, data, 0, 0x10, 0, kind, font.mem), {w32(error_cell, 0xdeadbeef), w32(header(stream), 0x12345678)}),
                "delete kind " + hex(kind) + " with the stream's block stamp bad, error cell");
            add(delete_stream, {{3, stream}, {4, 0}}, concat(stream_of(stream, data, 0, 0x10, 0, kind, font.mem), {w32(header(stream), 0x12345678)}),
                "delete kind " + hex(kind) + " with the stream's block stamp bad, no error cell");
        }
        add(delete_stream, {{3, stream}, {4, error_cell}}, concat(stream_of(stream, data, 0, 0x10, 0, 1, font.mem), {w32(error_cell, 0xdeadbeef), w32(header(data), 0x12345678)}),
            "delete kind 1 with the data block's stamp bad, error cell");
        add(delete_stream, {{3, stream}, {4, error_cell}}, concat(stream_of(stream, data, 0, 0x10, 0, 1, font.mem), {w32(error_cell, 0xdeadbeef), w32(header(stream) + 8, 0x1f4)}),
            "delete kind 1 with the stream's slot index wrong, error cell");
        // A real stream (the font's own input stream) deleted.
        add(delete_stream, {{3, font.input}, {4, 0}}, {}, "delete the font's input stream");
        add(delete_stream, {{3, font.input}, {4, error_cell}}, errors, "delete the font's input stream with the error cell");
    }

    // ---- 0x10763390: the hint font of a font ----
    for (const auto& font : fonts) {
        add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}}, {}, "new hint font of the font " + hex(font.size));
        if (!font.maxp || !font.directory) continue;
        // maxp variants: more twilight points, composite points above the points, storage and definitions, the stack, a bigger table.
        const auto m = font.maxp;
        const std::vector<std::pair<std::string, std::vector<Write>>> variants = {
            {"twilight 5", {w16(m + 0x14, 5)}}, {"twilight 0", {w16(m + 0x14, 0)}}, {"twilight -1", {w16(m + 0x14, 0xffff)}},
            {"composite points above", {w16(m + 0xa, 3), w16(m + 0xe, 40)}}, {"points above", {w16(m + 0xa, 60), w16(m + 0xe, 40)}},
            {"storage 9", {w16(m + 0x16, 9)}}, {"storage 0", {w16(m + 0x16, 0)}}, {"storage -3", {w16(m + 0x16, 0xfffd)}},
            {"definitions", {w16(m + 0x18, 6), w16(m + 0x1a, 3)}}, {"stack 20", {w16(m + 0x1c, 20)}},
            {"all", {w16(m + 0x14, 12), w16(m + 0x16, 17), w16(m + 0x18, 2), w16(m + 0x1a, 1), w16(m + 0x1c, 33), w16(m + 0xa, 9), w16(m + 0xe, 4)}}};
        for (const auto& [name, writes] : variants)
            add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}}, writes, "new hint font with maxp " + name + " (font " + hex(font.size) + ")");
        // Tables the directory names: cvt, prep and fpgm over bytes of our own (instructions the interpreter runs: SVTCA, then a push and an add).
        const auto entries = word(scenario, font.directory + 0x10);
        const auto count = scenario.u16(font.directory + 8);
        if (count < 3) continue;
        const auto entry = [&](std::uint32_t n) { return word(scenario, entries + 4 * n); };
        const std::vector<std::uint8_t> program = {0x00, 0x01, 0xb0, 0x05, 0x20, 0x21, 0x00}, cvt_bytes = {0, 5, 0xff, 0xfe, 0, 0x30, 0x7f, 0xff, 0x80, 0, 0, 1};
        const auto table_of = [&](std::uint32_t n, std::uint32_t tag, std::uint32_t at, std::uint32_t length) {
            return std::vector<Write>{w32(entry(n) + 4, tag), w32(entry(n) + 0xc, at - font.data), w32(entry(n) + 0x10, length)};
        };
        add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}}, concat(table_of(0, 0x63767420, data_at, 12), bytes_at(data_at, cvt_bytes)), "new hint font with a cvt table (font " + hex(font.size) + ")");
        add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}}, concat(table_of(0, 0x63767420, data_at, 13), bytes_at(data_at, cvt_bytes)), "new hint font with an odd cvt table");
        add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}}, table_of(0, 0x63767420, data_at, 0), "new hint font with an empty cvt table");
        add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}}, concat(table_of(1, 0x70726570, extra_at, 7), bytes_at(extra_at, program)), "new hint font with a prep table");
        add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}}, concat(table_of(1, 0x6670676d, extra_at, 7), bytes_at(extra_at, program)), "new hint font with an fpgm table");
        add(new_hint_font, {{3, font.mem}, {4, font.input}, {5, font.sfnt}},
            concat(concat(table_of(0, 0x63767420, data_at, 12), concat(table_of(1, 0x70726570, extra_at, 7), table_of(2, 0x6670676d, extra_at + 8, 7))),
                   concat(bytes_at(data_at, cvt_bytes), concat(bytes_at(extra_at, program), bytes_at(extra_at + 8, program)))),
            "new hint font with cvt, prep and fpgm tables (font " + hex(font.size) + ")");
    }
    // The reporter for a missing maxp (the original then reads through it).
    add(new_hint_font, {{3, fonts[0].mem}, {4, fonts[0].input}, {5, fonts[0].sfnt}}, {w32(fonts[0].sfnt + 0x10, 0)}, "new hint font without maxp, answered");
    {
        auto& item = add(new_hint_font, {{3, fonts[0].mem}, {4, fonts[0].input}, {5, fonts[0].sfnt}}, {w32(fonts[0].sfnt + 0x10, 0)}, "new hint font without maxp, real reporter");
        item.answers.clear();
    }

    // ---- 0x107638c0, 0x10763eb0, 0x107642e0: the hint font's size, glyph program and release ----
    for (const auto& font : fonts) {
        if (!font.hint) continue;
        const auto hint = font.hint, gs = hint + 0x18;
        const std::vector<std::uint8_t> program = {0x00, 0x01, 0xb0, 0x05, 0x20, 0x21, 0x00};
        // The cvt of our own: halfwords in, words out.
        const std::vector<Write> cvt = concat(halves_at(extra_at, {0, 1, 0xffff, 100, 0x7fff, 0x8000, 0xff00, 5, 6, 7, 8, 9, 10}),
                                              {w32(hint + 0xc, extra_at), w32(gs + 8, extra_at + 0x100)});
        std::vector<Write> cvt13 = cvt;
        cvt13.push_back(w32(hint + 0x10, 13));
        std::vector<Write> cvt20 = cvt;
        cvt20.push_back(w32(hint + 0x10, 20));
        std::vector<Write> cvt3 = cvt;
        cvt3.push_back(w32(hint + 0x10, 3));
        const std::vector<Write> prep = concat(bytes_at(extra_at + 0x300, program), {w32(gs + 0x9c, extra_at + 0x300), w32(hint + 0xe8, 7)});
        const std::vector<Write> no_prep = {w32(gs + 0x9c, 0)};
        const std::vector<std::pair<std::int32_t, std::int32_t>> sizes = full ? std::vector<std::pair<std::int32_t, std::int32_t>>{{9, 9}, {14, 9}, {9, 14}, {20, 20}, {1000, 1000}, {5, 6}, {0, 0}, {-1, 7}, {0x7fff, 3}, {512, 512}, {64, 48}, {1, 1}}
                                                                       : std::vector<std::pair<std::int32_t, std::int32_t>>{{9, 9}, {14, 9}, {9, 14}, {20, 20}, {0, 0}, {-1, 7}, {0x7fff, 3}};
        add(set_hint_size, {{3, 0}, {4, 9}, {5, 9}}, {}, "size of no hint font");
        // The same size set again returns at once.
        add(set_hint_size, {{3, hint}, {4, 0x1234}, {5, 0x4321}}, {w32(hint + 4, 0x1234), w32(hint + 8, 0x4321)}, "size unchanged");
        add(set_hint_size, {{3, hint}, {4, 0x1234}, {5, 0x4321}}, {w32(hint + 4, 0x1234), w32(hint + 8, 0x4322)}, "size, only y changed");
        for (const auto& [x, y] : sizes)
            for (const std::uint32_t upem : full ? std::vector<std::uint32_t>{2048u, 1000u, 3u, 64u, 0x7fffu, 0u, 17u, 2u} : std::vector<std::uint32_t>{2048u, 1000u, 3u, 0u}) {
                // A zero size over a zero em: the ratio's loop halves two zeros for ever (as in the original).
                if (x == 0 && y == 0 && upem == 0) continue;
                for (const auto& [name, extra] : std::vector<std::pair<std::string, std::vector<Write>>>{{"font's own cvt and prep", {}}, {"cvt 13", concat(cvt13, no_prep)}, {"cvt 20", concat(cvt20, no_prep)}, {"cvt 3, prep", concat(cvt3, prep)}, {"no cvt, prep", prep}})
                    add(set_hint_size, {{3, hint}, {4, std::uint32_t(x)}, {5, std::uint32_t(y)}}, concat({w16(hint + 0xe4, upem), w32(hint + 4, 0xfffffff0), w32(hint + 0xd0, 0x1234), w32(hint + 0xd4, 0x4321)}, extra),
                        "size " + std::to_string(x) + "x" + std::to_string(y) + " upem " + hex(upem) + " " + name + " (font " + hex(font.size) + ")");
            }
        // Twilight zones cleared at the font's own count, and a count of our own.
        for (const std::uint32_t twilight : {1u, 2u, 3u, 9u, 12u})
            add(set_hint_size, {{3, hint}, {4, 12}, {5, 12}}, concat(cvt3, {w16(hint + 0xe6, twilight), w32(hint + 0xf4, extra_at + 0x800), w32(hint + 0xf8, extra_at + 0x900), w32(hint + 0xfc, extra_at + 0xa00), w32(hint + 0x100, extra_at + 0xb00),
                                                                           w32(hint + 0x104, extra_at + 0xc00), w32(hint + 0x108, extra_at + 0xd00), w32(hint + 0x10c, extra_at + 0xe00), w32(hint + 0x11c, extra_at + 0xf00), w32(hint + 4, 0)}),
                "size with " + std::to_string(twilight) + " twilight points");

        // The glyph program: a glyph record of our own over the font's own hint font (the zone block of the hint font is the font's).
        const auto glyph = big_at, zone = big_at + 0x1000;
        const auto glyph_record = [&](std::uint32_t points, std::uint32_t instructions, std::uint32_t length) {
            std::vector<Write> memory = {w32(glyph + 0x64, instructions), w32(glyph + 0x68, length), w16(glyph + 0x38, points), w16(glyph + 0x36, 2),
                                         w32(glyph + 0x50, glyph + 0x100), w32(glyph + 0x54, glyph + 0x200), w32(glyph + 0x44, glyph + 0x300), w32(glyph + 0x48, glyph + 0x340), w32(glyph + 0x4c, glyph + 0x380),
                                         w32(glyph + 0x3c, glyph + 0x3a0), w32(glyph + 0x40, glyph + 0x3b0), w32(hint + 0x14, zone)};
            for (std::uint32_t n = 0; n < points + 4; ++n) {
                memory.push_back(w32(glyph + 0x100 + 4 * n, std::uint32_t(random() % 4000) - 200));
                memory.push_back(w32(glyph + 0x200 + 4 * n, std::uint32_t(random() % 4000) - 200));
                memory.push_back(w16(glyph + 0x300 + 2 * n, std::uint32_t(random() % 2000) - 100));
                memory.push_back(w16(glyph + 0x340 + 2 * n, std::uint32_t(random() % 2000) - 100));
                memory.push_back(w8(glyph + 0x380 + n, random()));
            }
            append(memory, bytes_at(glyph + 0x3c0, program));
            return memory;
        };
        add(load_glyph_program, {{3, 0}, {4, glyph}}, glyph_record(5, glyph + 0x3c0, 7), "glyph program, no hint font");
        add(load_glyph_program, {{3, hint}, {4, glyph}}, glyph_record(5, 0, 7), "glyph program, no instructions");
        for (const std::uint32_t length : {0u, 0xffffffffu, 0x80000000u})
            add(load_glyph_program, {{3, hint}, {4, glyph}}, glyph_record(5, glyph + 0x3c0, length), "glyph program, length " + hex(length));
        for (const std::uint32_t points : {1u, 3u, 5u, 9u, 12u})
            for (const auto& [x_scale, y_scale] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0x10000, 0x10000}, {0x8000, 0x10000}, {0x10000, 0x18000}, {0x12345, 0xffff0001u}, {0, 0x10001}})
                add(load_glyph_program, {{3, hint}, {4, glyph}, {9, 0x99}, {10, 0xaa}}, concat(glyph_record(points, glyph + 0x3c0, 7), {w32(hint + 0xd0, x_scale), w32(hint + 0xd4, y_scale)}),
                    "glyph program of " + std::to_string(points) + " points, scales " + hex(x_scale) + " " + hex(y_scale) + " (font " + hex(font.size) + ")");
        add(load_glyph_program, {{3, hint}, {4, glyph}}, glyph_record(0, glyph + 0x3c0, 7), "glyph program of no points");

        // Deleting the hint font frees its blocks (the font's own, real).
        add(delete_hint_font, {{3, 0}}, {}, "delete no hint font");
        add(delete_hint_font, {{3, hint}}, {}, "delete the font's hint font " + hex(font.size));
        add(delete_hint_font, {{3, hint}}, {w32(gs + 0x9c, 0)}, "delete the hint font with no prep block");
        add(delete_hint_font, {{3, hint}}, {w32(hint + 0x14, 0)}, "delete the hint font with no zone block");
    }
}
}
