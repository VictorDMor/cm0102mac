// The font engine, block `t2k_core` (T2K.C, 0x1075e960..0x10761070): cases for the scaler object's constructor, transformation, the purge, the
// renderer of one glyph (and the character entry the game calls), the hinter's setup (dormant for the game), the coordinate scaling and the delete.
// Everything runs on the snapshot's own scalers (found by their stamps: one per font and style the game had loaded), their memory objects and
// sfnts, so the allocator, the glyph readers and the scan converter (still generated) are the real ones on both sides; a case that needs a state
// the game does not leave (a size, a matrix, a table missing, a value at an edge, a block spoiled so that the release raises) writes it into the
// scaler or the font (undone after the case). The error paths are real: a spoiled block makes the allocator longjmp to the entry's setjmp, a
// full table makes the allocation raise, an answered callee (Case::answers) stands in for the hinter. Labels start with "t2k_core:"
// (CM_FONT_ENGINE_BLOCK=t2k_core).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <cstdlib>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t new_t2k = 0x1075e960, prepare_hinting = 0x1075eb10, scale_coordinates = 0x1075ece0, set_transformation = 0x1075eef0,
                        release_glyph = 0x1075f870, purge = 0x1075f980, render_glyph = 0x1075fa10, render_character = 0x10760e30, delete_t2k = 0x10760f50;
constexpr std::uint32_t hinter_new = 0x107555d0, hinter_size = 0x10755930, hinter_delete = 0x10755840, hinter_setup = 0x107551c0, hinter_glyph = 0x107552b0,
                        hinter_copy = 0x1075ad60, hinter_make = 0x1075af60;
constexpr std::uint32_t reporter = 0x107755c0, setjmp_entry = 0x10773eb0, game_toc = 0x108bba90, block_stamp = 0xaa53c5aa;
constexpr std::uint32_t area = scratch_base + 0x80000, error_at = area, matrix_at = area + 0x100, ratio_at = area + 0x200, source_at = area + 0x1000,
                        destination_at = area + 0x3000;

struct Scaler { std::uint32_t t2k, mem, sfnt, stream; bool t2kg; std::uint32_t data_block; };
std::vector<Scaler> scalers_of(const Scenario& scenario) {
    std::vector<Scaler> found;
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x5f8 <= std::uint64_t(low) + size; at += 4)
            if (scenario.r32(std::uint32_t(at)) == 0x5a1234a5 && scenario.r32(std::uint32_t(at) + 0x5f4) == 0xa5fedc5a) {
                const auto t = std::uint32_t(at), sfnt = scenario.r32(t + 0x144);
                // The font's T2KG table (the .t2k fonts have one, symbol.ttf not): the hinter's data for the font is read from it, else made by the hinter's own
                // routine, which does not write every byte of the 0x6c-byte buffer on the stack that is then copied: the original copies the stack's leftovers,
                // which a native rewrite cannot reproduce (dormant: the game never hints). The cases give such a font a hint data block of its own instead.
                const auto directory = scenario.r32(sfnt), count = scenario.u16(directory + 8), array = scenario.r32(directory + 0x10);
                bool t2kg = false;
                for (std::uint32_t n = 0; n < count; ++n) t2kg = t2kg || scenario.r32(scenario.r32(array + 4 * n) + 4) == 0x54324b47;
                found.push_back({t, scenario.r32(t + 4), sfnt, scenario.r32(sfnt + 0x54), t2kg, 0});
            }
    }
    return found;
}
struct Block { std::uint32_t user, size; };
// The live blocks a memory object's table holds.
std::vector<Block> blocks_of(const Scenario& scenario, std::uint32_t mem) {
    std::vector<Block> blocks;
    const auto table = scenario.r32(mem + 0xc);
    for (std::uint32_t n = 0; n < scenario.r32(mem + 8); ++n) {
        const auto header = scenario.r32(table + 4 * n);
        if (header && scenario.r32(header) == block_stamp) blocks.push_back({header + 0x10, scenario.r32(header + 4)});
    }
    return blocks;
}
}

FONT_ENGINE_CASES("t2k_core") {
    std::mt19937 random(0x72c0);
    const auto first_case = cases.size();
    // The default set keeps this block's share of the equivalence test small (about two minutes with the mutated runs); CM_FONT_ENGINE_EXHAUSTIVE=1 adds
    // every command combination, every character and the full grids of the coordinate scaling.
    const bool exhaustive = std::getenv("CM_FONT_ENGINE_EXHAUSTIVE") != nullptr;
    const auto thin = [&](unsigned n, unsigned keep_every) { return exhaustive || n % keep_every == 0; };
    auto scalers = scalers_of(scenario);
    for (auto& s : scalers) {
        const auto blocks = blocks_of(scenario, s.mem);
        const auto found = std::ranges::find_if(blocks, [](const Block& b) { return b.size >= 0x6c; });
        if (found != blocks.end()) s.data_block = found->user;
    }
    // The writes that give a font without a T2KG table its hint data (see scalers_of).
    const auto hint_data = [&](const Scaler& s) -> std::vector<Write> {
        if (s.t2kg || s.data_block == 0) return {};
        return {w32(s.sfnt + 0x38, s.data_block)};
    };
    // The renderer entered directly has no setjmp of its own: the memory object's jump buffer is the one an earlier API call left, whose frame is
    // gone, and a mutated input that makes the allocator raise would resume on that dead stack (never in the game: 0x10760e30 is the one caller,
    // under its setjmp). The buffer is filled with addresses nothing maps, so such a run stops at the same place on both sides.
    const auto dead_jump_buffer = [&](const Scaler& s) {
        std::vector<Write> memory;
        for (std::uint32_t at = 0x10; at < 0x128; at += 4) memory.push_back(w32(s.mem + at, 0x0bad0000 + at));  // up to the scratch slots
        return memory;
    };
    const auto with = [](std::vector<Write> memory, const std::vector<Write>& more) {
        memory.insert(memory.end(), more.begin(), more.end());
        return memory;
    };
    const auto toc = std::pair<unsigned, std::uint32_t>{2, game_toc};
    // r9 and r10 are what the callers read after each entry: distinct values go in, so a pass-through shows (the arguments that are r9
    // are left alone).
    const std::pair<unsigned, std::uint32_t> r9{9, 0x91919191}, r10{10, 0xa0a0a0a0};
    const auto add = [&](std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> memory, const std::string& label,
                         std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) -> Case& {
        registers.push_back(toc);
        cases.push_back({.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .label = label, .answers = std::move(answers)});
        return cases.back();
    };
    const auto name = [](const char* what, const Scaler& s) { return std::string(what) + " scaler " + hex(s.t2k); };
    // ---- 0x1075ece0: the coordinates scaled by a ratio record ---------------------------------------------------------------------
    {
        const std::pair<unsigned, std::uint32_t> r9v{9, 0x12345678}, r10v{10, 0x0abcdef1};
        std::vector<std::int16_t> samples{0, 1, -1, 2, 100, -100, 0x7fff, -0x8000, 0x1234, -0x1234, 7, -7, 63, 64, 65, -65};
        for (int n = 0; n < 120; ++n) samples.push_back(std::int16_t(random()));
        std::vector<Write> data;
        for (std::size_t n = 0; n < samples.size(); ++n) data.push_back(w16(source_at + 2 * std::uint32_t(n), std::uint16_t(samples[n])));
        const auto record = [&](std::int32_t kind, std::int32_t multiplier, std::int32_t shift, std::uint32_t divisor, std::uint32_t rounding, std::uint32_t fixed) {
            std::vector<Write> memory = data;
            memory.push_back(w16(ratio_at, std::uint32_t(multiplier)));
            memory.push_back(w16(ratio_at + 2, std::uint32_t(shift)));
            memory.push_back(w32(ratio_at + 4, divisor));
            memory.push_back(w32(ratio_at + 8, rounding));
            memory.push_back(w32(ratio_at + 0xc, fixed));
            memory.push_back(w16(ratio_at + 0x10, std::uint32_t(kind)));
            return memory;
        };
        const auto run = [&](std::int32_t count, std::vector<Write> memory, const std::string& label) {
            add(scale_coordinates, {{3, source_at}, {4, destination_at}, {5, std::uint32_t(count)}, {6, ratio_at}, r9v, r10v}, std::move(memory), "scale " + label);
        };
        // The unrolled-by-eight shape: every count around the multiples of eight, with the values that reach r10.
        unsigned grid = 0;
        for (const std::int32_t count : {-3, -1, 0, 1, 2, 7, 8, 9, 10, 15, 16, 17, 18, 23, 24, 25, 31, 32, 33, 100, 136})
            for (const std::int32_t shift : {0, 1, 6, 15})
                for (const std::int32_t multiplier : {1, 3, 64, -5})
                    if (thin(grid++, 5)) run(count, record(0, multiplier, shift, 1, 0x20, 0), "kind 0 count " + std::to_string(count) + " mult " + std::to_string(multiplier) + " shift " + std::to_string(shift));
        for (const std::int32_t shift : {-1, 31, 32, 33, 63, 64, 65, 0x7fff, -0x8000})
            for (const std::uint32_t rounding : {0u, 0x7fffffffu, 0x80000000u, 0xffffffffu})
                if (thin(grid++, 3)) run(20, record(0, 0x7fff, shift, 1, rounding, 0), "kind 0 shift " + std::to_string(shift) + " rounding " + hex(rounding));
        for (const std::int32_t multiplier : {0, -0x8000, 0x7fff, -1, 2})
            run(17, record(0, multiplier, 6, 1, 0x20, 0), "kind 0 mult " + std::to_string(multiplier));
        for (const std::int32_t count : {-1, 0, 1, 2, 8, 9, 17, 40})
            for (const std::uint32_t divisor : {1u, 2u, 3u, 64u, 100u, 0xffffffffu, 0x80000000u, 0u})
                for (const std::uint32_t rounding : {0u, 0x20u, 0x7fffffffu, 0x80000000u})
                    if (thin(grid++, 3)) run(count, record(1, divisor == 64 ? 3 : -7, 0, divisor, rounding, 0), "kind 1 count " + std::to_string(count) + " divisor " + hex(divisor) + " rounding " + hex(rounding));
        for (const std::int32_t multiplier : {1, -1, 0x7fff, -0x8000, 0})
            run(33, record(1, multiplier, 0, 7, 3, 0), "kind 1 mult " + std::to_string(multiplier));
        for (const std::int32_t count : {-1, 0, 1, 5, 8, 9, 40})
            for (const std::uint32_t fixed : {0x10000u, 0x8000u, 0xffff0000u, 0x12345678u, 0u, 0x80000000u, 0x7fffffffu, 0xffffffffu})
                if (thin(grid++, 3)) run(count, record(2, 0, 0, 0, 0, fixed), "kind 2 count " + std::to_string(count) + " factor " + hex(fixed));
        for (const std::int32_t kind : {3, 4, -1, -2, 0x7fff, -0x8000})
            run(9, record(kind, 3, 2, 5, 1, 0x10000), "kind " + std::to_string(kind) + " (nothing)");
    }
    // ---- 0x1075e960: the constructor ----------------------------------------------------------------------------------------------
    for (const auto& s : scalers) {
        add(new_t2k, {{3, s.mem}, {4, s.sfnt}, {5, error_at}, r9, r10}, {w32(error_at, 0x55555555)}, name("new", s));
        add(new_t2k, {{3, 0}, {4, s.sfnt}, {5, error_at}, r9, r10}, {}, name("new null memory object", s));
        add(new_t2k, {{3, s.mem}, {4, s.sfnt}, {5, error_at}, r9, r10}, {}, name("new setjmp answered 0x2718", s), {{setjmp_entry, 0x2718}});
        add(new_t2k, {{3, s.mem}, {4, s.sfnt}, {5, error_at}, r9, r10}, {}, name("new setjmp answered 0x2719", s), {{setjmp_entry, 0x2719}});
        add(new_t2k, {{3, s.mem}, {4, s.sfnt}, {5, 0}, r9, r10}, {}, name("new no error pointer (assertion answered)", s), {{reporter, 0}});
        add(new_t2k, {{3, s.mem}, {4, s.sfnt}, {5, 0}, r9, r10}, {}, name("new no error pointer", s));
        add(new_t2k, {{3, 0}, {4, s.sfnt}, {5, 0}, r9, r10}, {}, name("new null memory object no error pointer (assertion answered)", s), {{reporter, 0}});
        // A real longjmp: the table of the memory object nearly full, so the allocation raises 0x271c and the entry catches it.
        for (const std::uint32_t k : {0u, 1u, 3u})
            add(new_t2k, {{3, s.mem}, {4, s.sfnt}, {5, error_at}, r9, r10}, {w32(s.mem + 4, scenario.r32(s.mem + 8) - k)}, name(("new table full after " + std::to_string(k) + " allocations (real longjmp)").c_str(), s));
        // Another sfnt, none, and the platform ids.
        for (const auto& other : scalers)
            if (other.sfnt != s.sfnt) add(new_t2k, {{3, s.mem}, {4, other.sfnt}, {5, error_at}, r9, r10}, {}, name(("new over the sfnt of " + hex(other.t2k)).c_str(), s));
    }
    // ---- the transformation --------------------------------------------------------------------------------------------------------
    // The matrices the game gives (FONT.CPP: the size in 16.16 on the diagonal and the skew of its italic in the xy word, 0 for yx) and others.
    struct Matrix { std::uint32_t m[4]; const char* label; };
    const std::vector<Matrix> matrices = {
        {{0x10000, 0, 0, 0x10000}, "identity"}, {{0x90000, 0x1234, 0, 0x90000}, "game size 9 skew"}, {{0x160000, 0xffff9000, 0, 0x160000}, "game size 22 skew"},
        {{0x10000, 0x4000, 0, 0x10000}, "shear"}, {{0, 0x10000, 0xffff0000, 0}, "quarter turn"}, {{0xb505, 0xb505, 0xffff4afb, 0xb505}, "eighth turn"},
        {{0x20000, 0, 0, 0x8000}, "stretch"}, {{0xffff0000, 0, 0, 0x10000}, "mirror"}, {{0, 0, 0, 0}, "zero"}, {{0x7fffffff, 0, 0, 0x7fffffff}, "huge"},
        {{0x1, 0, 0, 0x1}, "tiny"}, {{0x10000, 0x10000, 0x10000, 0x10000}, "singular"}, {{0x80000000, 0, 0, 0x80000000}, "minimum"}};
    const auto matrix_memory = [&](const Matrix& matrix) {
        std::vector<Write> memory;
        for (std::uint32_t n = 0; n < 4; ++n) memory.push_back(w32(matrix_at + 4 * n, matrix.m[n]));
        return memory;
    };
    for (const auto& s : scalers) {
        for (const std::uint32_t size : {1u, 9u, 14u, 22u, 72u, 0x7fffu, 0x8000u, 0x9001u, 0x1ffffu})
            for (const auto& matrix : matrices) {
                if (size != 9 && size != 22 && matrix.m[0] != 0x10000 && matrix.m[0] != 0x90000) continue;
                add(set_transformation, {{3, s.t2k}, {4, 0}, {5, size}, {6, size}, {7, matrix_at}, {8, 0x31}, {9, error_at}, r10}, matrix_memory(matrix),
                    name(("transform size " + std::to_string(size) + " " + matrix.label).c_str(), s));
            }
        // Different x and y sizes, the extra word, the hint flag (the real hinter, then answered).
        for (const std::uint32_t y : {1u, 9u, 33u})
            add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, y}, {7, matrix_at}, {8, 0xcafe}, {9, error_at}, r10}, matrix_memory(matrices[0]),
                name(("transform sizes 14 and " + std::to_string(y)).c_str(), s));
        for (const auto& matrix : {matrices[0], matrices[1], matrices[5]})
            add(set_transformation, {{3, s.t2k}, {4, 1}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, with(matrix_memory(matrix), hint_data(s)),
                name((std::string("transform with the hinter, ") + matrix.label).c_str(), s));
        // The errors: a null matrix, sizes that are not positive (real longjmps), no error pointer, the setjmp answered.
        add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, 0}, {8, 0}, {9, error_at}, r10}, {}, name("transform null matrix (real longjmp)", s));
        for (const std::uint32_t bad : {0u, 0xffffffffu, 0x80000000u}) {
            add(set_transformation, {{3, s.t2k}, {4, 0}, {5, bad}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, matrix_memory(matrices[0]), name(("transform size x " + hex(bad) + " (real longjmp)").c_str(), s));
            add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, bad}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, matrix_memory(matrices[0]), name(("transform size y " + hex(bad) + " (real longjmp)").c_str(), s));
        }
        add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, 0}, r10}, matrix_memory(matrices[0]), name("transform no error pointer (assertion answered)", s), {{reporter, 0}});
        add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, matrix_memory(matrices[0]), name("transform setjmp answered", s), {{setjmp_entry, 0x2718}});
        add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, matrix_memory(matrices[0]), name("transform raise answered", s), {{0x10773f30, 0}});
        // The glyph cache of the object made afresh or kept, its size; the font's glyph count and units per em at the edges; the metrics tables.
        const auto maxp = scenario.r32(s.sfnt + 0x10), head = scenario.r32(s.sfnt + 0xc), hhea = scenario.r32(s.sfnt + 0x18);
        add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10},
            [&] { auto m = matrix_memory(matrices[0]); m.push_back(w32(s.t2k + 0x18, 0)); return m; }(), name("transform no glyph cache yet", s));
        for (const std::uint32_t glyphs : {0u, 1u, 7u, 8u, 0x7ffu, 0x800u, 0x801u, 0xffffu}) {
            auto memory = matrix_memory(matrices[0]);
            memory.push_back(w32(s.t2k + 0x18, 0));
            memory.push_back(w16(maxp + 8, glyphs));
            if (scenario.r32(s.sfnt + 0x14) != 0) memory.push_back(w32(scenario.r32(s.sfnt + 0x14) + 8, glyphs));
            add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, memory, name(("transform cache for " + hex(glyphs) + " glyphs").c_str(), s));
        }
        for (const std::uint32_t upem : {0u, 1u, 0x400u, 0x800u, 0xffffu}) {
            auto memory = matrix_memory(matrices[0]);
            memory.push_back(w16(head + 0x16, upem));
            add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, memory, name(("transform units per em " + hex(upem)).c_str(), s));
        }
        for (const auto& [run, rise] : {std::pair{0, 0}, {1, 0}, {0, 1}, {0x4000, 0x100}, {0x8000, 0x8000}, {0x7fff, 0x7fff}, {0xffff8000, 0x1}, {1, 0xffff8000}, {0x10, 0x20}}) {
            auto memory = matrix_memory(matrices[1]);
            memory.push_back(w16(hhea + 0x18, std::uint32_t(run)));
            memory.push_back(w16(hhea + 0x16, std::uint32_t(rise)));
            add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, memory,
                name(("transform caret slope " + hex(std::uint32_t(run)) + " " + hex(std::uint32_t(rise))).c_str(), s));
        }
        {   // No horizontal header, and a font with a vertical one (hhea copied into vhea).
            auto memory = matrix_memory(matrices[1]);
            memory.push_back(w32(s.sfnt + 0x18, 0));
            add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, memory, name("transform no hhea", s));
            for (const auto& matrix : {matrices[0], matrices[1], matrices[5], matrices[4]}) {
                auto vertical = matrix_memory(matrix);
                vertical.push_back(w32(s.sfnt + 0x1c, hhea));
                add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, vertical,
                    name((std::string("transform with a vhea, ") + matrix.label).c_str(), s));
                auto only = vertical;
                only.push_back(w32(s.sfnt + 0x18, 0));
                add(set_transformation, {{3, s.t2k}, {4, 0}, {5, 9}, {6, 9}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, only,
                    name((std::string("transform with a vhea and no hhea, ") + matrix.label).c_str(), s));
            }
        }
        // The same size twice (the cache's tables are kept), and the cache tables stale.
        add(set_transformation, {{3, s.t2k}, {4, 0}, {5, scenario.r32(s.t2k + 0xe8)}, {6, scenario.r32(s.t2k + 0xec)}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, matrix_memory(matrices[0]),
            name("transform the size it has", s));
    }
    // ---- the hinter's setup (dormant for the game) ----------------------------------------------------------------------------------
    for (const auto& s : scalers) {
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, hint_data(s), name("hinting real", s));
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, with({w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 14)}, hint_data(s)), name("hinting real size 14", s));
        if (s.t2kg) add(prepare_hinting, {{3, s.t2k}, r9, r10}, {w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 14), w32(s.t2k + 0x148, 0), w32(scenario.r32(s.t2k + 0x144) + 0x38, 0)},
            name("hinting real, no hint data", s));
        const auto sized = with({w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 17)}, hint_data(s));
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, sized, name("hinting handle answered 0", s), {{hinter_new, 0}, {hinter_make, 0}, {hinter_copy, 0}, {hinter_setup, 0}, {hinter_size, 0}});
        for (const std::uint32_t code : {0x2718u, 1u})
            add(prepare_hinting, {{3, s.t2k}, r9, r10}, sized, name(("hinting the handle's constructor answered " + hex(code) + " (real longjmp unwinds to nothing: answered)").c_str(), s),
                {{hinter_new, code}, {0x10773f30, 0}});
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, sized, name("hinting setup answered with an error", s), {{hinter_new, 0}, {hinter_make, 0}, {hinter_copy, 0}, {hinter_setup, 0x2718}, {0x10773f30, 0}});
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, sized, name("hinting size answered with an error", s), {{hinter_new, 0}, {hinter_make, 0}, {hinter_copy, 0}, {hinter_setup, 0}, {hinter_size, 5}, {0x10773f30, 0}});
        // A kept hint handle with the sizes equal (nothing to do), and with another size; the interpreter's hint font present (its size set).
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, {w32(s.t2k + 0x148, 0x1234), w32(s.t2k + 0x12c, 14), w32(s.t2k + 0x130, 17), w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 17)},
            name("hinting sizes unchanged", s), {{hinter_setup, 0}});
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, {w32(s.t2k + 0x148, 0x1234), w32(s.t2k + 0x12c, 12), w32(s.t2k + 0x130, 17), w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 17)},
            name("hinting x size changed", s), {{hinter_setup, 0}, {hinter_size, 0}});
        add(prepare_hinting, {{3, s.t2k}, r9, r10}, {w32(s.t2k + 0x148, 0x1234), w32(s.t2k + 0x12c, 14), w32(s.t2k + 0x130, 12), w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 17)},
            name("hinting y size changed", s), {{hinter_setup, 0}, {hinter_size, 0}});
        // The font's big glyph count picks the hint mode 2; the hint data kept or not; the T2KG table or the hinter's own.
        for (const std::uint32_t glyphs : {0u, 999u, 1000u, 0x7fffu})
            if (s.t2kg) add(prepare_hinting, {{3, s.t2k}, r9, r10},
                {w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 17), w16(scenario.r32(s.sfnt + 0x10) + 8, glyphs), w32(scenario.r32(s.t2k + 0x144) + 0x38, 0), w32(scenario.r32(s.t2k + 0x144) + 0x14, 0)},
                name(("hinting " + hex(glyphs) + " glyphs").c_str(), s), {{hinter_new, 0}, {hinter_make, 0}, {hinter_copy, 0}, {hinter_setup, 0}, {hinter_size, 0}});
        if (s.t2kg) add(prepare_hinting, {{3, s.t2k}, r9, r10}, {w32(s.t2k + 0xe8, 14), w32(s.t2k + 0xec, 17), w32(scenario.r32(s.t2k + 0x144) + 0x3c, 0x1234), w32(scenario.r32(s.t2k + 0x144) + 0x38, 0)},
            name("hinting a font with a glyph callback", s), {{hinter_new, 0}, {hinter_make, 0}, {hinter_copy, 0}, {hinter_setup, 0}, {hinter_size, 0}});
    }
    // ---- 0x1075f870 and the purge --------------------------------------------------------------------------------------------------
    for (const auto& s : scalers) {
        const auto blocks = blocks_of(scenario, s.mem);
        const auto big = std::ranges::find_if(blocks, [](const Block& b) { return b.size >= 64; });
        if (big == blocks.end()) continue;
        const std::uint32_t mem = s.mem;
        for (const std::int32_t level : {-5, 0, 1, 2, 3, 99, 100, 150}) {
            // The bitmap a live block; slot five's kept buffer; nothing; a block of the object's own at +0xe0; the hinter's handle.
            add(release_glyph, {{3, s.t2k}, {4, std::uint32_t(level)}, r9, r10}, {w32(s.t2k + 0xdc, big->user)}, name(("release level " + std::to_string(level) + " bitmap").c_str(), s));
            add(release_glyph, {{3, s.t2k}, {4, std::uint32_t(level)}, r9, r10}, {w32(s.t2k + 0xdc, big->user), w32(mem + 0x13c, big->user), w32(mem + 0x16c, 0)},
                name(("release level " + std::to_string(level) + " bitmap in slot five").c_str(), s));
            add(release_glyph, {{3, s.t2k}, {4, std::uint32_t(level)}, r9, r10}, {w32(s.t2k + 0xdc, 0)}, name(("release level " + std::to_string(level) + " no bitmap").c_str(), s));
            for (const std::uint32_t grown : {0x100u, 0x101u, 0x10000u})
                add(release_glyph, {{3, s.t2k}, {4, std::uint32_t(level)}, r9, r10}, {w32(s.t2k + 0xdc, 0), w32(mem + 0x13c, big->user), w32(mem + 0x154, grown), w32(mem + 0x16c, 0)},
                    name(("release level " + std::to_string(level) + " slot five grown to " + hex(grown)).c_str(), s));
            add(release_glyph, {{3, s.t2k}, {4, std::uint32_t(level)}, r9, r10}, {w32(s.t2k + 0xdc, 0), w32(s.t2k + 0xe0, big->user)}, name(("release level " + std::to_string(level) + " owned block").c_str(), s));
            add(release_glyph, {{3, s.t2k}, {4, std::uint32_t(level)}, r9, r10}, {w32(s.t2k + 0xdc, 0), w32(s.t2k + 0x148, 0x1234)}, name(("release level " + std::to_string(level) + " hinter handle answered 0").c_str(), s),
                {{hinter_delete, 0}});
        }
        // The purge: the error pointer, the real longjmps (spoiled blocks), the hinter's delete answered with a code (raise_error's real longjmp).
        for (const std::uint32_t level : {0u, 1u, 2u, 100u})
            add(purge, {{3, s.t2k}, {4, level}, {5, error_at}, r9, r10}, {w32(s.t2k + 0xdc, big->user), w32(s.t2k + 0xe0, 0)}, name(("purge level " + std::to_string(level)).c_str(), s));
        add(purge, {{3, s.t2k}, {4, 1}, {5, 0}, r9, r10}, {}, name("purge no error pointer (assertion answered)", s), {{reporter, 0}});
        add(purge, {{3, s.t2k}, {4, 1}, {5, error_at}, r9, r10}, {}, name("purge setjmp answered", s), {{setjmp_entry, 0x2718}});
        add(purge, {{3, s.t2k}, {4, 1}, {5, error_at}, r9, r10}, {w32(s.t2k + 0xdc, big->user), w32(big->user - 0x10, 0)}, name("purge a spoiled bitmap block (real longjmp)", s));
        add(purge, {{3, s.t2k}, {4, 1}, {5, error_at}, r9, r10}, {w32(s.t2k + 0xdc, 0), w32(s.t2k + 0xe0, big->user), w32(big->user - 0x10, 0)}, name("purge a spoiled owned block (real longjmp)", s));
        add(purge, {{3, s.t2k}, {4, 2}, {5, error_at}, r9, r10}, {w32(s.t2k + 0xdc, 0), w32(s.t2k + 0x148, 0x1234)}, name("purge hinter delete answered with a code (real longjmp)", s), {{hinter_delete, 0x2719}});
        add(purge, {{3, s.t2k}, {4, 2}, {5, error_at}, r9, r10}, {w32(s.t2k + 0x148, 0x1234)}, name("purge hinter delete answered 0", s), {{hinter_delete, 0}});
    }
    // ---- 0x10760f50: the delete ----------------------------------------------------------------------------------------------------
    for (const auto& s : scalers) {
        const auto sfnt = scenario.r32(s.t2k + 0x144);
        const auto cache = scenario.r32(s.t2k + 0x18);
        add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {}, name("delete", s));
        add(delete_t2k, {{3, s.t2k}, {4, 0}, r9, r10}, {}, name("delete no error pointer (assertion answered)", s), {{reporter, 0}});
        add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {}, name("delete setjmp answered", s), {{setjmp_entry, 0x2718}});
        add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k, 0x12345)}, name("delete bad stamp (real longjmp)", s));
        add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k + 0x5f4, 0x12345)}, name("delete bad end stamp (real longjmp)", s));
        add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.mem + 0x174, 0)}, name("delete the object's block raises (error stamp spoiled)", s));
        add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k - 0x10, 0)}, name("delete the object's block spoiled (real longjmp)", s));
        const auto data = scenario.r32(sfnt + 0x38);
        if (data != 0) add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(data - 0x10, 0)}, name("delete the hint data block spoiled (real longjmp)", s));
        if (cache != 0) {
            add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(scenario.r32(cache + 0x10) - 0x10, 0)}, name("delete the cache's bits spoiled (real longjmp)", s));
            add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(cache - 0x10, 0)}, name("delete the cache record spoiled (real longjmp)", s));
        }
        const auto blocks = blocks_of(scenario, s.mem);
        const auto big = std::ranges::find_if(blocks, [](const Block& b) { return b.size >= 64; });
        if (big != blocks.end()) {
            add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k + 0x20, big->user), w32(s.t2k + 0x24, 0)}, name("delete with an owned buffer at +0x20", s));
            add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k + 0x24, big->user), w32(big->user - 0x10, 0)}, name("delete with a spoiled buffer at +0x24 (real longjmp)", s));
            add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k + 0x20, big->user), w32(big->user - 0x10, 0)}, name("delete with a spoiled buffer at +0x20 (real longjmp)", s));
            add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k + 0xdc, big->user)}, name("delete with a bitmap (released at level 2)", s));
            add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k + 0x18, 0)}, name("delete without a glyph cache", s));
        }
        add(delete_t2k, {{3, s.t2k}, {4, error_at}, r9, r10}, {w32(s.t2k + 0x148, 0x1234)}, name("delete with a hinter handle answered", s), {{hinter_delete, 0}});
    }
    // ---- the renderer -----------------------------------------------------------------------------------------------------------------
    // The game's call: cmd 2, characters 0x20..0xff; the error pointer a stack word. Then every other command, glyph numbers, the shifts, the
    // matrix and the sizes of the object poked, the errors.
    const auto render = [&](const Scaler& s, std::uint32_t character, std::uint32_t shift_x, std::uint32_t shift_y, std::uint32_t quality, std::uint32_t cmd, std::vector<Write> memory,
                            const std::string& label, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
        add(render_character, {{3, s.t2k}, {4, character}, {5, shift_x}, {6, shift_y}, {7, quality}, {8, cmd}, {9, error_at}, r10}, with(std::move(memory), hint_data(s)), label, std::move(answers));
    };
    const std::vector<std::uint32_t> characters = {'A', 'g', '0', '9', 0xe9, 'i', ' ', '%', 0xc4, 0xe7, '~', 0xff, 0xa9, 0xbf};
    for (const auto& s : scalers) {
        for (std::uint32_t character = 0x20; character < 0x100; ++character)
            if (exhaustive || character % 9 == 0 || character == 0x41 || character == 0x67 || character == 0xe9 || character == 0xff) render(s, character, 0, 0, 5, 2, {}, name(("render game call character " + hex(character)).c_str(), s));
        for (const std::uint32_t character : {0u, 1u, 0x1fu, 0x100u, 0x2022u, 0x20acu, 0xf000u, 0xffffu, 0x10041u, 0xffffffffu})
            render(s, character, 0, 0, 5, 2, {}, name(("render game call character " + hex(character)).c_str(), s));
    }
    for (std::size_t k = 0; k < scalers.size(); ++k) {
        // Every command bit combination on a few characters for the first scaler, a fifth of them (spread over the bits) on the others.
        for (std::uint32_t cmd = 0; cmd < 0x400; ++cmd)
            if (exhaustive ? (k == 0 || (cmd * 7 + k) % 5 == 0) : (k == 0 ? cmd % 4 == 0 : (cmd * 7 + k) % 17 == 0))
                render(scalers[k], (cmd % 3 == 0) ? 'A' : (cmd % 3 == 1) ? 'g' : '8', 0xfe, 3, 5, cmd, {}, name(("render cmd " + hex(cmd)).c_str(), scalers[k]));
    }
    for (const auto& s : scalers) {
        // Glyph numbers (cmd bit 8), the shifts (bit 0x10), qualities, the errors.
        for (std::uint32_t number : {0u, 1u, 36u, 50u, 0x67u, 200u, 0xffu, 0x3ffu, 0x800u, 0xffffu, 0x10001u})
            for (const std::uint32_t cmd : {0xau, 0xeu, 0x1au, 0x1eu, 0xbu, 0x20au})
                render(s, number, 0x7f, 0x81, 5, cmd, {}, name(("render glyph " + hex(number) + " cmd " + hex(cmd)).c_str(), s));
        for (const std::uint32_t quality : {0u, 1u, 4u, 5u, 6u, 255u, 0x105u})
            render(s, 'A', 0, 0, quality, 2, {}, name(("render quality " + hex(quality)).c_str(), s));
        for (const std::uint32_t shift : {0u, 1u, 0x7fu, 0x80u, 0xffu, 0x100u, 0xffffffffu, 0x80000000u}) {
            render(s, 'A', shift, 0, 5, 0x12, {}, name(("render shift x " + hex(shift)).c_str(), s));
            render(s, 'g', 0, shift, 5, 0x12, {}, name(("render shift y " + hex(shift)).c_str(), s));
        }
        render(s, 'A', 0, 0, 5, 2, {}, name("render no error pointer (assertion answered)", s), {{reporter, 0}});
        add(render_character, {{3, s.t2k}, {4, 'A'}, {5, 0}, {6, 0}, {7, 5}, {8, 2}, {9, 0}, r10}, {}, name("render no error pointer (assertion answered)", s), {{reporter, 0}});
        render(s, 'A', 0, 0, 5, 2, {}, name("render setjmp answered", s), {{setjmp_entry, 0x2718}});
        render(s, 'A', 0, 0, 5, 2, {w32(s.mem + 0x174, 0x1234)}, name("render error stamp spoiled (real longjmp)", s));
        render(s, 'A', 0, 0, 5, 2, {w32(s.mem + 0x174, 0x1234)}, name("render error stamp spoiled (raise answered)", s), {{0x10773f30, 0}});
        render(s, 'A', 0, 0, 5, 0x1c1, {}, name("render hint bit with the grid bit and the 0x40 bit (assertion answered)", s), {{reporter, 0}});
        render(s, 'A', 0, 0, 5, 0x42, {}, name("render the 0x40 bit with the scan bit (assertion answered)", s), {{reporter, 0}});
        for (const std::uint32_t cmd : {0x102u, 0x106u, 0x112u, 0x116u}) render(s, 'A', 0, 0, 5, cmd, {}, name(("render colour table cmd " + hex(cmd)).c_str(), s));
        // The object in other states: the matrix not the identity, other sizes, other limits.
        const auto matrix_state = [&](const Matrix& matrix) {
            std::vector<Write> memory{w32(s.t2k + 8, matrix.m[0]), w32(s.t2k + 0xc, matrix.m[1]), w32(s.t2k + 0x10, matrix.m[2]), w32(s.t2k + 0x14, matrix.m[3]), w32(s.t2k + 0x1c, 0)};
            return memory;
        };
        for (const auto& matrix : {matrices[1], matrices[3], matrices[4], matrices[5], matrices[6], matrices[7]})
            for (const std::uint32_t cmd : {2u, 0x12u, 0x3u, 0xau})
                for (const std::uint32_t character : {0x41u, 0x67u})
                    render(s, character, 0xfe, 3, 5, cmd, matrix_state(matrix), name((std::string("render cmd ") + hex(cmd) + " character " + hex(character) + " matrix " + matrix.label).c_str(), s));
        for (const std::uint32_t size : {1u, 3u, 9u, 24u, 25u, 80u, 0xffu, 0x10000u})
            for (const std::uint32_t cmd : {2u, 0x3u, 0x12u, 0x6u})
                for (const std::uint32_t character : {0x41u, 0x57u})
                    render(s, character, 0, 0, 5, cmd, {w32(s.t2k + 0xe8, size), w32(s.t2k + 0xec, size)}, name(("render cmd " + hex(cmd) + " size " + hex(size) + " character " + hex(character)).c_str(), s));
        // The cache of rendered bitmaps: stale bits set (the bitmap not made again), the 0x134 byte and the 0x12c size the gray mode tests.
        const auto cache = scenario.r32(s.t2k + 0x18);
        for (const std::uint32_t character : {0x41u, 0x42u, 0x69u})
            for (const std::uint32_t value : {0u, 0xffu})
                if (cache != 0) render(s, character, 0, 0, 0, 2, {w32(s.t2k + 0x1c, 1), w8(scenario.r32(cache + 0x10) + character / 8, value), w8(scenario.r32(cache + 0x14) + character / 8, value)},
                                       name(("render cached bits " + hex(value) + " character " + hex(character)).c_str(), s));
        // Different x and y sizes under a composite's offsets (the offsets scale by the size of their axis).
        for (const auto& [x, y] : {std::pair{13u, 25u}, {24u, 6u}})
            for (const std::uint32_t character : {0xe9u, 0x67u})
                render(s, character, 0, 0, 5, 2, {w32(s.t2k + 0xe8, x), w32(s.t2k + 0xec, y)}, name(("render composite sizes " + hex(x) + " and " + hex(y) + " character " + hex(character)).c_str(), s));
        // The units per em the offsets of a composite's components are scaled by (the object's halfword at +0x140): odd, even, zero (the division faults).
        for (const std::uint32_t upm : {0x803u, 0x7ffu, 0x3e8u, 0x1000u, 0x1u, 0u})
            for (const std::uint32_t character : {0xe9u, 0x67u, 0xc4u})
                render(s, character, 0, 0, 5, 2, {w16(s.t2k + 0x140, upm)}, name(("render composite units per em " + hex(upm) + " character " + hex(character)).c_str(), s));
        // Every glyph already in the cache of rendered bitmaps (the two tables of bits all set): the scan converter's flags are cleared.
        if (cache != 0) {
            std::vector<Write> full{w32(s.t2k + 0x1c, 1)};
            for (std::uint32_t n = 0; n < scenario.r32(cache + 0xc) / 2; ++n) {
                full.push_back(w8(scenario.r32(cache + 0x10) + n, 0xff));
                full.push_back(w8(scenario.r32(cache + 0x14) + n, 0xff));
            }
            for (const std::uint32_t character : {0x41u, 0x67u, 0xe9u})
                render(s, character, 0, 0, 0, 2, full, name(("render with every glyph cached character " + hex(character)).c_str(), s));
        }
        for (const std::uint32_t grayscale : {0u, 1u, 0xffu, 0x80u})
            for (const std::uint32_t size : {0x10u, 0x18u, 0x19u, 0u})
                render(s, 'A', 0, 0, 5, 3, {w8(s.t2k + 0x134, grayscale), w32(s.t2k + 0x12c, size)}, name(("render grayscale byte " + hex(grayscale) + " hint size " + hex(size)).c_str(), s));
    }
    // Direct entries of the renderer: a component's depth, the quality byte, and an identity that is not (the size limit of the cached bitmaps).
    for (const auto& s : scalers) {
        for (const std::uint32_t depth : {1u, 3u})
            for (const std::uint32_t cmd : {0xcu, 0x1cu, 0xdu})
                for (const std::uint32_t character : {0x41u, 0x67u, 0xe9u})
                    add(render_glyph, {{3, s.t2k}, {4, character}, {5, depth}, {6, 0}, {7, 0}, {8, 0}, {9, cmd}, r10}, with(dead_jump_buffer(s), hint_data(s)), name(("render glyph depth " + std::to_string(depth) + " cmd " + hex(cmd) + " character " + hex(character)).c_str(), s));
        for (const std::uint32_t cmd : {0xcu, 0x8u, 0x1cu, 0xeu, 0x4u, 0x2u})
            for (const std::uint32_t index : {0u, 1u, 36u, 37u, 200u, 0xffffu})
                add(render_glyph, {{3, s.t2k}, {4, index}, {5, 0}, {6, 1}, {7, 2}, {8, 3}, {9, cmd}, r10}, with(dead_jump_buffer(s), hint_data(s)), name(("render glyph direct cmd " + hex(cmd) + " index " + hex(index)).c_str(), s));
    }
    // The hinted commands (dormant for the game): the hinter's callees answered so that the path goes on, then the real ones.
    for (const auto& s : scalers) {
        const auto sfnt = scenario.r32(s.t2k + 0x144);
        const auto head = scenario.r32(sfnt + 0xc);
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> answered{{hinter_new, 0}, {hinter_make, 0}, {hinter_copy, 0}, {hinter_setup, 0}, {hinter_size, 0}, {hinter_glyph, 0}};
        // A head the hinted path accepts: another glyph data format, a smallest size under the object's.
        const auto accepted = [&](std::int16_t format, std::int16_t smallest, std::vector<Write> memory = {}) {
            memory.push_back(w16(head + 0x38, std::uint16_t(format)));
            memory.push_back(w16(head + 0x32, std::uint16_t(smallest)));
            return memory;
        };
        for (const std::uint32_t cmd : {0x80u, 0x83u, 0x8au, 0x282u})
            for (const std::uint32_t character : {0x41u, 0xe9u})
                for (const std::int16_t smallest : {0, 100})
                    for (const std::uint32_t hint_mode : {1u, 2u})
                        render(s, character, 0xfe, 3, 5, cmd, accepted(0, smallest, {w32(s.t2k + 0x138, hint_mode), w32(s.t2k + 0x148, 0x1234)}),
                               name(("render hinted cmd " + hex(cmd) + " character " + hex(character) + " smallest " + std::to_string(smallest) + " mode " + std::to_string(hint_mode)).c_str(), s), answered);
        for (const std::uint32_t code : {0xffffff9cu, 0x2718u})
            for (const std::uint32_t cmd : {0x81u, 0x83u, 0x281u, 0x201u})
                for (const std::uint32_t character : {0x41u, 0xe9u})
                    render(s, character, 0, 0, 5, cmd, accepted(0, 0, {w32(s.t2k + 0x138, 2), w32(s.t2k + 0x148, 0x1234)}), name(("render hinter answered " + hex(code) + " cmd " + hex(cmd) + " character " + hex(character)).c_str(), s),
                           {{hinter_new, 0}, {hinter_make, 0}, {hinter_copy, 0}, {hinter_setup, 0}, {hinter_size, 0}, {hinter_glyph, code}, {0x10773f30, 0}});
        render(s, 'A', 0, 0, 5, 0x81, accepted(0, 0, {w32(s.t2k + 0x138, 2)}), name("render hinted with the real hinter", s));
        render(s, 'A', 0, 0, 5, 0x83, accepted(0, 0, {w32(s.t2k + 0x138, 2)}), name("render hinted with the real hinter, grid", s));
        render(s, 'A', 0, 0, 5, 0x82, accepted(0, 0), name("render hinted scan with the real hinter", s));
        // A T2K font: the hinted bit is turned into the grid bit (the head's format 2000 or 2001), or a font without head.
        for (const std::int16_t format : {0x7d0, 0x7d1})
            render(s, 'A', 0, 0, 5, 0x82, accepted(format, 0), name(("render hint bit on a font of format " + hex(std::uint32_t(format))).c_str(), s));
        render(s, 'A', 0, 0, 5, 0x82, {w32(sfnt + 0xc, 0)}, name("render hint bit on a font without head", s));
        render(s, 'A', 0, 0, 5, 0x2, {w32(sfnt + 0xc, 0)}, name("render without head", s));
    }
    // Paths the other cases cannot reach alone. (1) The scan converter's two assertions (the glyph's word at +0xc positive: set after the hinter ran, through a
    // poke on its dispatch, since the glyph is made during the call; the glyph record of a plain character is the allocator's slot 0 buffer). (2) The hint data
    // made by the hinter's own routine (a font without T2KG) and the allocation of its block failing (a real longjmp through the transformation's setjmp), so the
    // routine's call is paired without the copy of the stack bytes it leaves unwritten (the original copies leftovers of the stack: not reproducible).
    for (const auto& s : scalers) {
        const auto slot0 = scenario.r32(s.mem + 0x128);
        Case& item = add(render_character, {{3, s.t2k}, {4, 0x41}, {5, 0}, {6, 0}, {7, 5}, {8, 3}, {9, error_at}, r10}, hint_data(s), name("render with a glyph word at +0xc (assertions answered)", s),
                         {{reporter, 0}});
        item.pokes = {{hinter_glyph, {w32(slot0 + 0xc, 1)}}};
        for (const bool grid_only : {true, false}) {
            Case& other = add(render_character, {{3, s.t2k}, {4, 0x67}, {5, 0}, {6, 0}, {7, 5}, {8, grid_only ? 3u : 0x203u}, {9, error_at}, r10}, hint_data(s),
                              name("render a composite with a glyph word at +0xc (assertions answered)", s), {{reporter, 0}});
            other.pokes = {{hinter_glyph, {w32(slot0 + 0xc, 1)}}};
        }
        for (const std::uint32_t k : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 8u, 10u}) {
            if (!s.t2kg && k != 0) continue;  // the font without T2KG: only the allocation of the data block failing leaves the unwritten stack bytes uncopied
            auto memory = matrix_memory(matrices[0]);
            memory.push_back(w32(s.mem + 4, scenario.r32(s.mem + 8) - k));
            memory.push_back(w32(s.sfnt + 0x38, 0));
            memory.push_back(w32(s.t2k + 0x148, 0));
            add(set_transformation, {{3, s.t2k}, {4, 1}, {5, 14}, {6, 14}, {7, matrix_at}, {8, 0}, {9, error_at}, r10}, memory,
                name(("transform with the hinter, table full after " + std::to_string(k) + " allocations (real longjmp)").c_str(), s),
                {{hinter_new, 0}, {hinter_setup, 0}, {hinter_size, 0}});
        }
    }
    // The error path of each API entry ends in the memory object's release (free_blocks 0x10767820), called at the entry's own frame: its prologue stores the link
    // register (the address after the entry's `bl`) at the entry's sp + 8, the last store there. The cases that take that path (the setjmp answered with a code) check the
    // word on both sides against the listing's address: the entry's frame size and the return address of its `bl 0x10767820`.
    {
        struct Path { std::uint32_t entry, frame, back; };
        constexpr Path paths[] = {{new_t2k, 0x50, 0x1075eaec}, {set_transformation, 0xa0, 0x1075f854}, {purge, 0x50, 0x1075f9ec}, {render_character, 0x60, 0x10760f30}, {delete_t2k, 0x40, 0x1076104c}};
        for (auto it = cases.begin() + std::ptrdiff_t(first_case); it != cases.end(); ++it)
            if (it->label.find("setjmp answered") != std::string::npos)
                for (const auto& path : paths)
                    if (path.entry == it->entry) {
                        const auto word = scenario.sp() - path.frame + 8, back = path.back;
                        it->verify = [word, back](State& state) { return std::uint32_t(state.memory.read<4>(word)) == back; };
                    }
    }
    // The rare cases first: the call trace pairs only the first 32 cases of an entry and every 64th after.
    const auto priority = [](const Case& item) {
        for (const char* word : {"with the hinter, table full", "glyph word at +0xc", "no error pointer", "setjmp answered"})
            if (item.label.find(word) != std::string::npos) return -1;
        for (const char* word : {"real longjmp", "answered", "hinter", "hinted", "composite", "depth", "no error pointer", "colour table", "glyph cache", "vhea"})
            if (item.label.find(word) != std::string::npos) return 0;
        for (const char* word : {"setjmp", "stamp", "table full", "spoiled", "quality", "shift", "size", "matrix", "units", "caret"})
            if (item.label.find(word) != std::string::npos) return 1;
        return 2;
    };
    std::stable_sort(cases.begin() + std::ptrdiff_t(first_case), cases.end(), [&](const Case& a, const Case& b) { return priority(a) < priority(b); });
    for (auto it = cases.begin() + std::ptrdiff_t(first_case); it != cases.end(); ++it) it->label = "t2k_core: " + it->label;
}
}
