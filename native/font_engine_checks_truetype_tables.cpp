// The font engine, block `truetype_tables` (0x10764610..0x10765810): cases for the table readers (cmap, kern, hhea/vhea, head, maxp, loca),
// the glyph reader and the directory lookups. The tables are bytes built in the scratch area behind a stream record (the readers' only
// input), allocated on the snapshot's own memory objects (found by the scaler's stamps, as in the t2ktt_head cases) so each case
// allocates real heap blocks on both sides; the directory lookups run over the snapshot's own sfnts.
// Table shapes: every cmap pass of the choice (the wanted platform and encoding with formats 0, 4, 6, the wildcards 0xffff, the by-format
// fallbacks, no format at all: the error 0x2725 answered), kern headers of both kinds, glyph outlines with every flag combination
// (short, same, word deltas, repeats) and composites with every transform and the instructions kept or skipped. Labels start with
// "truetype_tables:" (CM_FONT_ENGINE_BLOCK=truetype_tables).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t new_cmap = 0x10764610, new_kern_subtable = 0x107649a0, new_kern = 0x10764b10, new_hhea = 0x10764bf0, new_head = 0x10764d10,
                        new_maxp = 0x10764e50, new_loca = 0x10764f60, new_glyph = 0x10765060, find_table = 0x10765730, new_table_stream = 0x10765780;
constexpr std::uint32_t reporter = 0x107755c0, game_malloc = 0x10775130, game_free = 0x10775170, game_realloc = 0x107751b0, raise_error = 0x10767430, realloc_entry = 0x10767a00;
constexpr std::uint32_t area = scratch_base + 0x90000;
constexpr std::uint32_t stream_at = area, data_at = area + 0x1000;

struct Font { std::uint32_t mem, sfnt, directory, size, data; };
std::uint32_t word(const Scenario& scenario, std::uint32_t at) { return scenario.r32(at); }
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
            const Font font{word(scenario, record + 8), word(scenario, record + 0x10), word(scenario, word(scenario, record + 0x10)), word(scenario, word(scenario, record + 0xc) + 8), word(scenario, record + 4)};
            if (std::ranges::none_of(fonts, [&](const Font& other) { return other.size == font.size; })) fonts.push_back(font);
        }
    }
    return fonts;
}
// The stream over `bytes` at data_at: a record {base, position 0, length, origin 0, kind 2}.
std::vector<Write> stream_over(const std::vector<std::uint8_t>& bytes, std::uint32_t length_override = 0xffffffff, std::uint32_t position = 0) {
    std::vector<Write> memory{w32(stream_at, data_at), w32(stream_at + 4, position), w32(stream_at + 8, length_override == 0xffffffff ? std::uint32_t(bytes.size()) : length_override), w32(stream_at + 0xc, 0), w8(stream_at + 0x10, 2)};
    for (std::size_t n = 0; n < bytes.size(); ++n) memory.push_back(w8(data_at + std::uint32_t(n), bytes[n]));
    return memory;
}
struct Bytes {
    std::vector<std::uint8_t> data;
    Bytes& u8(std::uint32_t v) { data.push_back(std::uint8_t(v)); return *this; }
    Bytes& u16(std::uint32_t v) { u8(v >> 8); return u8(v); }
    Bytes& u32(std::uint32_t v) { u16(v >> 16); return u16(v); }
    Bytes& many(std::uint32_t count, std::mt19937& random) { for (std::uint32_t n = 0; n < count; ++n) u8(random()); return *this; }
};
}

FONT_ENGINE_CASES("truetype_tables") {
    const auto fonts = fonts_of(scenario);
    if (fonts.empty()) return;
    const auto& font = fonts[0];
    std::mt19937 random(0x7ab1);
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> answered = {{reporter, 0}, {raise_error, 0}};
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, const std::string& label) -> Case& {
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = "truetype_tables: " + label});
        cases.back().answers = answered;
        // The game's malloc, free and realloc dispatch on both sides, so each call of the unit's is paired and the answers reach both.
        cases.back().original_owners = {game_malloc, game_free, game_realloc};
        return cases.back();
    };
    // (No case fails the game's malloc here: the failing allocation longjmps to the setjmp of an API entry that is not on this stack, so what
    // follows is not defined; the stream constructors' cases, which hold their own setjmp, take that path.)

    // ---- the fixed-layout tables: hhea (also vhea), head, maxp ----
    for (const auto entry : {new_hhea, new_head, new_maxp}) {
        const std::string name = entry == new_hhea ? "hhea" : entry == new_head ? "head" : "maxp";
        for (unsigned n = 0; n < 40; ++n) {
            Bytes table;
            table.many(80, random);
            if (n == 0) for (auto& byte : table.data) byte = 0;
            if (n == 1) for (auto& byte : table.data) byte = 0xff;
            if (n == 2) for (auto& byte : table.data) byte = 0x80;
            add(entry, {{3, font.mem}, {4, stream_at}}, stream_over(table.data), name + " random " + std::to_string(n));
        }
        // A stream that ends inside the table: the reporter answers and the original goes on.
        for (const std::uint32_t length : {0u, 3u, 5u, 9u, 21u, 33u})
            add(entry, {{3, font.mem}, {4, stream_at}}, stream_over(Bytes{}.many(80, random).data, length), name + " stream ends at " + std::to_string(length));
    }

    // ---- loca ----
    for (const std::uint32_t format : {0u, 1u, 2u, 0xffffu, 0x10000u, 0x10001u, 0xffff0000u})
        for (const std::uint32_t length : {0u, 1u, 2u, 3u, 4u, 8u, 20u, 64u, 0x80000000u, 0xfffffff0u}) {
            if (length >= 0x1000 && format != 0 && format != 1) {}
            if (length > 200 && length < 0xf0000000u) continue;
            if (length >= 0x80000000u && format > 1 && format != 0x10000u && format != 0x10001u) continue;
            add(new_loca, {{3, font.mem}, {4, stream_at}, {5, format}, {6, length}}, stream_over(Bytes{}.many(200, random).data), "loca format " + hex(format) + " length " + hex(length));
        }

    // ---- kern ----
    {
        const auto subtable_ms = [&](std::uint32_t format, std::uint32_t length, std::uint32_t pairs, std::uint32_t entries) {
            Bytes b;
            b.u16(format).u16(length).u16(1).u16(pairs).u16(0x10).u16(2).u16(0x20);
            for (std::uint32_t n = 0; n < entries; ++n) b.u32(random()).u16(random());
            return b;
        };
        const auto subtable_apple = [&](std::uint32_t length, std::uint32_t coverage, std::uint32_t pairs, std::uint32_t entries) {
            Bytes b;
            b.u32(length).u16(coverage).u16(0).u16(pairs).u16(8).u16(1).u16(2);
            for (std::uint32_t n = 0; n < entries; ++n) b.u32(random()).u16(random());
            return b;
        };
        for (const bool apple : {false, true}) {
            for (const std::uint32_t pairs : {0u, 1u, 3u, 12u})
                for (const std::uint32_t length : {0u, 6u, 0x20u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
                    if (!apple && length > 0xffff) continue;
                    const auto bytes = apple ? subtable_apple(length, 0x00, pairs, pairs).data : subtable_ms(0, length, pairs, pairs).data;
                    add(new_kern_subtable, {{3, font.mem}, {4, apple}, {5, stream_at}}, stream_over(bytes), std::string("kern subtable ") + (apple ? "apple" : "microsoft") + " pairs " + std::to_string(pairs) + " length " + hex(length));
                }
            for (const std::uint32_t format : {1u, 2u, 0x100u, 0x7fffu}) {
                const auto bytes = apple ? subtable_apple(0x20, format, 2, 2).data : subtable_ms(format, 0x20, 2, 2).data;
                add(new_kern_subtable, {{3, font.mem}, {4, apple}, {5, stream_at}}, stream_over(bytes), std::string("kern subtable ") + (apple ? "apple" : "microsoft") + " format " + hex(format));
            }
        }
        add(new_kern_subtable, {{3, font.mem}, {4, 1}, {5, stream_at}}, stream_over(subtable_apple(0x20, 0, 3, 3).data, 7), "kern subtable stream ends");
        // Whole tables: Microsoft version 0 with n subtables, Apple version 1 with a zero count and a word count.
        for (const std::uint32_t count : {0u, 1u, 2u, 5u, 0xffffu}) {
            Bytes ms;
            ms.u16(0).u16(count);
            for (std::uint32_t n = 0; n < (count > 5 ? 0 : count); ++n) ms.data.insert(ms.data.end(), subtable_ms(0, 0x20, 2 + n, 2 + n).data.begin(), subtable_ms(0, 0x20, 2 + n, 2 + n).data.end());
            add(new_kern, {{3, font.mem}, {4, stream_at}}, stream_over(ms.data), "kern table microsoft, " + hex(count) + " subtables");
            Bytes apple;
            apple.u16(1).u16(0).u32(count > 5 ? 0 : count);
            for (std::uint32_t n = 0; n < (count > 5 ? 0 : count); ++n) apple.data.insert(apple.data.end(), subtable_apple(0x20, 0, 2, 2).data.begin(), subtable_apple(0x20, 0, 2, 2).data.end());
            add(new_kern, {{3, font.mem}, {4, stream_at}}, stream_over(apple.data), "kern table apple, " + hex(count) + " subtables");
        }
        for (const std::uint32_t version : {1u, 2u, 0x8000u, 0xffffu}) {
            Bytes odd;
            odd.u16(version).u16(version == 1 ? 1 : 0xfffe);
            odd.data.insert(odd.data.end(), subtable_ms(0, 0x20, 2, 2).data.begin(), subtable_ms(0, 0x20, 2, 2).data.end());
            add(new_kern, {{3, font.mem}, {4, stream_at}}, stream_over(odd.data), "kern table version " + hex(version) + " negative or one count");
        }
    }

    // ---- cmap ----
    {
        // A subtable of a format: format 0 a 256-byte array, format 4 two segments, format 6 five entries, anything else a few bytes.
        const auto subtable = [&](std::uint32_t format) {
            Bytes b;
            if (format == 0) { b.u16(0).u16(262).u16(0); for (unsigned n = 0; n < 256; ++n) b.u8(n ^ 5); }
            else if (format == 4) {
                b.u16(4).u16(0x28).u16(0).u16(6).u16(4).u16(1).u16(2);
                b.u16(0x7e).u16(0xffff).u16(0).u16(0x20).u16(0xffff).u16(0xffe1).u16(1).u16(0).u16(0);
                b.u16(0);
            } else if (format == 6) b.u16(6).u16(20).u16(0).u16(0x30).u16(5).u16(11).u16(12).u16(13).u16(14).u16(15);
            else b.u16(format).u16(8).u16(0).u16(0).u16(0);
            return b;
        };
        // A table of records {platform, encoding, format}: each subtable follows the header, in order.
        const auto table = [&](const std::vector<std::array<std::uint32_t, 3>>& records) {
            Bytes b;
            b.u16(0).u16(std::uint32_t(records.size()));
            std::uint32_t offset = 4 + 8 * std::uint32_t(records.size());
            std::vector<Bytes> subtables;
            for (const auto& record : records) {
                b.u16(record[0]).u16(record[1]).u32(offset);
                subtables.push_back(subtable(record[2]));
                offset += std::uint32_t(subtables.back().data.size());
            }
            for (const auto& s : subtables) b.data.insert(b.data.end(), s.data.begin(), s.data.end());
            return b;
        };
        const std::vector<std::vector<std::array<std::uint32_t, 3>>> layouts = {
            {{3, 1, 4}}, {{3, 0, 4}}, {{1, 0, 0}}, {{0, 3, 4}}, {{3, 1, 4}, {1, 0, 0}}, {{1, 0, 0}, {3, 1, 4}}, {{3, 1, 6}}, {{7, 7, 4}}, {{7, 7, 0}}, {{7, 7, 6}},
            {{3, 1, 2}}, {{3, 1, 2}, {3, 0, 4}}, {{1, 1, 2}, {1, 0, 6}, {3, 3, 0}}, {{3, 3, 4}, {3, 1, 0}, {3, 0, 6}}, {{0xffff, 5, 4}}, {{5, 0xffff, 6}}, {{3, 0xffff, 4}},
            {{2, 1, 4}, {3, 5, 6}, {3, 1, 0}}, {{9, 9, 2}, {8, 8, 2}}, {{3, 1, 4}, {3, 1, 0}, {3, 1, 6}}, {{4, 4, 4}, {4, 4, 6}, {4, 4, 0}}, {{4, 4, 4}, {4, 4, 6}}, {{4, 4, 6}, {4, 4, 4}}, {{8, 8, 6}, {8, 8, 6}, {8, 8, 4}}, {}};
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> wanted = {{3, 1}, {3, 0}, {1, 0}, {0, 3}, {7, 7}, {0xffff, 5}, {5, 0xffff}, {0xffff, 0xffff}, {3, 0xffff}, {0xffff, 1}, {0x10003, 0x10001}, {2, 2}};
        for (const auto& records : layouts) {
            std::string shape;
            for (const auto& r : records) shape += "(" + std::to_string(r[0]) + "," + std::to_string(r[1]) + ",f" + std::to_string(r[2]) + ")";
            for (const auto& [platform, encoding] : wanted)
                add(new_cmap, {{3, font.mem}, {4, platform}, {5, encoding}, {6, stream_at}}, stream_over(table(records).data), "cmap " + shape + " wants " + hex(platform) + "," + hex(encoding));
        }
        add(new_cmap, {{3, font.mem}, {4, 3}, {5, 1}, {6, stream_at}}, stream_over(table({{3, 1, 4}, {1, 0, 0}}).data, 9), "cmap, the stream ends in the records");
        // Random bytes after a header that names a few records: formats whatever the bytes say.
        for (unsigned n = 0; n < 80; ++n) {
            Bytes b;
            b.u16(0).u16(1 + random() % 3);
            b.many(300, random);
            for (auto& byte : b.data) if (random() % 4 == 0) byte = 0;
            const auto& [platform, encoding] = wanted[random() % wanted.size()];
            add(new_cmap, {{3, font.mem}, {4, platform}, {5, encoding}, {6, stream_at}}, stream_over(b.data), "cmap random " + std::to_string(n));
        }
    }

    // ---- glyphs ----
    {
        // A simple outline of `contours` contours with `points` points per contour; flags drawn from a set, repeats where asked.
        const auto simple = [&](std::uint32_t contours, std::uint32_t points, std::uint32_t instructions, bool repeats) {
            Bytes b;
            b.u16(contours).u16(0xfff0).u16(0xff00).u16(0x0300).u16(0x0400);
            for (std::uint32_t n = 1; n <= contours; ++n) b.u16(n * points - 1);
            b.u16(instructions);
            for (std::uint32_t n = 0; n < instructions; ++n) b.u8(0x40 + n);
            const auto total = contours * points;
            std::uint32_t written = 0;
            std::vector<std::uint8_t> flag_list;
            while (written < total) {
                const std::uint8_t flag = std::uint8_t(random() & 0x37) | (random() % 3 ? 1 : 0);
                if (repeats && random() % 2 && total - written > 1) {
                    const auto count = 1 + random() % (total - written - 1);
                    b.u8(flag | 8).u8(count);
                    for (std::uint32_t k = 0; k <= count; ++k) flag_list.push_back(flag);
                    written += count + 1;
                } else {
                    b.u8(flag & ~8u);
                    flag_list.push_back(flag & ~8u);
                    ++written;
                }
            }
            for (const auto flag : flag_list) {   // x then y
                if (flag & 2) b.u8(random()); else if (!(flag & 0x10)) b.u16(random());
            }
            for (const auto flag : flag_list) {
                if (flag & 4) b.u8(random()); else if (!(flag & 0x20)) b.u16(random());
            }
            return b;
        };
        const auto composite = [&](const std::vector<std::uint32_t>& flag_words, std::uint32_t instructions) {
            Bytes b;
            b.u16(0xffff).u16(1).u16(2).u16(3).u16(4);
            for (const auto flags : flag_words) {
                b.u16(flags).u16(random() & 0xff).u16(1 + random() % 50);
                if (flags & 1) b.u16(7); // second word of the arguments
                if (flags & 8) b.u16(0x4000); else if (flags & 0x40) b.u16(0x4000).u16(0x2000); else if (flags & 0x80) b.u16(0x4000).u16(1).u16(2).u16(0x3000);
            }
            if (!flag_words.empty() && (flag_words.back() & 0x100)) {
                b.u16(instructions);
                for (std::uint32_t n = 0; n < instructions; ++n) b.u8(0x20 + n);
            }
            return b;
        };
        const std::vector<std::pair<unsigned, std::vector<std::uint32_t>>> metrics = {{0, {0, 0, 0, 0}}, {1, {5, 100, 3, 9}}, {2, {0xfffffff0u, 0x7fff, 0x8000, 0xffffffffu}}};
        const auto base_registers = [&](std::uint32_t keep, const std::vector<std::uint32_t>& m) {
            return std::vector<std::pair<unsigned, std::uint32_t>>{{3, font.mem}, {4, stream_at}, {5, keep}, {6, m[0]}, {7, m[1]}, {8, m[2]}, {9, m[3]}};
        };
        for (const std::uint32_t contours : {1u, 2u, 7u, 8u, 9u, 12u})
            for (const std::uint32_t points : {1u, 3u, 11u})
                for (const std::uint32_t instructions : {0u, 5u}) {
                    for (const bool repeats : {false, true}) {
                        const auto bytes = simple(contours, points, instructions, repeats);
                        add(new_glyph, base_registers(1, metrics[1].second), stream_over(bytes.data), "glyph " + std::to_string(contours) + " contours of " + std::to_string(points) + " points, " + std::to_string(instructions) + " instructions" + (repeats ? " with repeats" : ""));
                        if (instructions) add(new_glyph, base_registers(0, metrics[2].second), stream_over(bytes.data), "glyph " + std::to_string(contours) + "x" + std::to_string(points) + " instructions skipped");
                    }
                }
        for (unsigned n = 0; n < 60; ++n) {
            const auto bytes = simple(1 + random() % 6, 1 + random() % 9, random() % 4 ? 0 : random() % 12, random() % 2);
            const auto& m = metrics[random() % metrics.size()].second;
            add(new_glyph, base_registers(random() % 2, m), stream_over(bytes.data), "glyph random " + std::to_string(n));
        }
        for (const std::uint32_t keep : {0u, 1u, 0x100u, 0x80u, 0xffu}) {
            add(new_glyph, base_registers(keep, metrics[1].second), stream_over(simple(2, 4, 6, true).data), "glyph, keep flag " + hex(keep));
        }
        // No contours (the assertion, answered and real), the stream ends early, the point count wraps.
        {
            Bytes b;
            b.u16(0).u16(1).u16(2).u16(3).u16(4);
            add(new_glyph, base_registers(1, metrics[1].second), stream_over(b.data), "glyph of no contours, answered");
            auto& item = add(new_glyph, base_registers(1, metrics[1].second), stream_over(b.data), "glyph of no contours, real reporter");
            item.answers.clear();
        }
        for (const std::uint32_t length : {0u, 5u, 11u, 13u, 20u, 27u})
            add(new_glyph, base_registers(1, metrics[1].second), stream_over(simple(2, 3, 4, true).data, length), "glyph, stream ends at " + std::to_string(length));
        {
            Bytes b;
            b.u16(1).u16(1).u16(2).u16(3).u16(4).u16(0xffff);  // end point -1: zero points, the assertion
            b.u16(0);
            add(new_glyph, base_registers(1, metrics[1].second), stream_over(b.data), "glyph of zero points, answered");
            Bytes wide;
            wide.u16(1).u16(1).u16(2).u16(3).u16(4).u16(0x7fff).u16(0);
            wide.many(200, random);
            add(new_glyph, base_registers(1, metrics[1].second), stream_over(wide.data), "glyph of a huge point count");
        }
        // Composites: every transform kind, several components, instructions, the table growing past its capacity.
        for (const std::vector<std::uint32_t>& flags : std::vector<std::vector<std::uint32_t>>{
                 {0}, {1}, {8}, {0x40}, {0x80}, {1 | 8}, {1 | 0x40}, {1 | 0x80}, {0x20, 0}, {0x20 | 1, 0x20 | 8, 0x40}, {0x20, 0x20, 0x20, 0x80},
                 {0x100}, {0x100 | 1}, {0x20, 0x100 | 0x40}, {0x20, 0x20, 0x20, 0x20, 0x100}, {0x2a, 0x8}})
            for (const std::uint32_t keep : {0u, 1u}) {
                std::string label;
                for (const auto f : flags) label += hex(f) + " ";
                add(new_glyph, base_registers(keep, metrics[1].second), stream_over(composite(flags, 6).data), "composite " + label + (keep ? "keeping instructions" : "skipping them"));
            }
        {
            // 0x400 words of capacity, 10 spare: a long chain of components outgrows it (the reallocation, answered and failing).
            std::vector<std::uint32_t> chain(400, 0x20 | 1 | 0x80);
            chain.push_back(1);
            auto bytes = composite(chain, 0).data;
            add(new_glyph, base_registers(1, metrics[1].second), stream_over(bytes), "composite of 401 components (the buffer grows)");
            auto& item = add(new_glyph, base_registers(1, metrics[1].second), stream_over(bytes), "composite of 401 components, the reallocation fails");
            item.answers = {{reporter, 0}, {realloc_entry, 0}};
        }
    }

    // ---- the directory lookups over each font's own sfnt ----
    for (const auto& f : fonts) {
        const auto entries = word(scenario, f.directory + 0x10);
        const auto count = scenario.u16(f.directory + 8);
        std::vector<std::uint32_t> tags = {0, 0x12345678, 0xffffffff, 0x63767420, 0x70726570, 0x6670676d, 0x636d6170, 0x68656164, 0x68686561, 0x68746d78, 0x6c6f6361, 0x676c7966, 0x6d617870, 0x6b65726e, 0x54324b47};
        for (std::uint32_t n = 0; n < count && n < 40; ++n) tags.push_back(word(scenario, word(scenario, entries + 4 * n) + 4));
        for (const auto tag : tags) {
            add(find_table, {{3, f.sfnt}, {4, tag}, {9, 0x99}, {10, 0xaa}}, {}, "find table " + hex(tag) + " of the font " + hex(f.size));
            add(new_table_stream, {{3, f.sfnt}, {4, tag}, {9, 0x99}, {10, 0xaa}}, {}, "stream over table " + hex(tag) + " of the font " + hex(f.size));
        }
        // A directory of our own: zero tables, a negative count.
        add(find_table, {{3, f.sfnt}, {4, 0x636d6170}}, {w16(f.directory + 8, 0)}, "find table in an empty directory");
        add(find_table, {{3, f.sfnt}, {4, 0x636d6170}}, {w16(f.directory + 8, 0xffff)}, "find table with a negative count");
    }
}
}
