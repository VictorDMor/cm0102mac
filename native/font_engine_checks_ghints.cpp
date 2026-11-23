// The font engine, block `ghints` (GHINTS.C, 0x1075ac40..0x1075b970): cases for the five routines of the global hint data. The two extreme leaves run on
// synthetic glyph records built in the scratch area (shapes at the edges of the halfwords, one-point and reversed contours, counts past the arrays); the
// T2KG reader on streams over random bytes (the version word, the exact length and one byte short, the reporter real and answered); the median and the
// analysis on the snapshot's three real sfnts (found by the scalers' stamps) with the pool's letter lists, lists of every character, lists of the
// limit's length and the glyph limit poked; the analysis needs a hinter handle, which a prepared case makes by running the original constructor
// 0x107555d0 on the run's own state (the autogridder's analysis 0x10754f70 runs generated on both sides, or is answered and its four result arrays
// written by a poke: the arrays' values, counts and null pointers the real analysis does not produce). Labels start with "ghints:"
// (CM_FONT_ENGINE_BLOCK=ghints).
#include "font_engine_checks.hpp"
#include "font_engine_adapter.hpp"
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t highest = 0x1075ac40, lowest = 0x1075acd0, read_table = 0x1075ad60, median = 0x1075ae30, analyse = 0x1075af60;
constexpr std::uint32_t reporter = 0x107755c0, game_toc = 0x108bba90, analysis = 0x10754f70, hinter_new = 0x107555d0;
constexpr std::uint32_t area = scratch_base + 0x90000, glyph_at = area, starts_at = area + 0x100, ends_at = area + 0x200, ys_at = area + 0x300, stream_at = area + 0x1000,
                        data_at = area + 0x1100, buffer_at = area + 0x1400, list_at = area + 0x2000, cell_at = area + 0x2800;
constexpr std::pair<unsigned, std::uint32_t> toc{2, game_toc};

struct Font { std::uint32_t t2k, mem, sfnt; };
std::vector<Font> fonts_of(const Scenario& scenario) {
    std::vector<Font> found;
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4)
            if (scenario.r32(std::uint32_t(at)) == 0x5a1234a5 && scenario.r32(std::uint32_t(at) + 0x5f4) == 0xa5fedc5a) {
                const auto t = std::uint32_t(at), sfnt = scenario.r32(t + 0x144);
                if (std::ranges::none_of(found, [&](const Font& f) { return f.sfnt == sfnt; })) found.push_back({t, scenario.r32(t + 4), sfnt});
            }
    }
    return found;
}

// A glyph record for the extreme leaves: contours (start, end, ys) with the arrays after the record.
struct Contour { int start, end; std::vector<int> ys; };
std::vector<Write> glyph_of(int contour_count, const std::vector<Contour>& contours, int y_offset = 0) {
    std::vector<Write> memory{w16(glyph_at + 0x36, std::uint32_t(contour_count)), w32(glyph_at + 0x3c, starts_at), w32(glyph_at + 0x40, ends_at), w32(glyph_at + 0x48, ys_at)};
    for (std::size_t n = 0; n < contours.size(); ++n) {
        memory.push_back(w16(starts_at + 2 * std::uint32_t(n), std::uint32_t(contours[n].start)));
        memory.push_back(w16(ends_at + 2 * std::uint32_t(n), std::uint32_t(contours[n].end)));
        for (std::size_t k = 0; k < contours[n].ys.size(); ++k) memory.push_back(w16(ys_at + 2 * std::uint32_t(contours[n].start + y_offset) + 2 * std::uint32_t(k), std::uint32_t(contours[n].ys[k])));
    }
    return memory;
}
}

FONT_ENGINE_CASES("ghints") {
    std::mt19937 random(0x6817);
    const auto first_case = cases.size();
    const bool exhaustive = std::getenv("CM_FONT_ENGINE_EXHAUSTIVE") != nullptr;
    const std::pair<unsigned, std::uint32_t> r4{4, 0x44444444}, r5{5, 0x55555555}, r6{6, 0x66666666}, r7{7, 0x77777777}, r8{8, 0x88888888}, r9{9, 0x99999999}, r10{10, 0xaaaaaaaa};
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, const std::string& label) -> Case& {
        registers.push_back(toc);
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = "ghints: " + label});
        return cases.back();
    };

    // ---- 0x1075ac40 and 0x1075acd0: the highest and the lowest y ----------------------------------------------------------------------
    for (const auto entry : {highest, lowest}) {
        const std::string name = entry == highest ? "highest " : "lowest ";
        const auto run = [&](int count, const std::vector<Contour>& contours, const std::string& label) {
            add(entry, {{3, glyph_at}, r4, r5, r6, r7, r8, r9, r10}, glyph_of(count, contours), name + label);
        };
        run(0, {}, "no contours");
        run(-1, {{0, 3, {1, 2, 3, 4}}}, "negative count");
        run(-32768, {{0, 3, {1, 2, 3, 4}}}, "count -32768");
        run(1, {{0, 3, {5, 9, -3, 7}}}, "one contour");
        run(1, {{0, 0, {42}}}, "one point skipped");
        run(1, {{3, 0, {1, 2, 3, 4}}}, "start after end");
        run(1, {{2, 3, {-5, 8}}}, "two points");
        run(2, {{0, 3, {5, 9, -3, 7}}, {4, 7, {100, -100, 50, 60}}}, "two contours");
        run(2, {{0, 0, {900}}, {1, 3, {1, 2, 3}}}, "a skipped contour then one");
        run(3, {{0, 1, {-32768, -32768}}, {2, 3, {32767, 32767}}, {4, 5, {0, 0}}}, "the extremes");
        run(1, {{0, 4, {-32768, -32768, -32768, -32768, -32768}}}, "all the lowest");
        run(1, {{0, 4, {32767, 32767, 32767, 32767, 32767}}}, "all the highest");
        run(1, {{0, 3, {7, 7, 7, 7}}}, "all equal");
        run(1, {{0, 3, {-1, -2, -3, -4}}}, "all negative");
        run(1, {{0, 11, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}}}, "ascending");
        run(1, {{0, 11, {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1}}}, "descending");
        run(5, {{0, 1, {1, 2}}, {2, 3, {3, 4}}, {4, 5, {5, 6}}, {6, 7, {7, 8}}, {8, 9, {9, 10}}}, "five contours");
        run(4, {{0, 1, {1, 2}}, {2, 3, {3, 4}}}, "count past the arrays");
        for (unsigned n = 0; n < 60; ++n) {
            std::vector<Contour> contours;
            int next = 0;
            const int count = int(random() % 5);
            for (int k = 0; k < count; ++k) {
                const int length = int(random() % 9);
                Contour contour{next, next + length - int(random() % 2), {}};
                for (int j = 0; j < length + 1; ++j) contour.ys.push_back(int(std::int16_t(random() % 5 == 0 ? random() : random() % 200 - 100)));
                contours.push_back(contour);
                next += length + 1;
            }
            run(count + int(random() % 2), contours, "random " + std::to_string(n));
        }
    }

    // ---- 0x1075ad60: the T2KG data -------------------------------------------------------------------------------------------------------
    {
        std::vector<std::uint8_t> bytes(0x60);
        for (auto& byte : bytes) byte = std::uint8_t(random());
        const auto stream_of = [&](std::uint32_t position, std::uint32_t length, const std::vector<std::uint8_t>& content, std::uint32_t version) {
            std::vector<Write> memory{w32(stream_at, data_at), w32(stream_at + 4, position), w32(stream_at + 8, length)};
            auto data = content;
            data[position] = std::uint8_t(version >> 8);
            data[position + 1] = std::uint8_t(version);
            for (std::size_t n = 0; n < data.size(); ++n) memory.push_back(w8(data_at + std::uint32_t(n), data[n]));
            for (std::uint32_t n = 0; n < 0x6c; n += 4) memory.push_back(w32(buffer_at + n, 0xcdcdcdcdu));
            return memory;
        };
        const std::pair<unsigned, std::uint32_t> held29{29, 0x29292929}, held30{30, 0x30303030}, held31{31, 0x31313131};
        // The assertions first (the first cases are traced): a bad version, answered and real, and a stream one byte short.
        for (const bool answered : {false, true}) {
            auto& item = add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(0, 0x5a, bytes, 1),
                             std::string("T2KG data with version 1 ") + (answered ? "answered" : "real reporter"));
            if (answered) item.answers = {{reporter, 0}};
            auto& shortened = add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(0, 0x59, bytes, 0),
                                  std::string("T2KG data one byte short ") + (answered ? "answered" : "real reporter"));
            if (answered) shortened.answers = {{reporter, 0}};
        }
        add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(0, 0x5a, bytes, 0), "T2KG data exact");
        add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(0, 0x60, bytes, 0), "T2KG data with a tail");
        add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(6, 0x60, bytes, 0), "T2KG data from position 6");
        add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(0, 0, bytes, 0), "T2KG data empty stream").answers = {{reporter, 0}};
        for (const std::uint32_t version : {0x100u, 0x8000u, 0xffffu, 0x1u, 0x10000u})
            add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(0, 0x60, bytes, version), "T2KG data version " + hex(version)).answers = {{reporter, 0}};
        for (unsigned n = 0; n < 80; ++n) {
            auto random_bytes = bytes;
            for (auto& byte : random_bytes) byte = std::uint8_t(n % 4 == 0 ? (random() % 3 == 0 ? 0xff : 0x7f) : random());
            add(read_table, {{3, buffer_at}, {4, stream_at}, r5, r6, r7, r8, r9, r10, held29, held30, held31}, stream_of(0, 0x5a + random() % 6, random_bytes, 0), "T2KG data random " + std::to_string(n));
        }
    }

    // ---- 0x1075ae30: the median height ---------------------------------------------------------------------------------------------------
    const auto fonts = fonts_of(scenario);
    const auto pool = scenario.toc(-0x3c48);
    for (const auto& font : fonts) {
        const std::string name = "font " + hex(font.sfnt) + " ";
        // The pool's twenty lists, tops and bottoms.
        for (const std::uint32_t offset : {0x16u, 0x1cu, 0x1eu, 0x24u, 0x29u, 0x2bu, 0x2fu, 0x31u, 0x35u, 0x3du, 0x43u, 0x45u, 0x4au, 0x4fu, 0x56u, 0x59u, 0x5bu, 0x5eu})
            for (const std::uint32_t top : {1u, 0u})
                add(median, {{3, font.sfnt}, {4, pool + offset}, {5, top}, r6, r7, r8, r9, r10}, {}, name + "pool list " + hex(offset) + (top ? " top" : " bottom"));
        // The flag is read as a halfword.
        for (const std::uint32_t top : {0x10000u, 0x8000u, 0xffffu, 2u})
            add(median, {{3, font.sfnt}, {4, pool + 0x16}, {5, top}, r6, r7, r8, r9, r10}, {}, name + "flag " + hex(top));
        // Every character on its own, then in groups (the lists are zero-terminated and at most 32 long).
        for (unsigned code = 1; code < 256; code += (code < 0x20 ? 1 : 3)) {
            std::vector<Write> memory{w8(list_at, code), w8(list_at + 1, 0)};
            add(median, {{3, font.sfnt}, {4, list_at}, {5, code & 1}, r6, r7, r8, r9, r10}, memory, name + "character " + hex(code));
        }
        for (unsigned first = 1; first < 256; first += 17) {
            std::vector<Write> memory;
            for (unsigned n = 0; n < 40; ++n) memory.push_back(w8(list_at + n, first + n < 256 ? first + n : 0));
            add(median, {{3, font.sfnt}, {4, list_at}, {5, 1}, r6, r7, r8, r9, r10}, memory, name + "forty characters from " + hex(first));
            add(median, {{3, font.sfnt}, {4, list_at}, {5, 0}, r6, r7, r8, r9, r10}, memory, name + "forty characters (bottoms) from " + hex(first));
        }
        add(median, {{3, font.sfnt}, {4, list_at}, {5, 1}, r6, r7, r8, r9, r10}, {w8(list_at, 0)}, name + "empty list");
        {
            std::vector<Write> memory;
            for (unsigned n = 0; n < 33; ++n) memory.push_back(w8(list_at + n, 0x41 + n % 26));
            add(median, {{3, font.sfnt}, {4, list_at}, {5, 1}, r6, r7, r8, r9, r10}, memory, name + "33 characters, no terminator before the 33rd");
        }
    }
    // ---- 0x1075af60: the measured data -----------------------------------------------------------------------------------------------------
    // A prepared case: the hinter's handle is made by the ORIGINAL constructor on the run's own state (the memory object, the font's point limit and
    // units per em), then r4 is set to it. `limit` pokes the glyph count the analysis loops over (maxp's numGlyphs; loca's count is made larger).
    const auto prepared = [&](const Font& font, std::uint32_t mode, std::vector<Write> memory, const std::string& label,
                              std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}, std::function<void(guest::Cpu&, std::uint32_t)> extra = nullptr) -> Case& {
        for (std::uint32_t n = 0; n < 0x6c; n += 4) memory.push_back(w32(buffer_at + n, 0xcdcdcdcdu));
        auto& item = add(analyse, {{3, font.sfnt}, {4, 0}, {5, buffer_at}, {6, mode}, r7, r8, r9, r10, {21, 0x21212121}, {25, 0x25252525}, {28, 0x28282828}, {29, 0x29292929}, {31, 0x31313131}},
                         std::move(memory), label);
        item.answers = std::move(answers);
        item.prepare = [font, extra = std::move(extra)](State& s) {
            guest::Cpu c(s);
            const auto saved = s.registers;
            c.set(2, game_toc);
            c.set(3, font.sfnt);
            font_engine_run_original(s, 0x10766d30);
            const auto points = std::uint32_t(std::int32_t(std::int16_t(c.r(3))));
            c.set(3, font.sfnt);
            font_engine_run_original(s, 0x10766cc0);
            const auto units = std::uint32_t(std::int32_t(std::int16_t(c.r(3))));
            c.put32(cell_at, 0);
            c.set({{3, font.mem}, {4, points}, {5, units}, {6, cell_at}});
            font_engine_run_original(s, hinter_new);
            const auto handle = c.u32(cell_at);
            if (extra) extra(c, font.mem);
            s.registers = saved;
            c.set(4, handle);
        };
        return item;
    };
    const auto limit_of = [&](const Font& font, std::uint32_t limit) {
        const auto maxp = scenario.r32(font.sfnt + 0x10), loca = scenario.r32(font.sfnt + 0x14);
        std::vector<Write> memory{w16(maxp + 8, limit)};
        if (loca) memory.push_back(w32(loca + 8, 0x20000));
        return memory;
    };
    for (const auto& font : fonts) {
        const std::string name = "analysis of font " + hex(font.sfnt) + " ";
        for (const std::uint32_t mode : {0u, 1u, 0x10000u}) prepared(font, mode, {}, name + "mode " + hex(mode));
        // The glyph blocks: limits around the multiples of 32 and 64 and past the arrays' 32 samples.
        for (const std::uint32_t limit : {0u, 1u, 2u, 5u, 31u, 32u, 33u, 63u, 64u, 65u, 100u, 127u, 128u, 200u, 400u, 1000u, 0x7fffu, 0x8000u, 0xffffu})
            for (const std::uint32_t mode : {1u, 0u})
                prepared(font, mode, limit_of(font, limit), name + "limit " + hex(limit) + " mode " + hex(mode));
    }
    // The pool's letter lists replaced by random letters (the medians, and with them the data the tidying works on, then take other values than the three
    // fonts' own: the clamps, the near-equal levels and the two orders of the final pair), and no sample at all (a glyph limit of -1: loca without entries).
    {
        struct Slot { std::uint32_t offset, length; };
        const Slot slots[] = {{0x16, 5}, {0x1c, 1}, {0x1e, 5}, {0x24, 4}, {0x29, 1}, {0x2b, 3}, {0x2f, 1}, {0x31, 3}, {0x35, 7}, {0x3d, 5}, {0x43, 1}, {0x45, 4}, {0x4a, 4},
                              {0x4f, 6}, {0x56, 2}, {0x59, 1}, {0x5b, 2}, {0x5e, 4}};
        for (const auto& font : fonts) {
            if (scenario.u16(scenario.r32(font.sfnt + 0x10) + 8) <= 100) continue;
            const std::uint32_t count = exhaustive ? 400 : 150;
            for (std::uint32_t n = 0; n < count; ++n) {
                std::vector<Write> memory;
                for (const auto& slot : slots) {
                    const auto length = 1 + random() % slot.length;
                    for (std::uint32_t k = 0; k < length; ++k) memory.push_back(w8(pool + slot.offset + k, n % 3 == 0 ? 0x41 + random() % 58 : 0x20 + random() % 224));
                    memory.push_back(w8(pool + slot.offset + length, 0));
                }
                prepared(font, 0, memory, "analysis of font " + hex(font.sfnt) + " with random letters " + std::to_string(n));
            }
        }
    }
    for (const auto& font : fonts) {
        const auto maxp = scenario.r32(font.sfnt + 0x10), loca = scenario.r32(font.sfnt + 0x14);
        if (!loca) continue;
        for (const std::uint32_t mode : {0u, 1u}) prepared(font, mode, {w16(maxp + 8, 5), w32(loca + 8, 0)}, "analysis of font " + hex(font.sfnt) + " with a glyph limit of -1, mode " + hex(mode));
        // No sample: the sorted arrays are empty and the original reads their first halfwords as the stack left them (frame + 0x9c and + 0x5c), which the
        // case sets: the medians' two orders, one apart, equal, two apart and the extremes.
        for (const int high : {0, 5, -5, 0x7fff, -0x8000, 100})
            for (const int low : {0, 5, 4, 6, 3, 7, -5, -6, 0x7fff, -0x8000, 99, 101, 102}) {
                auto& item = prepared(font, 1, {w16(maxp + 8, 5), w32(loca + 8, 0)}, "analysis of font " + hex(font.sfnt) + " with no sample, stale medians " + std::to_string(high) + " and " + std::to_string(low),
                                      {}, [=](guest::Cpu& c, std::uint32_t) {
                                          c.put16(c.sp() - 0x140 + 0x9c, std::uint32_t(high));
                                          c.put16(c.sp() - 0x140 + 0x5c, std::uint32_t(low));
                                      });
                (void)item;
            }
        // The glyph limits of many blocks of glyphs (real analyses: the medians of the samples the glyphs give).
        for (std::uint32_t limit = 64; limit <= (exhaustive ? 1200u : 400u); limit += exhaustive ? 1 : 7)
            prepared(font, 1, limit_of(font, limit), "analysis of font " + hex(font.sfnt) + " limit " + hex(limit) + " (sweep)");
    }
    // The analysis answered (its body not run) with result arrays of chosen counts and values, written by a poke where the original wrote them.
    if (!fonts.empty()) {
        // A font that has the letter 'o' (more than a hundred glyphs): the first analysis is of that glyph in mode 0 and of a block of glyphs in mode 1.
        const auto large = std::ranges::find_if(fonts, [&](const Font& f) { return scenario.u16(scenario.r32(f.sfnt + 0x10) + 8) > 100; });
        const auto& font = large != fonts.end() ? *large : fonts[0];
        // The result arrays are real blocks of the font's allocator, made by the ORIGINAL allocator in the prepared state and left in the frame's result slots
        // (the analysis is answered: its body, which would write them, does not run; the frame's words at +0x48, +0x40, +0x44, +0x3c are read where it leaves them).
        // The releases that follow are real. Only the first analysis of a run gets arrays (the original clears the slots as it frees them): a loop that analyses
        // a second glyph then finds the slots empty (the assertion, then a fault on both sides).
        const auto answered = [&](std::uint32_t result, bool block_one, std::uint32_t count_one, bool block_two, std::uint32_t count_two,
                                  const std::vector<int>& one, const std::vector<int>& two, std::uint32_t mode, std::uint32_t limit, const std::string& label) {
            prepared(font, mode, limit_of(font, limit), "analysis answered " + label, {{analysis, result}},
                     [=](guest::Cpu& c, std::uint32_t mem) {
                         const auto make = [&](const std::vector<int>& values) {
                             c.set({{3, mem}, {4, 64}});
                             font_engine_run_original(c.s, 0x107678a0);
                             const auto block = c.r(3);
                             for (std::size_t n = 0; n < values.size(); ++n) c.put16(block + 2 * std::uint32_t(n), std::uint32_t(values[n]));
                             return block;
                         };
                         const auto frame = c.sp() - 0x140;
                         c.put32(frame + 0x48, block_one ? make(one) : 0);
                         c.put32(frame + 0x40, count_one);
                         c.put32(frame + 0x44, block_two ? make(two) : 0);
                         c.put32(frame + 0x3c, count_two);
                     });
        };
        const std::vector<int> ascending{-300, -20, -1, 0, 1, 20, 300, 0x7fff}, mixed{50, -50, 7, 0x7fff, -0x8000, 3, 3, 9};
        // The assertions: a failed analysis, missing arrays (the real reporter, answered by the checker's dialog hook).
        answered(1, true, 3, true, 3, ascending, mixed, 0, 400, "failed (1), mode 0");
        answered(0xffffffffu, true, 3, true, 3, ascending, mixed, 0, 400, "failed (-1), mode 0");
        answered(0, false, 3, true, 3, ascending, mixed, 0, 400, "first array missing, mode 0");
        answered(0, true, 3, false, 3, ascending, mixed, 0, 400, "second array missing, mode 0");
        answered(0, false, 0, false, 0, ascending, mixed, 0, 400, "no arrays, mode 0");
        answered(1, true, 3, true, 3, ascending, mixed, 1, 400, "failed (1), mode 1");
        answered(0, false, 3, true, 3, ascending, mixed, 1, 400, "first array missing, mode 1");
        answered(0, true, 3, false, 3, ascending, mixed, 1, 400, "second array missing, mode 1");
        for (const std::uint32_t count : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u}) {
            const std::string label = "mode 0 counts " + hex(count);
            answered(0, true, count, true, count, ascending, mixed, 0, 400, label);
            answered(0, true, count, true, 8 - count, mixed, ascending, 0, 400, label + " split");
        }
        for (const std::uint32_t count : {0u, 1u, 3u}) answered(0, true, count, true, count, ascending, mixed, 1, 400, "mode 1 counts " + hex(count) + " (the first analysis only)");
        // The two medians one apart, equal, reversed.
        for (const int a : {10, 11, 12, -1})
            for (const int b : {10, 11, 12, 9})
                answered(0, true, 1, true, 1, {a}, {b}, 0, 400, "medians " + std::to_string(a) + " and " + std::to_string(b));
        answered(0, true, 1, true, 1, {0x7fff}, {-0x8000}, 0, 400, "medians at the extremes");
        answered(0, true, 1, true, 1, {-0x8000}, {0x7fff}, 0, 400, "medians at the extremes, reversed");
    }
    // The shapes of the stack the original leaves: registers the caller holds.
    if (!fonts.empty())
        add(median, {{3, fonts[0].sfnt}, {4, pool + 0x16}, {5, 1}, {26, 0x26262626}, {27, 0x27272727}, {28, 0x28282828}, {29, 0x29292929}, {30, 0x30303030}, {31, 0x31313131}, r6, r7, r8, r9, r10}, {}, "held registers");
    std::stable_partition(cases.begin() + std::ptrdiff_t(first_case), cases.end(), [](const Case& item) { 
        for (const char* rare : {"T2KG data with version 1", "one byte short", "failed", "missing", "no arrays"}) if (item.label.find(rare) != std::string::npos) return true;
        return false; });
}
}
