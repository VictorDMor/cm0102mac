// The font engine, block `truetype_glyph` (TRUETYPE.C, 0x10765810..0x10767460): cases for the sfnt's lookups (glyph limit, units per em,
// the largest outline, the character's glyph and the digits through the cmap), the glyph by index, the caret slopes, the cmap's and the
// sfnt's deletion, the reading of the tables and the API entries that create and delete an sfnt. Everything runs on the snapshot's own
// fonts (found by the scaler's stamps): the three sfnts, their streams (reset to the start of the file), their memory objects and
// glyph data, so the allocator, the table readers and the glyph readers are the real ones on both sides; a case that needs a table
// missing, a value at an edge or another format copies the sfnt into the scratch area (or pokes the font's own bytes, undone after the
// case) with that one change. A TTC and the tables no shipped font has (vhea, vmtx, fvar) are made by renaming directory entries of a
// font in place. Labels start with "truetype_glyph:" (CM_FONT_ENGINE_BLOCK=truetype_glyph).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <cstdlib>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t read_sfnt = 0x10765810, glyph_limit = 0x10766430, load_glyph = 0x107664a0, character_glyph = 0x10766770, is_digit = 0x107668d0,
                        new_sfnt = 0x10766ab0, units_per_em = 0x10766cc0, max_points = 0x10766d30, caret_slopes = 0x10766dc0, delete_cmap = 0x107670e0,
                        delete_sfnt = 0x10767190, raise_error = 0x10767430;
constexpr std::uint32_t reporter = 0x107755c0, setjmp_entry = 0x10773eb0, longjmp_entry = 0x10773f30, make_cmap = 0x10764610, game_toc = 0x108bba90;
constexpr std::uint32_t area = scratch_base + 0x50000, sfnt_at = area, out_at = area + 0x1000, fake_head = area + 0x2000, fake_maxp = area + 0x2100,
                        fake_loca = area + 0x2200, fake_hhea = area + 0x2300, fake_vhea = area + 0x2400, fake_cmap = area + 0x2500, fake_array = area + 0x2600,
                        fake_record = area + 0x2700, fake_data = area + 0x2800, callbacks_at = area + 0x3000, error_at = area + 0x3100,
                        descriptor_at = area + 0x3200, caret_h = area + 0x3300, caret_v = area + 0x3400, stream_at = area + 0x4000, data_at = area + 0x4100;

struct Font { std::uint32_t sfnt, mem, stream, data, size; };
std::uint32_t word(const Scenario& scenario, std::uint32_t at) { return scenario.r32(at); }
std::uint32_t half(const Scenario& scenario, std::uint32_t at) { return scenario.u16(at); }
std::vector<Font> fonts_of(const Scenario& scenario) {
    std::vector<Font> fonts;
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4)
            if (word(scenario, std::uint32_t(at)) == 0x5a1234a5 && word(scenario, std::uint32_t(at) + 0x5f4) == 0xa5fedc5a) {
                const auto sfnt = word(scenario, std::uint32_t(at) + 0x144), stream = word(scenario, sfnt + 0x54);
                const Font font{sfnt, word(scenario, sfnt + 0x5c), stream, word(scenario, stream), word(scenario, stream + 8)};
                if (std::ranges::none_of(fonts, [&](const Font& other) { return other.sfnt == font.sfnt; })) fonts.push_back(font);
            }
    }
    return fonts;
}
// The sfnt's 0x68 bytes copied into the scratch area.
void copy_sfnt(std::vector<Write>& memory, const Scenario& scenario, const Font& font, std::uint32_t to = sfnt_at) {
    for (std::uint32_t at = 0; at < 0x68; at += 4) memory.push_back(w32(to + at, word(scenario, font.sfnt + at)));
}
void rewind(std::vector<Write>& memory, const Font& font) { memory.push_back(w32(font.stream + 4, 0)); }
std::vector<std::uint8_t> random_bytes(std::mt19937& random, std::size_t count) {
    std::vector<std::uint8_t> bytes(count);
    for (auto& byte : bytes) byte = std::uint8_t(random());
    return bytes;
}
void put_bytes(std::vector<Write>& memory, std::uint32_t at, const std::vector<std::uint8_t>& bytes) {
    for (std::size_t n = 0; n < bytes.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), bytes[n]));
}
// The directory entries of a font: their tags and addresses.
struct Entry { std::uint32_t at, tag, index; };
std::vector<Entry> entries_of(const Scenario& scenario, const Font& font) {
    const auto directory = word(scenario, font.sfnt), count = half(scenario, directory + 8), array = word(scenario, directory + 0x10);
    std::vector<Entry> entries;
    for (std::uint32_t n = 0; n < count; ++n) {
        const auto entry = word(scenario, array + 4 * n);
        entries.push_back({entry, word(scenario, entry + 4), n});
    }
    return entries;
}
constexpr std::uint32_t tag_of(const char (&text)[5]) { return std::uint32_t(std::uint8_t(text[0])) << 24 | std::uint32_t(std::uint8_t(text[1])) << 16 | std::uint32_t(std::uint8_t(text[2])) << 8 | std::uint8_t(text[3]); }
}

FONT_ENGINE_CASES("truetype_glyph") {
    std::mt19937 random(0x7747);
    std::vector<Write> vertical_pokes;  // the first font's vhea and vmtx (the entries renamed), for the allocation sweep
    const auto first_case = cases.size();
    const auto fonts = fonts_of(scenario);
    const auto toc = std::pair<unsigned, std::uint32_t>{2, game_toc};
    // ---- the limits and the sizes ------------------------------------------------------------------------------------------------
    for (const auto& font : fonts) {
        cases.push_back({.entry = glyph_limit, .registers = {{3, font.sfnt}, toc}, .label = "glyph limit real font " + hex(font.sfnt)});
        cases.push_back({.entry = units_per_em, .registers = {{3, font.sfnt}, toc}, .label = "units real font " + hex(font.sfnt)});
        cases.push_back({.entry = max_points, .registers = {{3, font.sfnt}, toc}, .label = "max points real font " + hex(font.sfnt)});
    }
    if (!fonts.empty()) {
        const auto& font = fonts.front();
        const auto numbers = half(scenario, word(scenario, font.sfnt + 0x10) + 8);
        for (const std::uint32_t count : {0u, 1u, numbers - 1, numbers, numbers + 1, numbers + 7, 0x7fffffffu, 0x80000000u, 0xffffffffu})
            for (const bool has_loca : {true, false}) {
                std::vector<Write> memory;
                copy_sfnt(memory, scenario, font);
                memory.push_back(w32(sfnt_at + 0x10, fake_maxp));
                memory.push_back(w16(fake_maxp + 8, numbers));
                memory.push_back(w32(sfnt_at + 0x14, has_loca ? fake_loca : 0));
                memory.push_back(w32(fake_loca + 8, count));
                cases.push_back({.entry = glyph_limit, .registers = {{3, sfnt_at}, toc}, .memory = memory,
                                 .label = "glyph limit loca count " + hex(count) + (has_loca ? "" : " no loca")});
            }
        {   // No maxp: the assertion (real reporter, then answered), then the read through null faults alike.
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x10, 0));
            for (const bool answered : {true, false}) {
                Case item{.entry = glyph_limit, .registers = {{3, sfnt_at}, toc}, .memory = memory, .label = std::string("glyph limit no maxp (assertion) ") + (answered ? "answered" : "real reporter")};
                if (answered) item.answers = {{reporter, 0}};
                cases.push_back(item);
            }
        }
        for (const std::uint32_t upem : {0u, 1u, 0x800u, 0x7fffu, 0x8000u, 0xffffu, 0x1000u})
            for (const bool has_head : {true, false}) {
                std::vector<Write> memory;
                copy_sfnt(memory, scenario, font);
                memory.push_back(w32(sfnt_at + 0xc, has_head ? fake_head : 0));
                memory.push_back(w16(fake_head + 0x16, upem));
                cases.push_back({.entry = units_per_em, .registers = {{3, sfnt_at}, toc}, .memory = memory, .label = "units " + hex(upem) + (has_head ? "" : " no head")});
            }
        cases.push_back({.entry = units_per_em, .registers = {{3, 0}, toc}, .answers = {{reporter, 0}}, .label = "units null font (assertion answered)"});
        for (const auto& [first, second] : {std::pair{1u, 2u}, {2u, 1u}, {5u, 5u}, {0xffffu, 0u}, {0x7fffu, 0x8000u}, {0x8000u, 0x7fffu}, {0u, 0u}, {0x100u, 0xffu}}) {
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x10, fake_maxp));
            memory.push_back(w16(fake_maxp + 0xa, first));
            memory.push_back(w16(fake_maxp + 0xe, second));
            cases.push_back({.entry = max_points, .registers = {{3, sfnt_at}, toc}, .memory = memory, .label = "max points " + hex(first) + " " + hex(second)});
        }
        {
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x10, 0));
            cases.push_back({.entry = max_points, .registers = {{3, sfnt_at}, toc}, .memory = memory, .answers = {{reporter, 0}},
                             .label = "max points no maxp (assertion answered)"});
            cases.push_back({.entry = max_points, .registers = {{3, 0}, toc}, .answers = {{reporter, 0}}, .label = "max points null font (assertion answered)"});
        }
    }
    // ---- the allocator's error site ----------------------------------------------------------------------------------------------
    for (const auto& font : fonts)
        for (const std::uint32_t code : {0x2718u, 0x2719u, 0x271bu, 0u, 1u, 0xffffffffu})
            cases.push_back({.entry = raise_error, .registers = {{3, font.mem}, {4, code}, toc}, .answers = {{longjmp_entry, 0}},
                             .label = "raise " + hex(code) + " (longjmp answered)"});
    // ---- the character's glyph and the digits -----------------------------------------------------------------------------------
    for (const auto& font : fonts) {
        std::vector<std::uint32_t> codes;
        for (std::uint32_t code = 0; code < 0x100; ++code) codes.push_back(code);
        for (const std::uint32_t code : {0x100u, 0x1ffu, 0x2022u, 0x20acu, 0xf000u, 0xf0ffu, 0xfffeu, 0xffffu, 0x10041u, 0xffff0041u}) codes.push_back(code);
        for (const auto code : codes)
            cases.push_back({.entry = character_glyph, .registers = {{3, font.sfnt}, {4, code}, toc}, .label = "character " + hex(code) + " font " + hex(font.sfnt)});
        const auto cmap = word(scenario, font.sfnt + 0x28);
        for (unsigned n = 0; n < 10 && cmap != 0; ++n) {
            const auto glyph = half(scenario, cmap + 0x18 + 2 * n);
            for (const std::uint32_t code : {glyph, glyph + 1, glyph ^ 0x10000u})
                cases.push_back({.entry = is_digit, .registers = {{3, font.sfnt}, {4, code}, toc}, .label = "digit " + hex(code) + " font " + hex(font.sfnt)});
        }
        for (const std::uint32_t code : {0u, 0x20u, 0x41u, 0xffffu})
            cases.push_back({.entry = is_digit, .registers = {{3, font.sfnt}, {4, code}, toc}, .label = "digit other " + hex(code) + " font " + hex(font.sfnt)});
        // The cmap read on first use (the sfnt copy has none), with the platform and encoding the font has and others.
        for (const auto& [platform, encoding] : {std::pair{word(scenario, font.sfnt + 0x30) >> 16, word(scenario, font.sfnt + 0x30) & 0xffff}, {3u, 1u}, {1u, 0u}, {0u, 3u}, {0xffffu, 0xffffu}, {9u, 9u}}) {
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x28, 0));
            memory.push_back(w16(sfnt_at + 0x30, platform));
            memory.push_back(w16(sfnt_at + 0x32, encoding));
            for (const std::uint32_t code : {0x41u, 0x20u, 0xffu, 0x2022u}) {
                cases.push_back({.entry = character_glyph, .registers = {{3, sfnt_at}, {4, code}, toc}, .memory = memory,
                                 .label = "character " + hex(code) + " cmap read on first use, platform " + hex(platform) + " encoding " + hex(encoding)});
            }
            cases.push_back({.entry = is_digit, .registers = {{3, sfnt_at}, {4, 0x30}, toc}, .memory = memory,
                             .label = "digit cmap read on first use, platform " + hex(platform) + " encoding " + hex(encoding)});
        }
        {   // The cmap reader answered with nothing: the digit lookup reports the assertion.
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x28, 0));
            for (const bool answered : {true, false}) {
                Case item{.entry = is_digit, .registers = {{3, sfnt_at}, {4, 0x30}, toc}, .memory = memory, .answers = {{make_cmap, 0}},
                          .label = std::string("digit no cmap (assertion) ") + (answered ? "answered" : "real reporter")};
                if (answered) item.answers.push_back({reporter, 0});
                cases.push_back(item);
            }
        }
        // The format of the chosen subtable: the real cmap with its format changed (0, 6, 4, and one nobody reads: 5).
        if (cmap != 0)
            for (const std::uint32_t format : {0u, 4u, 5u, 6u, 0xffffu})
                for (const std::uint32_t code : {0x20u, 0x41u, 0xffu, 0x100u}) {
                    std::vector<Write> memory{w16(cmap + 0x16, format)};
                    cases.push_back({.entry = character_glyph, .registers = {{3, font.sfnt}, {4, code}, toc}, .memory = memory,
                                     .label = "character " + hex(code) + " cmap format " + hex(format) + " font " + hex(font.sfnt)});
                }
    }
    if (!fonts.empty()) {
        // A format 0 cmap of the case's own: records, the table behind them, and the code's edges.
        const auto& font = fonts.front();
        for (const std::uint32_t code : {0u, 1u, 0x41u, 0xffu, 0x100u, 0x1ffu, 0xffffu, 0x10041u})
            for (const std::uint32_t chosen : {0u, 1u}) {
                std::vector<Write> memory;
                copy_sfnt(memory, scenario, font);
                memory.push_back(w32(sfnt_at + 0x28, fake_cmap));
                memory.push_back(w32(fake_cmap + 8, fake_array));
                memory.push_back(w32(fake_cmap + 0xc, fake_data));
                memory.push_back(w16(fake_cmap + 0x14, chosen));
                memory.push_back(w16(fake_cmap + 0x16, 0));
                memory.push_back(w32(fake_array, fake_record));
                memory.push_back(w32(fake_array + 4, fake_record + 0x20));
                memory.push_back(w32(fake_record + 4, 4));
                memory.push_back(w32(fake_record + 0x24, 8));
                put_bytes(memory, fake_data, random_bytes(random, 0x240));
                cases.push_back({.entry = character_glyph, .registers = {{3, sfnt_at}, {4, code}, toc}, .memory = memory,
                                 .label = "character " + hex(code) + " format 0 chosen " + hex(chosen)});
            }
    }
    // ---- the caret slopes (a leaf over two records) --------------------------------------------------------------------------------
    for (const auto& font : fonts) cases.push_back({.entry = caret_slopes, .registers = {{3, font.sfnt}, {4, caret_h}, {5, caret_v}, {9, 0x5555}, {10, 0x6666}, toc}, .label = "caret real font " + hex(font.sfnt)});
    if (!fonts.empty()) {
        const auto& font = fonts.front();
        const std::int32_t slopes[] = {0, 1, -1, 3, 0x7fff, -0x8000, 0x100, 0x4000, -0x4000, 0x8000 - 1, 0x2000, 7, -9};
        for (const auto run : slopes)
            for (const auto rise : slopes) {
                if ((run + 3 * rise) % 3 != 0 && run % 2) continue;
                std::vector<Write> memory;
                copy_sfnt(memory, scenario, font);
                memory.push_back(w32(sfnt_at + 0x18, fake_hhea));
                memory.push_back(w32(sfnt_at + 0x1c, fake_vhea));
                for (const auto record : {fake_hhea, fake_vhea}) {
                    put_bytes(memory, record, random_bytes(random, 0x28));
                    memory.push_back(w16(record + 0x18, std::uint32_t(run)));
                    memory.push_back(w16(record + 0x16, std::uint32_t(rise)));
                }
                cases.push_back({.entry = caret_slopes, .registers = {{3, sfnt_at}, {4, caret_h}, {5, caret_v}, {9, 0x5555}, {10, 0x6666}, toc}, .memory = memory,
                                 .label = "caret run " + hex(std::uint32_t(run)) + " rise " + hex(std::uint32_t(rise))});
            }
        for (const auto [horizontal, vertical] : {std::pair{false, false}, {true, false}, {false, true}}) {
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x18, horizontal ? fake_hhea : 0));
            memory.push_back(w32(sfnt_at + 0x1c, vertical ? fake_vhea : 0));
            put_bytes(memory, fake_hhea, random_bytes(random, 0x28));
            put_bytes(memory, fake_vhea, random_bytes(random, 0x28));
            cases.push_back({.entry = caret_slopes, .registers = {{3, sfnt_at}, {4, caret_h}, {5, caret_v}, {9, 0x5555}, {10, 0x6666}, toc}, .memory = memory,
                             .label = std::string("caret tables ") + (horizontal ? "hhea" : "-") + (vertical ? " vhea" : " -")});
        }
    }
    // ---- the cmap's deletion --------------------------------------------------------------------------------------------------------
    for (const auto& font : fonts) cases.push_back({.entry = delete_cmap, .registers = {{3, font.sfnt}, toc}, .label = "delete cmap real font " + hex(font.sfnt)});
    if (!fonts.empty()) {
        std::vector<Write> memory;
        copy_sfnt(memory, scenario, fonts.front());
        memory.push_back(w32(sfnt_at + 0x28, 0));
        cases.push_back({.entry = delete_cmap, .registers = {{3, sfnt_at}, toc}, .memory = memory, .label = "delete cmap none"});
    }
    // ---- the glyph by index ------------------------------------------------------------------------------------------------------------
    for (const auto& font : fonts) {
        const auto limit = std::int32_t(word(scenario, font.sfnt + 0x64));
        std::vector<std::int32_t> indexes;
        for (std::int32_t index = -2; index <= limit + 2; ++index) indexes.push_back(index);
        for (const std::int32_t index : {0x7fffffff, std::int32_t(0x80000000), 0x10000, -0x10000}) indexes.push_back(index);
        for (const auto index : indexes)
            for (const std::uint32_t keep : {0u, 1u}) {
                if (keep && index > 4 && index % 3) continue;
                cases.push_back({.entry = load_glyph, .registers = {{3, font.sfnt}, {4, std::uint32_t(index)}, {5, keep}, {6, out_at}, {7, out_at + 4}, toc},
                                 .label = "load glyph " + std::to_string(index) + " keep " + std::to_string(keep) + " font " + hex(font.sfnt)});
            }
    }
    if (!fonts.empty()) {
        const auto& font = fonts.front();
        const auto limit = word(scenario, font.sfnt + 0x64);
        const auto add = [&](const std::string& label, const std::vector<Write>& pokes, std::uint32_t index, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            append(memory, pokes);
            Case item{.entry = load_glyph, .registers = {{3, sfnt_at}, {4, index}, {5, 0}, {6, out_at}, {7, out_at + 4}, toc}, .memory = {}, .label = "load glyph " + label};
            item.memory = std::move(memory);
            item.answers = std::move(answers);
            cases.push_back(item);
        };
        const auto index = std::min<std::uint32_t>(limit > 4 ? limit / 2 : 1, limit);
        // The metrics tables, the data format and the glyph callback of the sfnt, one at a time.
        add("no hmtx", {w32(sfnt_at + 0x20, 0)}, index);
        add("no hmtx no vmtx (empty glyph)", {w32(sfnt_at + 0x20, 0), w32(sfnt_at + 0x24, 0)}, index);
        add("no hhea (no vertical metrics)", {w32(sfnt_at + 0x18, 0)}, index);
        add("no loca (empty glyph)", {w32(sfnt_at + 0x14, 0)}, index);
        add("limit 1", {w32(sfnt_at + 0x64, 1)}, index);
        add("limit 0", {w32(sfnt_at + 0x64, 0)}, 0);
        for (const std::uint32_t format : {0u, 1u, 5u, 0x7d0u, 0x7cfu}) {
            std::vector<Write> memory{w16(word(scenario, font.sfnt + 0xc) + 0x38, format)};
            add("data format " + hex(format) + " (assertion answered)", memory, index, {{reporter, 0}});
        }
        add("reader answered with nothing (assertion answered)", {w16(word(scenario, font.sfnt + 0xc) + 0x38, 0)}, index, {{0x10765060, 0}, {reporter, 0}});
        add("data format 5 (real reporter)", {w16(word(scenario, font.sfnt + 0xc) + 0x38, 5)}, index);
        // The glyph callback: a descriptor of a function that returns a value from its first two arguments and writes nothing.
        add("callback", {w32(sfnt_at + 0x3c, descriptor_at), w32(descriptor_at, 0x10767d40), w32(descriptor_at + 4, game_toc)}, index);
        add("no glyf table", {w32(sfnt_at + 0x64, limit)}, index);
        {   // A glyph with no data (loca's next offset is the glyph's own) and one with a negative length, in a scratch copy of loca.
            const auto loca = word(scenario, font.sfnt + 0x14), offsets = word(scenario, loca + 4);
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x14, fake_loca));
            memory.push_back(w32(fake_loca + 4, fake_data));
            for (std::uint32_t n = 0; n < 8; ++n) memory.push_back(w32(fake_data + 4 * n, word(scenario, offsets + 4 * n)));
            memory.push_back(w32(fake_data + 4 * index, word(scenario, offsets + 4 * index) + 40));
            memory.push_back(w32(fake_data + 4 * (index + 1), word(scenario, offsets + 4 * index)));
            for (const std::uint32_t at : {index, index + 1}) memory.push_back(w32(fake_data + 4 * (at + 1), word(scenario, offsets + 4 * (at + 1))));
            cases.push_back({.entry = load_glyph, .registers = {{3, sfnt_at}, {4, index}, {5, 0}, {6, out_at}, {7, out_at + 4}, toc}, .memory = memory,
                             .label = "load glyph with a loca entry past the next (an empty glyph with metrics)"});
        }
        // Vertical metrics from vmtx: a scratch record {+0xc bearings, +0x10 advances}.
        {
            std::vector<Write> memory;
            copy_sfnt(memory, scenario, font);
            memory.push_back(w32(sfnt_at + 0x24, fake_vhea));
            memory.push_back(w32(fake_vhea + 0xc, fake_data));
            memory.push_back(w32(fake_vhea + 0x10, fake_data + 0x100));
            put_bytes(memory, fake_data, random_bytes(random, 0x200));
            cases.push_back({.entry = load_glyph, .registers = {{3, sfnt_at}, {4, index}, {5, 0}, {6, out_at}, {7, out_at + 4}, toc}, .memory = memory,
                             .label = "load glyph with vmtx"});
        }
    }
    // ---- the sfnt read from a stream ----------------------------------------------------------------------------------------------
    for (const auto& font : fonts) {
        // A fresh sfnt (nothing to release), and the font's own sfnt (everything to release first); both over the font's stream.
        std::vector<Write> memory;
        rewind(memory, font);
        memory.push_back(w32(sfnt_at + 0x5c, font.mem));
        memory.push_back(w32(sfnt_at + 0x54, font.stream));
        memory.push_back(w16(sfnt_at + 0x30, half(scenario, font.sfnt + 0x30)));
        memory.push_back(w16(sfnt_at + 0x32, half(scenario, font.sfnt + 0x32)));
        cases.push_back({.entry = read_sfnt, .registers = {{3, sfnt_at}, {4, font.stream}, {5, 0}, toc}, .memory = memory, .label = "read sfnt fresh font " + hex(font.sfnt)});
        {   // The metrics callback of the sfnt (a pure function of its first two arguments), set before the tables are read.
            auto with_callback = memory;
            with_callback.push_back(w32(sfnt_at + 0x40, descriptor_at));
            with_callback.push_back(w32(descriptor_at, 0x10767d40));
            with_callback.push_back(w32(descriptor_at + 4, game_toc));
            cases.push_back({.entry = read_sfnt, .registers = {{3, sfnt_at}, {4, font.stream}, {5, 0}, toc}, .memory = with_callback,
                             .label = "read sfnt fresh with the metrics callback font " + hex(font.sfnt)});
        }
        std::vector<Write> again;
        rewind(again, font);
        cases.push_back({.entry = read_sfnt, .registers = {{3, font.sfnt}, {4, font.stream}, {5, 0}, toc}, .memory = again, .label = "read sfnt again font " + hex(font.sfnt)});
        // The tables the fonts lack, made by renaming entries of the directory of the font in place: an unused entry becomes vhea (its bytes are read as one),
        // the next vmtx, a third fvar (with kern present kern is skipped) and a table that is neither.
        const auto entries = entries_of(scenario, font);
        std::vector<Entry> spare;
        for (const auto& entry : entries)
            if (entry.tag != tag_of("head") && entry.tag != tag_of("hhea") && entry.tag != tag_of("maxp") && entry.tag != tag_of("loca") && entry.tag != tag_of("hmtx") &&
                entry.tag != tag_of("glyf") && entry.tag != tag_of("cmap") && entry.tag != tag_of("kern") && entry.tag != tag_of("post") && entry.tag != tag_of("fpgm") &&
                entry.tag != tag_of("prep") && entry.tag != tag_of("cvt "))
                spare.push_back(entry);
        // The tag of the i-th entry is renamed in the file's own directory (the table directory starts at byte 12 of the font, 16 bytes an
        // entry), and raw bytes of the file may be poked too.
        const auto variant = [&](const std::string& label, const std::vector<std::pair<std::uint32_t, std::uint32_t>>& renames, std::vector<Write> pokes = {},
                                 std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            for (const auto& [index, tag] : renames) pokes.push_back(w32(font.data + 12 + 16 * index, tag));
            rewind(pokes, font);
            pokes.push_back(w32(sfnt_at + 0x5c, font.mem));
            pokes.push_back(w32(sfnt_at + 0x54, font.stream));
            pokes.push_back(w16(sfnt_at + 0x30, half(scenario, font.sfnt + 0x30)));
            pokes.push_back(w16(sfnt_at + 0x32, half(scenario, font.sfnt + 0x32)));
            Case item{.entry = read_sfnt, .registers = {{3, sfnt_at}, {4, font.stream}, {5, 0}, toc}, .memory = pokes, .label = "read sfnt " + label + " font " + hex(font.sfnt)};
            item.answers = std::move(answers);
            cases.push_back(item);
        };
        for (const auto& entry : entries)
            if (spare.size() < 2 && (entry.tag == tag_of("post") || entry.tag == tag_of("kern"))) spare.push_back(entry);
        if (spare.size() >= 2) {
            // Two entries made into a vhea and a vmtx that are the font's own hhea and hmtx (tag, offset and length poked), so the vertical metrics are real.
            std::uint32_t hhea_at = 0, hmtx_at = 0;
            for (const auto& entry : entries) {
                if (entry.tag == tag_of("hhea")) hhea_at = entry.at;
                if (entry.tag == tag_of("hmtx")) hmtx_at = entry.at;
            }
            if (hhea_at != 0 && hmtx_at != 0) {
                const auto directory_entry = [&](const Entry& slot, std::uint32_t tag, std::uint32_t source) {
                    const auto at = font.data + 12 + 16 * slot.index;
                    return std::vector<Write>{w32(at, tag), w32(at + 8, word(scenario, source + 0xc)), w32(at + 12, word(scenario, source + 0x10))};
                };
                auto pokes = directory_entry(spare[0], tag_of("vhea"), hhea_at);
                append(pokes, directory_entry(spare[1], tag_of("vmtx"), hmtx_at));
                variant("with vhea and vmtx", {}, pokes);
                if (vertical_pokes.empty()) vertical_pokes = pokes;
            }
            variant("with vhea only", {{spare[0].index, tag_of("vhea")}});
            variant("with vmtx only", {{spare[0].index, tag_of("vmtx")}});
        }
        if (!spare.empty()) {
            variant("with fvar (kern skipped)", {{spare[0].index, tag_of("fvar")}});
            variant("with another table", {{spare[0].index, tag_of("zzzz")}});
        }
        for (const auto& entry : entries)
            if (entry.tag == tag_of("hhea"))
                for (const std::uint32_t metrics : {3u, 1u, 0u})
                    variant("with " + std::to_string(metrics) + " long metrics (fewer than glyphs)", {}, {w16(font.data + word(scenario, entry.at + 0xc) + 0x22, metrics)});
        for (const auto& [name, tag] : {std::pair{"no kern", tag_of("kern")}, {"no post", tag_of("post")}, {"no hmtx", tag_of("hmtx")}, {"no hhea", tag_of("hhea")}, {"no loca", tag_of("loca")},
                                       {"no head", tag_of("head")}, {"no cmap", tag_of("cmap")}})
            for (const auto& entry : entries)
                if (entry.tag == tag) variant(name, {{entry.index, tag_of("zzzz")}});
    }
    if (!fonts.empty()) {
        // The allocator returns nothing for one allocation of the first font's read (the allocator never does in the game: it longjmps; the
        // numbers were found by answering every allocation in turn): the 25th and 26th are the two arrays of hmtx's record, the 31st and
        // 32nd (with a vhea and vmtx made of the font's own tables) vmtx's. The assertions are answered, the run goes on and faults alike.
        const auto& font = fonts.front();
        for (const bool vertical : {false, true})
            for (const std::uint32_t n : vertical ? std::vector<std::uint32_t>{31, 32} : std::vector<std::uint32_t>{25, 26}) {
                if (vertical && vertical_pokes.empty()) continue;
                std::vector<Write> memory;
                rewind(memory, font);
                memory.push_back(w32(sfnt_at + 0x5c, font.mem));
                memory.push_back(w32(sfnt_at + 0x54, font.stream));
                if (vertical) append(memory, vertical_pokes);
                Case item{.entry = read_sfnt, .registers = {{3, sfnt_at}, {4, font.stream}, {5, 0}, toc}, .memory = memory,
                          .label = std::string("read sfnt ") + (vertical ? "vertical " : "") + "allocation " + std::to_string(n) + " answered 0 (assertion answered)"};
                item.answers = {{reporter, 0}};
                item.nth_answers = {{0x107678a0, n, 0}};
                cases.push_back(item);
            }
    }
    if (!fonts.empty()) {
        // A collection: 'ttcf', version, the number of fonts and their offsets, then a font whose directory lists maxp only (the tables
        // behind are bytes of the case's own). The index in range, past it and negative.
        const auto& font = fonts.front();
        std::vector<std::uint8_t> bytes(0x80, 0);
        const auto put32 = [&](std::size_t at, std::uint32_t value) { for (int k = 0; k < 4; ++k) bytes[at + std::size_t(k)] = std::uint8_t(value >> (24 - 8 * k)); };
        const auto put16 = [&](std::size_t at, std::uint32_t value) { bytes[at] = std::uint8_t(value >> 8); bytes[at + 1] = std::uint8_t(value); };
        put32(0, tag_of("ttcf"));
        put32(4, 0x10000);
        put32(8, 1);
        put32(12, 0x10);
        put32(0x10, 0x10000);
        put16(0x14, 1);
        put32(0x1c, tag_of("maxp"));
        put32(0x28, 0x40);
        put32(0x2c, 0x20);
        for (std::size_t k = 0x40; k < 0x60; ++k) bytes[k] = std::uint8_t(random());
        for (const std::int32_t index : {0, 1, -1}) {
            std::vector<Write> memory;
            memory.push_back(w32(stream_at, data_at));
            memory.push_back(w32(stream_at + 4, 0));
            memory.push_back(w32(stream_at + 8, std::uint32_t(bytes.size())));
            memory.push_back(w32(stream_at + 0x10, 3));
            memory.push_back(w32(stream_at + 0x14, font.mem));
            put_bytes(memory, data_at, bytes);
            memory.push_back(w32(sfnt_at + 0x5c, font.mem));
            memory.push_back(w32(sfnt_at + 0x54, stream_at));
            Case item{.entry = read_sfnt, .registers = {{3, sfnt_at}, {4, stream_at}, {5, std::uint32_t(index)}, toc}, .memory = memory,
                      .label = "read sfnt collection index " + std::to_string(index) + (index == 0 ? "" : " (assertion answered)")};
            if (index != 0) item.answers = {{reporter, 0}};
            cases.push_back(item);
        }
    }
    // ---- the API entries ----------------------------------------------------------------------------------------------------------------
    for (const auto& font : fonts) {
        const auto make = [&](const std::string& label, std::uint32_t mem, std::uint32_t format, std::uint32_t index, std::uint32_t stream, std::uint32_t callbacks,
                              std::uint32_t error, std::vector<Write> memory = {}, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
            rewind(memory, font);
            Case item{.entry = new_sfnt, .registers = {{3, mem}, {4, format}, {5, index}, {6, stream}, {7, callbacks}, {8, error}, toc}, .memory = std::move(memory),
                      .label = "new sfnt " + label + " font " + hex(font.sfnt)};
            item.answers = std::move(answers);
            cases.push_back(item);
        };
        std::vector<Write> callbacks;
        for (std::uint32_t n = 0; n < 6; ++n) callbacks.push_back(w32(callbacks_at + 4 * n, word(scenario, font.sfnt + 0x3c + 4 * n)));
        make("format 2 no callbacks no error pointer", font.mem, 2, 0, font.stream, 0, 0);
        make("format 2 with error pointer", font.mem, 2, 0, font.stream, 0, error_at, {w32(error_at, 0x55555555)});
        make("format 2 with callbacks", font.mem, 2, 0, font.stream, callbacks_at, error_at, callbacks);
        make("callbacks whose first word is zero (assertion answered)", font.mem, 2, 0, font.stream, callbacks_at, error_at, {}, {{reporter, 0}});
        make("format 1 (assertion answered)", font.mem, 1, 0, font.stream, 0, error_at, {}, {{reporter, 0}});
        make("format 1 (real reporter)", font.mem, 1, 0, font.stream, 0, error_at);
        make("setjmp answered with an error code", font.mem, 2, 0, font.stream, 0, error_at, {}, {{setjmp_entry, 0x2718}});
        make("setjmp answered with a second code", font.mem, 2, 0, font.stream, 0, error_at, {}, {{setjmp_entry, 0x2719}});
        make("null memory object (assertion answered)", 0, 2, 0, font.stream, 0, 0, {}, {{reporter, 0}});
        make("null stream (assertion answered)", font.mem, 2, 0, 0, 0, 0, {}, {{reporter, 0}});
        // A real longjmp through the entry's setjmp: the allocator's table made nearly full (new_sfnt: the n-th allocation after raises 0x271c) and a block of
        // the sfnt whose header stamp is spoiled (delete_sfnt: its release raises 0x2719). The entry catches the transfer and frees what the object holds.
        for (const std::uint32_t k : {0u, 1u, 2u, 5u, 12u, 30u}) {
            std::vector<Write> memory{w32(font.mem + 4, word(scenario, font.mem + 8) - k)};
            rewind(memory, font);
            cases.push_back({.entry = new_sfnt, .registers = {{3, font.mem}, {4, 2}, {5, 0}, {6, font.stream}, {7, 0}, {8, error_at}, toc}, .memory = memory,
                             .label = "new sfnt table full after " + std::to_string(k) + " allocations (real longjmp) font " + hex(font.sfnt)});
        }
        {
            const auto directory = word(scenario, font.sfnt);
            const std::uint32_t blocks[] = {word(scenario, word(scenario, directory + 0x10)), word(scenario, font.sfnt + 0xc), word(scenario, font.sfnt + 0x18),
                                            word(scenario, font.sfnt + 0x20), word(scenario, font.sfnt + 0x14), word(scenario, font.sfnt + 0x10), word(scenario, font.sfnt + 0x2c)};
            for (const auto block : blocks)
                if (block != 0)
                    cases.push_back({.entry = delete_sfnt, .registers = {{3, font.sfnt}, {4, error_at}, toc}, .memory = {w32(block - 0x10, 0)},
                                     .label = "delete sfnt spoiled block " + hex(block) + " (real longjmp) font " + hex(font.sfnt)});
        }
        cases.push_back({.entry = delete_sfnt, .registers = {{3, font.sfnt}, {4, 0}, toc}, .label = "delete sfnt no error pointer font " + hex(font.sfnt)});
        cases.push_back({.entry = delete_sfnt, .registers = {{3, font.sfnt}, {4, error_at}, toc}, .memory = {w32(error_at, 0x55555555)},
                         .label = "delete sfnt with error pointer font " + hex(font.sfnt)});
        cases.push_back({.entry = delete_sfnt, .registers = {{3, font.sfnt}, {4, error_at}, toc}, .answers = {{setjmp_entry, 0x271f}},
                         .label = "delete sfnt setjmp answered with an error code font " + hex(font.sfnt)});
        // An sfnt that holds every optional table (a collection header, vhea, vmtx) over records of the case's own, its releases answered: the
        // arguments of each release are compared, nothing is freed.
        {
            std::vector<Write> full;
            copy_sfnt(full, scenario, font);
            full.push_back(w32(sfnt_at + 0x8, fake_record));
            full.push_back(w32(sfnt_at + 0x1c, fake_vhea));
            full.push_back(w32(sfnt_at + 0x24, fake_loca));
            for (const auto record : {fake_record, fake_vhea, fake_loca})
                for (std::uint32_t at = 0; at < 0x20; at += 4) full.push_back(w32(record + at, 0x60060000 + record % 0x1000 + at));
            cases.push_back({.entry = delete_sfnt, .registers = {{3, sfnt_at}, {4, 0}, toc}, .memory = full, .answers = {{0x10767bb0, 0}},
                             .label = "delete sfnt with ttc vhea vmtx font " + hex(font.sfnt)});
            auto again = full;
            rewind(again, font);
            cases.push_back({.entry = read_sfnt, .registers = {{3, sfnt_at}, {4, font.stream}, {5, 0}, toc}, .memory = again, .answers = {{0x10767bb0, 0}},
                             .label = "read sfnt again with ttc vhea vmtx (releases answered) font " + hex(font.sfnt)});
        }
        // The sfnt without its optional tables, and with kern, in a copy.
        std::vector<Write> bare;
        copy_sfnt(bare, scenario, font);
        for (const std::uint32_t field : {0x8u, 0xcu, 0x18u, 0x1cu, 0x20u, 0x24u, 0x28u, 0x2cu}) bare.push_back(w32(sfnt_at + field, 0));
        cases.push_back({.entry = delete_sfnt, .registers = {{3, sfnt_at}, {4, 0}, toc}, .memory = bare, .label = "delete sfnt without tables font " + hex(font.sfnt)});
    }
    // The rare cases first: the call trace pairs only the first 32 cases of an entry and every 64th after. The rarest (the collection, vhea and
    // vmtx, fvar, the callback, the format 6 cmap, the releases of the full sfnt) come before the other rare ones.
    const auto priority = [](const Case& item) {
        for (const char* word : {"collection", "with vhea", "with vmtx", "with fvar", "callback", "reader answered", "allocation", "real longjmp", "long metrics", "cmap format 6", "with ttc", "setjmp", "with error pointer", "format 0 chosen"})
            if (item.label.find(word) != std::string::npos) return 0;
        for (const char* word : {"assertion", "no ", "cmap read on first use", "again", "format 2 with", "cmap format"})
            if (item.label.find(word) != std::string::npos) return 1;
        return 2;
    };
    std::stable_sort(cases.begin() + std::ptrdiff_t(first_case), cases.end(), [&](const Case& a, const Case& b) { return priority(a) < priority(b); });
    for (auto it = cases.begin() + std::ptrdiff_t(first_case); it != cases.end(); ++it) it->label = "truetype_glyph: " + it->label;
}
}
