// The font engine, block `t2kextra` (T2KEXTRA.C, 0x10761070): cases for the colour table rendering. Dormant for the game (the character renderer
// runs it only for command bit 0x80, and the game asks for cmd = 2), so no capture and no end-to-end run reaches it: these cases are its only
// evidence. The T2K record is built in the scratch area with the fields the routine reads (mem, left, +0xc4, width, height, row stride, bitmap);
// the old bitmap is either a live block of a real tsiMemObject (released through tsi_DeAllocMem on the real allocator) or the memory object's
// scratch buffer of slot 5 (marked free instead), and the new bitmap comes from the real tsi_AllocMem. Every outcome of the routine's branches
// is a case: empty and negative sizes, all-zero, all-255 and ramp pixel rows, each pixel value 0..255 (the filter's rounding), strides narrower
// and wider than the width, the slot-5 test both ways. The null result of tsi_AllocMem (the assertion after it) cannot happen: tsi_AllocMem
// raises instead of returning null, so that branch is not reached by any state (it is written as the original's). Labels start with
// "t2kextra:" (CM_FONT_ENGINE_BLOCK=t2kextra).
#include "font_engine_checks.hpp"
#include <algorithm>
#include <random>

namespace cm::aot::font_engine_checks {
namespace {
constexpr std::uint32_t render = 0x10761070, game_malloc = 0x10775130, game_free = 0x10775170;
constexpr std::uint32_t record_at = scratch_base + 0x70000, source_at = scratch_base + 0x70400;
constexpr std::uint32_t block_stamp = 0xaa53c5aa;

struct Block { std::uint32_t user, size; };
struct Memory { std::uint32_t at; std::vector<Block> blocks; };
Memory memory_object(const Scenario& scenario) {
    Memory best{};
    for (const auto& [low, size] : scenario.s.memory.mapped_ranges()) {
        if (low < 0x20000000 || low >= 0x60000000) continue;
        for (std::uint64_t at = low; at + 0x17c <= std::uint64_t(low) + size; at += 4) {
            if (scenario.r32(std::uint32_t(at)) != 0xab1500ff || scenario.r32(std::uint32_t(at) + 0x178) != 0xa5a55a5a || scenario.r32(std::uint32_t(at) + 8) != 0x1f5) continue;
            Memory found{std::uint32_t(at), {}};
            const auto table = scenario.r32(std::uint32_t(at) + 0xc);
            for (std::uint32_t n = 0; n < 0x1f5; ++n) {
                const auto header = scenario.r32(table + 4 * n);
                if (header && scenario.r32(header) == block_stamp) found.blocks.push_back({header + 0x10, scenario.r32(header + 4)});
            }
            if (found.blocks.size() > best.blocks.size()) best = std::move(found);
        }
    }
    return best;
}
}

FONT_ENGINE_CASES("t2kextra") {
    const auto memory = memory_object(scenario);
    if (memory.blocks.empty()) return;
    // A block big enough for the biggest bitmap the cases read (rows of up to 40 bytes, 10 rows).
    const auto big = std::ranges::find_if(memory.blocks, [](const Block& b) { return b.size >= 600; });
    if (big == memory.blocks.end()) return;
    std::mt19937 random(0x7e2e);
    const auto add = [&](std::vector<std::pair<unsigned, std::uint32_t>> registers, std::vector<Write> writes, const std::string& label) -> Case& {
        cases.push_back({.entry = render, .registers = std::move(registers), .memory = std::move(writes), .label = "t2kextra: " + label});
        cases.back().original_owners = {game_malloc, game_free};
        return cases.back();
    };
    enum class Pixels { zero, full, ramp, random, ones, sparse };
    const auto pixel = [&](Pixels kind, unsigned n) -> std::uint32_t {
        switch (kind) {
            case Pixels::zero: return 0;
            case Pixels::full: return 0xff;
            case Pixels::ramp: return n & 0xff;
            case Pixels::ones: return 1;
            case Pixels::sparse: return n % 7 == 0 ? 0x80 + n : 0;
            default: return std::uint32_t(random());
        }
    };
    // The record: mem, left, +0xc4, width, height, old row stride, old bitmap (a live block `from_block`, or the scratch buffer of slot 5).
    const auto build = [&](std::uint32_t width, std::uint32_t height, std::uint32_t stride, Pixels kind, bool slot_five, std::uint32_t left = 0x100, std::uint32_t margin = 0x90) {
        const auto bitmap = slot_five ? source_at : big->user;
        std::vector<Write> writes{w32(record_at + 4, memory.at), w32(record_at + 0xa8, left), w32(record_at + 0xc4, margin), w32(record_at + 0xd0, width),
                                  w32(record_at + 0xd4, height), w32(record_at + 0xd8, stride), w32(record_at + 0xdc, bitmap)};
        if (slot_five) {
            writes.push_back(w32(memory.at + 0x13c, bitmap));
            writes.push_back(w32(memory.at + 0x16c, 0));
        }
        const std::uint32_t rows = std::min<std::uint32_t>(height, 12), columns = std::max(width, stride);
        for (std::uint32_t r = 0; r < rows; ++r)
            for (std::uint32_t x = 0; x < std::min<std::uint32_t>(columns, 48); ++x) writes.push_back(w8(bitmap + r * stride + x, pixel(kind, r * 48 + x)));
        return writes;
    };
    const std::pair<unsigned, std::uint32_t> t2k{3, record_at};
    // The new bitmap lands where the allocator's first fit is (0x20f3aca0 in this snapshot, found by running a case): that heap is dirtied first, so the
    // routine's zero fill shows (a fresh block's bytes are otherwise zero already). A different snapshot just makes this write harmless.
    const auto dirty = [&](std::vector<Write> writes) {
        for (std::uint32_t n = 0; n < 0x300; n += 4) writes.push_back(w32(0x20f3aca0 + n, 0xa5a5a5a5));
        return writes;
    };
    // Shapes x pixel kinds x where the old bitmap lives.
    struct Size { std::uint32_t width, height, stride; };
    const std::vector<Size> sizes = {{0, 0, 0}, {0, 3, 4}, {3, 0, 4}, {1, 1, 1}, {2, 1, 2}, {5, 2, 5}, {8, 3, 8}, {9, 2, 9}, {13, 4, 16}, {16, 3, 16}, {7, 5, 3},
                                     {4, 4, 40}, {40, 8, 40}, {17, 9, 19}, {1, 10, 1}};
    for (const auto& size : sizes)
        for (const auto kind : {Pixels::zero, Pixels::full, Pixels::ramp, Pixels::random, Pixels::sparse})
            for (const bool slot_five : {false, true})
                add({t2k}, dirty(build(size.width, size.height, size.stride, kind, slot_five)),
                    "size " + std::to_string(size.width) + "x" + std::to_string(size.height) + " stride " + std::to_string(size.stride) + " kind " + std::to_string(int(kind)) + (slot_five ? " slot five" : " block"));
    // Every pixel value 0..255 (the taps' roundings), in rows of 32.
    for (unsigned base = 0; base < 256; base += 32) {
        auto writes = build(32, 1, 32, Pixels::zero, true);
        for (unsigned n = 0; n < 32; ++n) writes.push_back(w8(source_at + n, base + n));
        add({t2k}, writes, "values " + hex(base) + ".." + hex(base + 31) + " slot five");
        add({t2k}, [&] { auto w = build(32, 1, 32, Pixels::zero, false); for (unsigned n = 0; n < 32; ++n) w.push_back(w8(big->user + n, base + n)); return w; }(), "values " + hex(base) + ".." + hex(base + 31) + " block");
    }
    // The left and the second field around 0x80 (the subtraction wraps).
    for (const std::uint32_t left : {0u, 0x7fu, 0x80u, 0x81u, 0xffffffffu, 0x80000000u})
        add({t2k}, build(4, 2, 4, Pixels::ramp, true, left, left + 5), "left " + hex(left));
    // The old bitmap is the slot-five buffer but the flag was set already, and a slot-five pointer that is not the bitmap (the block is released).
    add({t2k}, [&] { auto w = build(6, 2, 6, Pixels::ramp, true); w.push_back(w32(memory.at + 0x16c, 1)); return w; }(), "slot five flag already set");
    add({t2k}, [&] { auto w = build(6, 2, 6, Pixels::ramp, false); w.push_back(w32(memory.at + 0x13c, big->user + 4)); return w; }(), "slot five holds another pointer");
    add({t2k}, [&] { auto w = build(6, 2, 6, Pixels::ramp, false); w.push_back(w32(memory.at + 0x13c, 0)); return w; }(), "slot five empty");
    // A size of exactly one byte through the wrap: width -3 makes the stride 1 (the pixel loop does not run, the fill does).
    add({t2k}, dirty(build(0xfffffffd, 1, 1, Pixels::zero, true)), "size one byte (width -3, height 1)");
    // Sizes whose product overflows or goes negative: the allocation of a huge size fails on the real allocator (both sides alike), a zero or negative size is skipped by the fill.
    for (const auto& size : std::vector<Size>{{0x7ffffffc, 1, 1}, {0xfffffffb, 1, 1}, {0x10000, 0x10000, 1}, {5, 0xffffffff, 5}, {5, 0x80000000, 5}})
        add({t2k}, build(size.width, 0, size.stride, Pixels::zero, true), "width " + hex(size.width) + " height " + hex(size.height) + " (written only)").memory.push_back(w32(record_at + 0xd4, size.height));
    // Random shapes.
    for (unsigned n = 0; n < 60; ++n) {
        const Size size{random() % 20, random() % 7, random() % 24};
        add({t2k}, build(size.width, size.height, size.stride, Pixels::random, random() % 2 == 0, std::uint32_t(random()), std::uint32_t(random())),
            "random " + std::to_string(n) + " " + std::to_string(size.width) + "x" + std::to_string(size.height) + " stride " + std::to_string(size.stride));
    }
}
}
