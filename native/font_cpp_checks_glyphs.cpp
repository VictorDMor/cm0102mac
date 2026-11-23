// FONT.CPP, block `glyphs` (0x10750580 the glyph cache, 0x107500f0 the string width): cases from the save-info snapshot's font records
// (slots 0, 1 and 3 hold loaded fonts whose caches hold 2, 58 and 45 glyph records; slot 2 a font with no scaler; slots 4 to 7 are empty
// records). The engine calls below (render, purge) and the allocator's are the real ones on both sides: the rewrite calls them through the
// dispatcher, so the answers, nth answers and pokes of a case reach both sides alike.
//
// 0x10750580: a hit at every position of every real cache (the three key parts, and the style bits above the five the key keeps); misses
// that grow the cache (plain and, with style bit 0x10, smoothed, over characters of every size of bitmap: none for the space, wide ones)
// and in a full cache (256 records, built in the scratch area: the oldest with a bitmap to free, the oldest with none, a hit at the last
// position); the allocation failures (realloc, new, the bitmap's malloc in both copies) and the engine's errors (the scaler's stamp spoiled
// before the render, or by a poke at the callee that runs just before the purge); the null font record (the assertion, with page 0 mapped).
// 0x107500f0: strings of every length (empty, one character, the last character's left and width term, long ones that wrap the halfword),
// styles, sizes, an unloaded record, a null record and a null string. Labels start with "glyphs:".
#include "font_cpp_checks.hpp"
#include "font_cpp_detail.hpp"
#include <cstdlib>
#include <random>
#include <string>

namespace cm::aot::font_cpp_checks {
namespace {
using namespace cm::aot::font_cpp;
constexpr std::uint32_t glyph_entry = 0x10750580, width_entry = 0x107500f0;
constexpr std::uint32_t resize_block = 0x107751b0, new_object = 0x10773520, allocate_block = 0x107735f0, byte_move = 0x10739ef0, game_malloc = 0x10775130;
constexpr int font_array_slot = -0x3bd8;
constexpr std::uint32_t bitmap_at = scratch_base + 0x70000, array_at = scratch_base + 0x40000, records_at = scratch_base + 0x41000, text_at = scratch_base + 0x50000, slot_at = scratch_base + 0x60;
}

FONT_CPP_CASES("glyphs") {
    const auto array = scenario.toc(font_array_slot);
    struct Slot { std::uint32_t slot, record; };
    std::vector<Slot> loaded, partial, empty;
    for (unsigned k = 0; k < 8; ++k) {
        const Slot slot{array + 4 * k, scenario.r32(array + 4 * k)};
        if (!slot.record || !scenario.u8(slot.record)) empty.push_back(slot);
        else if (scenario.r32(slot.record + font_record::scaler) && scenario.r32(slot.record + font_record::sfnt)) loaded.push_back(slot);
        else partial.push_back(slot);
    }
    if (loaded.size() < 2 || partial.empty() || empty.empty()) return;
    const auto real = loaded[0];
    const auto scaler = scenario.r32(real.record + font_record::scaler);
    // The memory object's stamp word (TSIMEM.C): the engine's calls raise an error when it is not the initial value.
    const auto stamp = scenario.r32(scaler + 4) + 0x174;

    const auto glyph = [&](const std::string& what, std::uint32_t slot, std::uint32_t style, std::uint32_t size, std::uint32_t character, std::vector<Write> memory = {}) {
        Case item{.entry = glyph_entry, .registers = {{3, slot}, {4, style}, {5, size}, {6, character}}, .memory = std::move(memory), .label = "glyphs: " + what};
        return item;
    };
    const auto width = [&](const std::string& what, std::uint32_t slot, std::uint32_t style, std::uint32_t size, std::uint32_t text, std::vector<Write> memory = {}) {
        Case item{.entry = width_entry, .registers = {{3, slot}, {4, style}, {5, size}, {6, text}}, .memory = std::move(memory), .label = "glyphs: width " + what};
        return item;
    };
    // A glyph record's key.
    struct Key { std::uint32_t flags, size, character; };
    const auto key_of = [&](std::uint32_t record) { return Key{scenario.u16(record + glyph_cache::flags), scenario.u8(record + glyph_cache::size), scenario.u8(record + glyph_cache::character)}; };
    const auto cache_of = [&](const Slot& slot) { return scenario.r32(slot.record + font_record::cache); };
    const auto count_of = [&](const Slot& slot) { return scenario.r32(slot.record + font_record::cache_count); };
    const auto real_key = key_of(scenario.r32(cache_of(real)));
    if (std::getenv("CM_FONT_CPP_SHOW_STATE"))
        for (std::uint32_t k = 0; k < count_of(real); ++k) {
            const auto r = scenario.r32(cache_of(real) + 4 * k);
            std::fprintf(stderr, "glyphs state: slot0 cache %u: char %02x size %u flags %x width %u height %u stride %u\n", k, scenario.u8(r + 0x1f), scenario.u8(r + 0x1e), scenario.u16(r + 0x1c),
                         scenario.r32(r + 8), scenario.r32(r + 0x10), scenario.r32(r + 0xc));
        }
    // A cache of 256 records in the scratch area: an array of pointers to records of the real cache (or to scratch records with no bitmap) whose key is not
    // the one a miss asks for, on the real record of slot 0.
    const auto full_cache = [&](bool bitmap_at_end, std::uint32_t hit_character, bool hit_last) {
        std::vector<Write> memory{w32(real.record + font_record::cache, array_at), w32(real.record + font_record::cache_count, 256)};
        const auto first = scenario.r32(cache_of(real));
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t record = first;
            if (n == 255 || n == 254) {
                // Records of the scratch area: key {flags 0, size 1, character n}, the bitmap null (or the real record's pointer).
                record = records_at + (n - 254) * 0x20;
                memory.push_back(w32(record + glyph_cache::left, 1));
                memory.push_back(w32(record + glyph_cache::bitmap, bitmap_at_end ? scenario.r32(first + glyph_cache::bitmap) : 0));
                memory.push_back(w16(record + glyph_cache::flags, 0));
                memory.push_back(w8(record + glyph_cache::size, 1));
                memory.push_back(w8(record + glyph_cache::character, n == 255 && hit_last ? hit_character : 0x11 + n));
            }
            memory.push_back(w32(array_at + 4 * n, record));
        }
        return memory;
    };
    // The glyph record to ask for that is not in any real cache.
    const auto fresh_character = [&](const Slot& slot, std::uint32_t style, std::uint32_t size) {
        for (std::uint32_t character = 0x41; character < 0x100; ++character) {
            bool found = false;
            for (std::uint32_t k = 0; k < count_of(slot) && !found; ++k) {
                const auto record = scenario.r32(cache_of(slot) + 4 * k);
                const auto key = key_of(record);
                found = key.character == character && key.size == (size & 0xff) && (key.flags & 0x1f) == (style & 0x1f);
            }
            if (!found) return character;
        }
        return 0x41u;
    };
    const std::uint32_t miss_size = 13;
    const auto miss_character = fresh_character(real, 0, miss_size);

    // ---- Rare sites first (the call trace pairs only the first 32 cases of an entry): every call site of the miss path once.
    cases.push_back(glyph("hit at the front", real.slot, real_key.flags, real_key.size, real_key.character));
    // The cached record carries style bits above the key's five (0x20, 0x200): they must not keep it from matching.
    cases.push_back(glyph("hit, cached record with stray style bits", real.slot, real_key.flags & 0x1f, real_key.size, real_key.character,
                          {w16(scenario.r32(cache_of(real)) + glyph_cache::flags, real_key.flags | 0x3e0)}));
    cases.push_back(glyph("miss, plain", real.slot, 0, miss_size, miss_character));
    cases.push_back(glyph("miss, smoothed", real.slot, 0x10, miss_size, miss_character));
    cases.push_back(glyph("null font record (page 0 mapped)", slot_at, 0, 9, 'A', {w32(slot_at, 0)}));
    {
        auto item = glyph("miss, full cache, oldest has a bitmap", real.slot, 0, miss_size, miss_character, full_cache(true, 0, false));
        cases.push_back(item);
        cases.push_back(glyph("miss, full cache, oldest has no bitmap", real.slot, 0, miss_size, miss_character, full_cache(false, 0, false)));
        cases.push_back(glyph("miss, full cache, smoothed", real.slot, 0x10, miss_size, miss_character, full_cache(true, 0, false)));
        cases.push_back(glyph("hit at the last of 256", real.slot, 0, 1, 0x7d, full_cache(false, 0x7d, true)));
    }
    for (const auto style : {0u, 0x10u}) {
        const std::string kind = style ? "smoothed" : "plain";
        auto item = glyph("realloc fails, " + kind, real.slot, style, miss_size, miss_character);
        item.nth_answers = {{resize_block, 1, 0}};
        item.original_owners = {resize_block};
        cases.push_back(item);
        item = glyph("new fails, " + kind, real.slot, style, miss_size, miss_character);
        item.nth_answers = {{new_object, 1, 0}};
        item.original_owners = {new_object};
        cases.push_back(item);
        item = glyph("bitmap malloc fails, " + kind, real.slot, style, miss_size, miss_character);
        item.nth_answers = {{allocate_block, 1, 0}};
        item.original_owners = {allocate_block};
        cases.push_back(item);
        // The memory object's stamp spoiled before the render: its error and the purge's.
        cases.push_back(glyph("memory stamp spoiled, " + kind, real.slot, style, miss_size, miss_character, {w32(stamp, 0)}));
        // ... or the block the renderer owns replaced by an address the allocator never made, by the callee that runs last before the purge: the
        // purge's release raises.
        item = glyph("purge error alone, " + kind, real.slot, style, miss_size, miss_character);
        item.pokes = {{style ? allocate_block : byte_move, {w32(scaler + 0xe0, scratch_base + 0x800)}}};
        cases.push_back(item);
        // An allocation of the engine's failing (the nth of the game's malloc).
        for (const unsigned nth : {1u, 2u, 3u, 4u}) {
            item = glyph("render malloc " + std::to_string(nth) + " fails, " + kind, real.slot, style, miss_size, miss_character);
            item.nth_answers = {{game_malloc, nth, 0}};
            cases.push_back(item);
        }
    }
    cases.push_back(glyph("hit in the middle", loaded[1].slot, key_of(scenario.r32(cache_of(loaded[1]) + 4 * (count_of(loaded[1]) / 2))).flags,
                          key_of(scenario.r32(cache_of(loaded[1]) + 4 * (count_of(loaded[1]) / 2))).size,
                          key_of(scenario.r32(cache_of(loaded[1]) + 4 * (count_of(loaded[1]) / 2))).character));

    const auto text = [&](const std::string& value, std::uint32_t at = text_at) {
        std::vector<Write> memory;
        for (std::size_t n = 0; n <= value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), n < value.size() ? std::uint8_t(value[n]) : 0));
        return memory;
    };
    // ---- Synthetic pixels: the engine's bitmap pointer is replaced, by a poke at the bitmap's malloc (the last dispatched call before the copy or the smoothing
    // reads the pixels), with one in the scratch area: uniform (a 5x5 sum that is a multiple of 25: the quotient's exact cases), saturated, alternating, random.
    // The sizes come from the real render of the character.
    {
        std::mt19937 random(0x61f7);
        // Characters the second font really renders with a bitmap of at least 7x7 (the render of the others is empty or thin), and its scaler.
        std::vector<std::uint32_t> characters;
        for (std::uint32_t k = 0; k < count_of(loaded[1]) && characters.size() < 5; ++k) {
            const auto r = scenario.r32(cache_of(loaded[1]) + 4 * k);
            if (scenario.r32(r + glyph_cache::width) >= 7 && scenario.r32(r + glyph_cache::height) >= 7) characters.push_back(scenario.u8(r + glyph_cache::character));
        }
        characters.push_back(0x20);
        const auto second_scaler = scenario.r32(loaded[1].record + font_record::scaler);
        if (std::getenv("CM_FONT_CPP_SHOW_STATE")) std::fprintf(stderr, "glyphs state: %zu characters with a bitmap\n", characters.size());
        for (const int pattern : {1, 2, 3, 4, 5, 6})
            for (const std::uint32_t character : characters) {
                std::vector<Write> memory;
                for (std::uint32_t k = 0; k < 0x1800; ++k) {
                    std::uint32_t value = 0;
                    switch (pattern) {
                        case 1: value = 3; break;                          // uniform: every interior sum is a multiple of 25
                        case 2: value = random() % 256; break;
                        case 3: value = 255; break;
                        case 4: value = (k & 1) ? 255 : 0; break;
                        case 5: value = 1 + k % 3; break;
                        default: value = 40;
                    }
                    memory.push_back(w8(bitmap_at + k, value));
                }
                for (const auto smooth : {0u, 0x10u}) {
                    auto item = glyph("synthetic pixels " + std::to_string(pattern) + " character " + hex(character) + (smooth ? " smoothed" : " plain"), loaded[1].slot, smooth, 20, character, memory);
                    item.pokes = {{allocate_block, {w32(second_scaler + scaler::bitmap, bitmap_at)}}};
                    cases.push_back(item);
                }
            }
    }
    // The width's rounding: a cached record with chosen advance, left and width (the string is its character twice: the first adds the advance, the
    // last the left bearing and width term), the sum's low half on each side of 0x8000.
    for (const std::uint32_t advance : {0x7fffu, 0x8000u, 0x8001u, 0xffffu, 0x10000u, 0x18000u, 0x17fffu, 0x8000u + 0x30000u, 0xfff00000u, 0x7fff0000u})
        for (const std::uint32_t term : {0u, 1u, 0x8000u / 0x400})
            cases.push_back(width("rounding, advance " + hex(advance) + " left " + std::to_string(term), real.slot, real_key.flags, real_key.size, text_at,
                                  [&] {
                                      auto memory = text(std::string(2, char(real_key.character)));
                                      const auto record = scenario.r32(cache_of(real));
                                      memory.push_back(w32(record + glyph_cache::advance, advance));
                                      memory.push_back(w32(record + glyph_cache::left, term));
                                      memory.push_back(w32(record + glyph_cache::width, 0));
                                      return memory;
                                  }()));

    // ---- Hits: every position of every real cache, and the near-misses of each key part.
    for (const auto& slot : loaded) {
        for (std::uint32_t k = 0; k < count_of(slot); ++k) {
            const auto key = key_of(scenario.r32(cache_of(slot) + 4 * k));
            cases.push_back(glyph("hit " + std::to_string(k) + " of " + std::to_string(count_of(slot)) + " slot " + hex(slot.slot), slot.slot, key.flags, key.size, key.character));
        }
        const auto key = key_of(scenario.r32(cache_of(slot) + 4 * (count_of(slot) - 1)));
        // The style's bits above the key's five, the registers' high bits.
        cases.push_back(glyph("hit with high style bits", slot.slot, key.flags | 0x3e0, key.size, key.character));
        cases.push_back(glyph("hit with wide registers", slot.slot, 0xabcd0000u | key.flags, 0x7700u | key.size, 0x1200u | key.character));
        // Near misses: each part of the key changed.
        cases.push_back(glyph("miss, other character", slot.slot, key.flags, key.size, key.character ^ 1));
        cases.push_back(glyph("miss, other size", slot.slot, key.flags, key.size + 1, key.character));
        cases.push_back(glyph("miss, other style bit", slot.slot, key.flags ^ 1, key.size, key.character));
        cases.push_back(glyph("miss, other smoothing bit", slot.slot, key.flags ^ 0x10, key.size, key.character));
    }
    // ---- Misses that grow the cache: characters of every kind, sizes and the style's key bits.
    for (const auto& slot : {loaded[0], loaded[1], loaded.back()})
        for (const std::uint32_t style : {0u, 1u, 2u, 4u, 8u, 0x10u, 0x15u, 0x200u, 0x214u})
            for (const std::uint32_t character : {0x20u, 0x41u, 0x57u, 0x69u, 0x7cu, 0x80u, 0xffu, 0x00u, 0x2eu})
                cases.push_back(glyph("miss style " + hex(style) + " character " + hex(character) + " slot " + hex(slot.slot), slot.slot, style,
                                      scenario.u8(slot.record + font_record::size) + (character & 3), character));
    cases.push_back(glyph("miss, empty cache", real.slot, 0, miss_size, miss_character, {w32(real.record + font_record::cache_count, 0)}));
    cases.push_back(glyph("miss, empty cache, smoothed", real.slot, 0x10, miss_size, 0x20, {w32(real.record + font_record::cache_count, 0)}));
    for (const auto& slot : partial) cases.push_back(glyph("miss, slot with no scaler", slot.slot, 0, 9, 'A'));
    for (const auto& slot : empty) cases.push_back(glyph("miss, empty record", slot.slot, 0, 9, 'A'));

    // ---- The string width.
    cases.push_back(width("null font record (page 0 mapped)", slot_at, 0, 9, text_at, [&] { auto m = text("Hi"); m.push_back(w32(slot_at, 0)); return m; }()));
    cases.push_back(width("null string (page 0 mapped)", real.slot, 0, real_key.size, 0));
    cases.push_back(width("unloaded record", empty[0].slot, 0, 9, text_at, text("Hello")));
    for (const auto& slot : loaded) {
        const auto size = scenario.u8(slot.record + font_record::size);
        for (const std::string value : {"", "A", "Hello, World", "Zz9 |", "i", "W", "   ", "Championship Manager 01/02", "\x80\xff\x01\x7f", "ab\x80"})
            cases.push_back(width("\"" + value + "\" slot " + hex(slot.slot), slot.slot, 0x200, size, text_at, text(value)));
        cases.push_back(width("smoothed", slot.slot, 0x210, size, text_at, text("Hello, World")));
        cases.push_back(width("style 4", slot.slot, 4, size + 2, text_at, text("Hello, World")));
        cases.push_back(width("style 1", slot.slot, 1, size, text_at, text("Hello")));
        cases.push_back(width("wide registers", slot.slot, 0x12340200u, 0x5500u | size, text_at, text("Hello")));
    }
    // Long strings: the sum wraps the halfword result (a pixel is 0x10000 in the sum).
    cases.push_back(width("2400 characters", real.slot, 0x200, real_key.size, text_at, text(std::string(2400, 'W'))));
    cases.push_back(width("5000 characters", real.slot, 0x200, real_key.size, text_at, text(std::string(5000, 'M'))));
    cases.push_back(width("cache misses while measuring", real.slot, 0x14, 13, text_at, text("The quick brown fox jumps over the lazy dog 0123456789")));
    cases.push_back(width("slot with no scaler", partial[0].slot, 0, 9, text_at, text("Hello")));
}
}
