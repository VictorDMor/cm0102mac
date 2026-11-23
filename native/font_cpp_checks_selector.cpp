// FONT.CPP, block `selector` (0x10750320): cases for the style selector, from the save-info snapshot's font records (the game's eight
// Font slots at TOC -0x3bd8: slots 0, 1 and 3 hold loaded fonts with an sfnt and a scaler of the Traditional style at three sizes, slot 2 a
// font whose sfnt and scaler are not made yet, the others are empty records). Every case runs the original and the C++ from the same
// state, so the engine calls below the selector (new_sfnt, new_t2k, the transformation, the deletes) are the real ones.
//
// The branches: the early exit (style bits 0-3 and size equal to the record's); the transformation only (bits 0 and 1 equal, bits 2 and
// 3 or the size differ: the skew angle by bit 2, then bit 3, else the third constant; size 0..255); the recreation (bits 0 and 1 differ,
// or no scaler: the old scaler and sfnt deleted when present, the callbacks record for bit 0 or bit 1, new_sfnt, new_t2k, the ids, the
// size byte reset). The error words are reached through a corrupted font stream and through failed allocations (nth answers of the game's
// malloc, which both sides see when the engine calls it; a poke on a routine the C++ calls typed would fire on one side only). The null record
// and the unloaded slots need the null page.
// Floating point: the constants (angles, the skew factor and the integer conversion's magic number) are poked to the thresholds the
// conversion has: zero, negative, huge, infinite and NaN. Labels start with "selector:".
#include "font_cpp_checks.hpp"
#include "font_cpp_detail.hpp"
#include <cstdlib>
#include <cstring>
#include <random>

namespace cm::aot::font_cpp_checks {
namespace {
using namespace cm::aot::font_cpp;
constexpr std::uint32_t entry = 0x10750320, game_malloc = 0x10775130;
constexpr int font_array_slot = -0x3bd8;
std::vector<Write> double_at(std::uint32_t address, double value) {
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return {w32(address, std::uint32_t(bits >> 32)), w32(address + 4, std::uint32_t(bits))};
}
std::vector<Write> bits_at(std::uint32_t address, std::uint64_t bits) { return {w32(address, std::uint32_t(bits >> 32)), w32(address + 4, std::uint32_t(bits))}; }
}

FONT_CPP_CASES("selector") {
    const auto array = scenario.toc(font_array_slot);
    struct Slot { std::uint32_t slot, record; };
    std::vector<Slot> loaded, partial, empty;
    for (unsigned k = 0; k < 8; ++k) {
        const Slot slot{array + 4 * k, scenario.r32(array + 4 * k)};
        if (!slot.record || !scenario.u8(slot.record)) empty.push_back(slot);
        else if (scenario.r32(slot.record + font_record::scaler) && scenario.r32(slot.record + font_record::sfnt)) loaded.push_back(slot);
        else partial.push_back(slot);
    }
    if (std::getenv("CM_FONT_CPP_SHOW_STATE"))
        std::fprintf(stderr, "selector state: %zu loaded, %zu partial, %zu empty slots; first loaded record style %x size %u\n", loaded.size(), partial.size(),
                     empty.size(), loaded.empty() ? 0 : scenario.u16(loaded[0].record + font_record::style), loaded.empty() ? 0 : scenario.u8(loaded[0].record + font_record::size));
    if (loaded.empty() || partial.empty() || empty.empty()) return;
    const auto constants = scenario.toc(toc::skew_constants);
    std::mt19937 random(0x5e1ec7);

    const auto make = [&](const std::string& what, std::uint32_t slot, std::uint32_t style, std::uint32_t size, std::vector<Write> memory = {}) {
        Case item{.entry = entry, .registers = {{3, slot}, {4, style}, {5, size}}, .memory = std::move(memory), .label = "selector: " + what};
        return item;
    };
    const auto real = loaded[0];
    const auto real_style = scenario.u16(real.record + font_record::style), real_size = scenario.u8(real.record + font_record::size);
    const auto sfnt = scenario.r32(real.record + font_record::sfnt);
    const auto stream = scenario.r32(real.record + font_record::stream);
    const auto stream_data = scenario.r32(stream);
    (void)sfnt;

    // ---- Rare sites first (the call trace pairs only the first 32 cases of an entry): every call site and every error word once.
    // Allocation failures inside the engine's calls (the nth call of the game's malloc answered with 0): new_sfnt makes 41 allocations,
    // so 42 fails new_t2k's (the error word at 0x10750440's check), 43 the transformation's, 1 and the rest new_sfnt's.
    const auto failing = [&](const std::string& what, const Slot& slot, std::uint32_t style, std::uint32_t size, unsigned nth) {
        auto item = make(what + ", malloc " + std::to_string(nth) + " fails", slot.slot, style, size);
        item.nth_answers = {{game_malloc, nth, 0}};
        cases.push_back(item);
    };
    cases.push_back(make("recreation (bit 0)", real.slot, real_style ^ 1, real_size));
    cases.push_back(make("null record (page 0 mapped: the assertion, then the original reads through it)", 0x60000100u, 1, 12, {w32(0x60000100u, 0)}));
    failing("recreation", real, real_style ^ 1, real_size, 42);
    failing("recreation", real, real_style ^ 2, real_size, 43);
    failing("recreation", real, real_style ^ 1, real_size, 1);
    cases.push_back(make("recreation (bit 1)", real.slot, real_style ^ 2, real_size));
    cases.push_back(make("recreation of a slot with no scaler", partial[0].slot, 1, 12));
    cases.push_back(make("recreation, scaler zero", real.slot, real_style, real_size, {w32(real.record + font_record::scaler, 0)}));
    cases.push_back(make("recreation, sfnt zero", real.slot, real_style ^ 1, real_size, {w32(real.record + font_record::sfnt, 0)}));
    cases.push_back(make("transformation (bit 2)", real.slot, (real_style & 0xfff3u) | 4, real_size + 1));
    failing("transformation", real, (real_style & 0xfff3u) | 4, real_size + 3, 1);
    // The font's stream cut to nothing, then a garbled directory, so new_sfnt reports an error.
    cases.push_back(make("recreation, stream length 0 (sfnt error)", real.slot, real_style ^ 1, real_size, {w32(stream + 8, 0)}));
    {
        auto garbled = std::vector<Write>{};
        for (std::uint32_t n = 0; n < 12; ++n) garbled.push_back(w8(stream_data + n, 0xff));
        cases.push_back(make("recreation, garbled directory (sfnt error)", real.slot, real_style ^ 1, real_size, garbled));
        cases.push_back(make("recreation, garbled directory, bit 1", real.slot, real_style ^ 2, 12, garbled));
    }
    for (const unsigned nth : {2u, 3u, 4u, 5u, 8u, 16u, 24u, 32u, 40u, 41u, 44u, 45u, 46u}) failing("recreation", real, real_style ^ 1, real_size, nth);
    failing("recreation of a slot with no scaler", partial[0], 1, 12, 3);
    failing("recreation of a slot with no scaler", partial[0], 2, 12, 42);
    failing("recreation of a slot with no scaler", partial[0], 2, 12, 43);
    for (unsigned nth = 2; nth <= 6; ++nth) failing("transformation", real, (real_style & 0xfff3u) | 8, real_size + 3, nth);

    // ---- The recreation: every style's low bits against the record's, the callbacks record for bit 0 (wins) and bit 1, with and without the skew bits.
    for (const auto& slot : {loaded[0], loaded[loaded.size() > 1 ? 1 : 0], partial[0]}) {
        for (std::uint32_t style = 0; style < 0x20; ++style)
            for (const std::uint32_t size : {0u, 9u, 20u, 0xffu})
                cases.push_back(make("style " + hex(style) + " size " + std::to_string(size) + " slot " + hex(slot.slot), slot.slot, style, size));
    }
    // High bits of the registers: the style's halfword and the size's byte only.
    for (const auto style : {0x12340000u, 0xffff0001u, 0xdead0204u, 0x8000u, 0x1ffffu})
        for (const auto size : {0x100u, 0x12345609u, 0xfffffffeu})
            cases.push_back(make("wide registers style " + hex(style) + " size " + hex(size), real.slot, style, size));
    // No scaler, no sfnt, neither (the record keeps its style and size).
    cases.push_back(make("scaler zero", real.slot, real_style, real_size, {w32(real.record + font_record::scaler, 0)}));
    cases.push_back(make("sfnt zero", real.slot, real_style ^ 1, real_size, {w32(real.record + font_record::sfnt, 0)}));
    cases.push_back(make("sfnt and scaler zero", real.slot, real_style, real_size, {w32(real.record + font_record::scaler, 0), w32(real.record + font_record::sfnt, 0)}));

    // ---- The transformation only, and the early exit: same low bits, every combination of bits 2, 3 and the size.
    for (const auto& slot : loaded) {
        const auto style = scenario.u16(slot.record + font_record::style) & 0xfff3u, size = scenario.u8(slot.record + font_record::size);
        cases.push_back(make("early exit, the record's own style and size", slot.slot, scenario.u16(slot.record + font_record::style), size));
        for (const std::uint32_t skew : {0u, 4u, 8u, 12u})
            for (const std::uint32_t other : {size, size + 1, 0u, 1u, 0x7fu, 0x80u, 0xffu, 0x60u}) {
                cases.push_back(make("transformation skew " + hex(skew) + " size " + std::to_string(other) + " slot " + hex(slot.slot), slot.slot, style | skew, other));
                cases.back().clear_fpscr = (other & 1) != 0;
            }
    }

    // ---- Floating point: the constants moved to the conversion's thresholds (read from guest memory, so a host literal would not follow).
    const double magic = 4503599627370496.0 + 0.0;
    (void)magic;
    struct Constant { const char* name; std::uint32_t offset; };
    const Constant constants_to_poke[] = {{"angle bit 2", 0}, {"angle bit 3", 8}, {"angle otherwise", 0x10}, {"skew factor", 0x18}, {"magic", 0x20}};
    const double values[] = {0.0, -0.0, 1.0, -1.0, 0.5, 1e-300, 1e300, -1e300, 1e10, 3e9, 2147483647.5, -2147483648.5, 1.5, 2.5, -2.5};
    for (const auto& constant : constants_to_poke) {
        for (const auto value : values) {
            auto memory = double_at(constants + constant.offset, value);
            cases.push_back(make(std::string("constant ") + constant.name + " = " + std::to_string(value), real.slot, (real_style & 0xfff3u) | 0xc, 77, memory));
            cases.back().clear_fpscr = true;
        }
        for (const std::uint64_t bits : {0x7ff0000000000000ull, 0xfff0000000000000ull, 0x7ff8000000000000ull, 0x7ff0000000000001ull, 0x8000000000000001ull, 0x0000000000000001ull}) {
            cases.push_back(make(std::string("constant ") + constant.name + " bits " + hex(std::uint32_t(bits >> 32)) + ":" + hex(std::uint32_t(bits)), real.slot,
                                 (real_style & 0xfff3u) | 4, 33, bits_at(constants + constant.offset, bits)));
            cases.back().clear_fpscr = true;
        }
    }
    // The selected angle's value and the size at which the product crosses an integer: sweep the sizes over a skew of 0.2 (the real constant).
    for (std::uint32_t size = 0; size < 256; size += 5)
        cases.push_back(make("sweep size " + std::to_string(size), loaded[loaded.size() - 1].slot, (real_style & 0xfff3u) | 4 | ((size & 1) << 3), size));

    // ---- Unloaded records and the null record (page 0 mapped: the original reads through them).
    for (const auto& slot : empty) {
        cases.push_back(make("empty record " + hex(slot.slot), slot.slot, 1, 12));
        cases.push_back(make("empty record, skew " + hex(slot.slot), slot.slot, 0, 12));
    }
    (void)random;
}
}
